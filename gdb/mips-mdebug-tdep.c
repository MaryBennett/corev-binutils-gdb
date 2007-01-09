/* Target-dependent code for the MDEBUG MIPS architecture, for GDB,
   the GNU Debugger.

   Copyright (C) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"
#include "frame.h"
#include "mips-tdep.h"
#include "trad-frame.h"
#include "block.h"
#include "symtab.h"
#include "objfiles.h"
#include "elf/mips.h"
#include "elf-bfd.h"
#include "gdb_assert.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "mips-mdebug-tdep.h"
#include "mdebugread.h"

#define PROC_LOW_ADDR(proc) ((proc)->pdr.adr)	/* least address */
#define PROC_FRAME_OFFSET(proc) ((proc)->pdr.frameoffset)
#define PROC_FRAME_REG(proc) ((proc)->pdr.framereg)
#define PROC_REG_MASK(proc) ((proc)->pdr.regmask)
#define PROC_FREG_MASK(proc) ((proc)->pdr.fregmask)
#define PROC_REG_OFFSET(proc) ((proc)->pdr.regoffset)
#define PROC_FREG_OFFSET(proc) ((proc)->pdr.fregoffset)
#define PROC_PC_REG(proc) ((proc)->pdr.pcreg)
/* FIXME drow/2002-06-10: If a pointer on the host is bigger than a long,
   this will corrupt pdr.iline.  Fortunately we don't use it.  */
#define PROC_SYMBOL(proc) (*(struct symbol**)&(proc)->pdr.isym)
#define _PROC_MAGIC_ 0x0F0F0F0F

struct mips_objfile_private
{
  bfd_size_type size;
  char *contents;
};

/* Global used to communicate between non_heuristic_proc_desc and
   compare_pdr_entries within qsort ().  */
static bfd *the_bfd;

static int
compare_pdr_entries (const void *a, const void *b)
{
  CORE_ADDR lhs = bfd_get_signed_32 (the_bfd, (bfd_byte *) a);
  CORE_ADDR rhs = bfd_get_signed_32 (the_bfd, (bfd_byte *) b);

  if (lhs < rhs)
    return -1;
  else if (lhs == rhs)
    return 0;
  else
    return 1;
}

static const struct objfile_data *mips_pdr_data;

static struct mdebug_extra_func_info *
non_heuristic_proc_desc (CORE_ADDR pc, CORE_ADDR *addrptr)
{
  CORE_ADDR startaddr;
  struct mdebug_extra_func_info *proc_desc;
  struct block *b = block_for_pc (pc);
  struct symbol *sym;
  struct obj_section *sec;
  struct mips_objfile_private *priv;

  find_pc_partial_function (pc, NULL, &startaddr, NULL);
  if (addrptr)
    *addrptr = startaddr;

  priv = NULL;

  sec = find_pc_section (pc);
  if (sec != NULL)
    {
      priv = (struct mips_objfile_private *) objfile_data (sec->objfile, mips_pdr_data);

      /* Search the ".pdr" section generated by GAS.  This includes most of
         the information normally found in ECOFF PDRs.  */

      the_bfd = sec->objfile->obfd;
      if (priv == NULL
	  && (the_bfd->format == bfd_object
	      && bfd_get_flavour (the_bfd) == bfd_target_elf_flavour
	      && elf_elfheader (the_bfd)->e_ident[EI_CLASS] == ELFCLASS64))
	{
	  /* Right now GAS only outputs the address as a four-byte sequence.
	     This means that we should not bother with this method on 64-bit
	     targets (until that is fixed).  */

	  priv = obstack_alloc (&sec->objfile->objfile_obstack,
				sizeof (struct mips_objfile_private));
	  priv->size = 0;
	  set_objfile_data (sec->objfile, mips_pdr_data, priv);
	}
      else if (priv == NULL)
	{
	  asection *bfdsec;

	  priv = obstack_alloc (&sec->objfile->objfile_obstack,
				sizeof (struct mips_objfile_private));

	  bfdsec = bfd_get_section_by_name (sec->objfile->obfd, ".pdr");
	  if (bfdsec != NULL)
	    {
	      priv->size = bfd_section_size (sec->objfile->obfd, bfdsec);
	      priv->contents = obstack_alloc (&sec->objfile->objfile_obstack,
					      priv->size);
	      bfd_get_section_contents (sec->objfile->obfd, bfdsec,
					priv->contents, 0, priv->size);

	      /* In general, the .pdr section is sorted.  However, in the
	         presence of multiple code sections (and other corner cases)
	         it can become unsorted.  Sort it so that we can use a faster
	         binary search.  */
	      qsort (priv->contents, priv->size / 32, 32,
		     compare_pdr_entries);
	    }
	  else
	    priv->size = 0;

	  set_objfile_data (sec->objfile, mips_pdr_data, priv);
	}
      the_bfd = NULL;

      if (priv->size != 0)
	{
	  int low, mid, high;
	  char *ptr;
	  CORE_ADDR pdr_pc;

	  low = 0;
	  high = priv->size / 32;

	  /* We've found a .pdr section describing this objfile.  We want to
	     find the entry which describes this code address.  The .pdr
	     information is not very descriptive; we have only a function
	     start address.  We have to look for the closest entry, because
	     the local symbol at the beginning of this function may have
	     been stripped - so if we ask the symbol table for the start
	     address we may get a preceding global function.  */

	  /* First, find the last .pdr entry starting at or before PC.  */
	  do
	    {
	      mid = (low + high) / 2;

	      ptr = priv->contents + mid * 32;
	      pdr_pc = bfd_get_signed_32 (sec->objfile->obfd, ptr);
	      pdr_pc += ANOFFSET (sec->objfile->section_offsets,
				  SECT_OFF_TEXT (sec->objfile));

	      if (pdr_pc > pc)
		high = mid;
	      else
		low = mid + 1;
	    }
	  while (low != high);

	  /* Both low and high point one past the PDR of interest.  If
	     both are zero, that means this PC is before any region
	     covered by a PDR, i.e. pdr_pc for the first PDR entry is
	     greater than PC.  */
	  if (low > 0)
	    {
	      ptr = priv->contents + (low - 1) * 32;
	      pdr_pc = bfd_get_signed_32 (sec->objfile->obfd, ptr);
	      pdr_pc += ANOFFSET (sec->objfile->section_offsets,
				  SECT_OFF_TEXT (sec->objfile));
	    }

	  /* We don't have a range, so we have no way to know for sure
	     whether we're in the correct PDR or a PDR for a preceding
	     function and the current function was a stripped local
	     symbol.  But if the PDR's PC is at least as great as the
	     best guess from the symbol table, assume that it does cover
	     the right area; if a .pdr section is present at all then
	     nearly every function will have an entry.  The biggest exception
	     will be the dynamic linker stubs; conveniently these are
	     placed before .text instead of after.  */

	  if (pc >= pdr_pc && pdr_pc >= startaddr)
	    {
	      struct symbol *sym = find_pc_function (pc);

	      if (addrptr)
		*addrptr = pdr_pc;

	      /* Fill in what we need of the proc_desc.  */
	      proc_desc = (struct mdebug_extra_func_info *)
		obstack_alloc (&sec->objfile->objfile_obstack,
			       sizeof (struct mdebug_extra_func_info));
	      PROC_LOW_ADDR (proc_desc) = pdr_pc;

	      PROC_FRAME_OFFSET (proc_desc)
		= bfd_get_signed_32 (sec->objfile->obfd, ptr + 20);
	      PROC_FRAME_REG (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						       ptr + 24);
	      PROC_REG_MASK (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						      ptr + 4);
	      PROC_FREG_MASK (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						       ptr + 12);
	      PROC_REG_OFFSET (proc_desc)
		= bfd_get_signed_32 (sec->objfile->obfd, ptr + 8);
	      PROC_FREG_OFFSET (proc_desc)
		= bfd_get_signed_32 (sec->objfile->obfd, ptr + 16);
	      PROC_PC_REG (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						    ptr + 28);
	      proc_desc->pdr.isym = (long) sym;

	      return proc_desc;
	    }
	}
    }

  if (b == NULL)
    return NULL;

  if (startaddr > BLOCK_START (b))
    {
      /* This is the "pathological" case referred to in a comment in
         print_frame_info.  It might be better to move this check into
         symbol reading.  */
      return NULL;
    }

  sym = lookup_symbol (MDEBUG_EFI_SYMBOL_NAME, b, LABEL_DOMAIN, 0, NULL);

  /* If we never found a PDR for this function in symbol reading, then
     examine prologues to find the information.  */
  if (sym)
    {
      proc_desc = (struct mdebug_extra_func_info *) SYMBOL_VALUE (sym);
      if (PROC_FRAME_REG (proc_desc) == -1)
	return NULL;
      else
	return proc_desc;
    }
  else
    return NULL;
}

struct mips_frame_cache
{
  CORE_ADDR base;
  struct trad_frame_saved_reg *saved_regs;
};

static struct mips_frame_cache *
mips_mdebug_frame_cache (struct frame_info *next_frame, void **this_cache)
{
  CORE_ADDR startaddr = 0;
  struct mdebug_extra_func_info *proc_desc;
  struct mips_frame_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (next_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  /* r0 bit means kernel trap */
  int kernel_trap;
  /* What registers have been saved?  Bitmasks.  */
  unsigned long gen_mask, float_mask;

  if ((*this_cache) != NULL)
    return (*this_cache);
  cache = FRAME_OBSTACK_ZALLOC (struct mips_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (next_frame);

  /* Get the mdebug proc descriptor.  */
  proc_desc = non_heuristic_proc_desc (frame_pc_unwind (next_frame),
				       &startaddr);
  /* Must be true.  This is only called when the sniffer detected a
     proc descriptor.  */
  gdb_assert (proc_desc != NULL);

  /* Extract the frame's base.  */
  cache->base = (frame_unwind_register_signed (next_frame, NUM_REGS + PROC_FRAME_REG (proc_desc))
		 + PROC_FRAME_OFFSET (proc_desc));

  kernel_trap = PROC_REG_MASK (proc_desc) & 1;
  gen_mask = kernel_trap ? 0xFFFFFFFF : PROC_REG_MASK (proc_desc);
  float_mask = kernel_trap ? 0xFFFFFFFF : PROC_FREG_MASK (proc_desc);
  
  /* Must be true.  The in_prologue case is left for the heuristic
     unwinder.  This is always used on kernel traps.  */
  gdb_assert (!in_prologue (frame_pc_unwind (next_frame), PROC_LOW_ADDR (proc_desc))
	      || kernel_trap);

  /* Fill in the offsets for the registers which gen_mask says were
     saved.  */
  {
    CORE_ADDR reg_position = (cache->base + PROC_REG_OFFSET (proc_desc));
    int ireg;

    for (ireg = MIPS_NUMREGS - 1; gen_mask; --ireg, gen_mask <<= 1)
      if (gen_mask & 0x80000000)
	{
	  cache->saved_regs[NUM_REGS + ireg].addr = reg_position;
	  reg_position -= mips_abi_regsize (gdbarch);
	}
  }

  /* Fill in the offsets for the registers which float_mask says were
     saved.  */
  {
    CORE_ADDR reg_position = (cache->base
			      + PROC_FREG_OFFSET (proc_desc));
    int ireg;
    /* Fill in the offsets for the float registers which float_mask
       says were saved.  */
    for (ireg = MIPS_NUMREGS - 1; float_mask; --ireg, float_mask <<= 1)
      if (float_mask & 0x80000000)
	{
	  if (mips_abi_regsize (gdbarch) == 4
	      && TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	    {
	      /* On a big endian 32 bit ABI, floating point registers
	         are paired to form doubles such that the most
	         significant part is in $f[N+1] and the least
	         significant in $f[N] vis: $f[N+1] ||| $f[N].  The
	         registers are also spilled as a pair and stored as a
	         double.

	         When little-endian the least significant part is
	         stored first leading to the memory order $f[N] and
	         then $f[N+1].

	         Unfortunately, when big-endian the most significant
	         part of the double is stored first, and the least
	         significant is stored second.  This leads to the
	         registers being ordered in memory as firt $f[N+1] and
	         then $f[N].

	         For the big-endian case make certain that the
	         addresses point at the correct (swapped) locations
	         $f[N] and $f[N+1] pair (keep in mind that
	         reg_position is decremented each time through the
	         loop).  */
	      if ((ireg & 1))
		cache->saved_regs[NUM_REGS + mips_regnum (current_gdbarch)->fp0 + ireg]
		  .addr = reg_position - mips_abi_regsize (gdbarch);
	      else
		cache->saved_regs[NUM_REGS + mips_regnum (current_gdbarch)->fp0 + ireg]
		  .addr = reg_position + mips_abi_regsize (gdbarch);
	    }
	  else
	    cache->saved_regs[NUM_REGS + mips_regnum (current_gdbarch)->fp0 + ireg]
	      .addr = reg_position;
	  reg_position -= mips_abi_regsize (gdbarch);
	}

    cache->saved_regs[NUM_REGS + mips_regnum (current_gdbarch)->pc]
      = cache->saved_regs[NUM_REGS + MIPS_RA_REGNUM];
  }

  /* SP_REGNUM, contains the value and not the address.  */
  trad_frame_set_value (cache->saved_regs, NUM_REGS + MIPS_SP_REGNUM, cache->base);

  return (*this_cache);
}

static void
mips_mdebug_frame_this_id (struct frame_info *next_frame, void **this_cache,
			   struct frame_id *this_id)
{
  struct mips_frame_cache *info = mips_mdebug_frame_cache (next_frame,
							   this_cache);
  (*this_id) = frame_id_build (info->base, frame_func_unwind (next_frame));
}

static void
mips_mdebug_frame_prev_register (struct frame_info *next_frame,
				 void **this_cache,
				 int regnum, int *optimizedp,
				 enum lval_type *lvalp, CORE_ADDR *addrp,
				 int *realnump, gdb_byte *valuep)
{
  struct mips_frame_cache *info = mips_mdebug_frame_cache (next_frame,
							   this_cache);
  trad_frame_get_prev_register (next_frame, info->saved_regs, regnum,
				optimizedp, lvalp, addrp, realnump, valuep);
}

static const struct frame_unwind mips_mdebug_frame_unwind =
{
  NORMAL_FRAME,
  mips_mdebug_frame_this_id,
  mips_mdebug_frame_prev_register
};

static const struct frame_unwind *
mips_mdebug_frame_sniffer (struct frame_info *next_frame)
{
  CORE_ADDR pc = frame_pc_unwind (next_frame);
  CORE_ADDR startaddr = 0;
  struct mdebug_extra_func_info *proc_desc;
  int kernel_trap;

  /* Don't use this on MIPS16.  */
  if (mips_pc_is_mips16 (pc))
    return NULL;

  /* Only use the mdebug frame unwinder on mdebug frames where all the
     registers have been saved.  Leave hard cases such as no mdebug or
     in prologue for the heuristic unwinders.  */

  proc_desc = non_heuristic_proc_desc (pc, &startaddr);
  if (proc_desc == NULL)
    return NULL;

  /* Not sure exactly what kernel_trap means, but if it means the
     kernel saves the registers without a prologue doing it, we better
     not examine the prologue to see whether registers have been saved
     yet.  */
  kernel_trap = PROC_REG_MASK (proc_desc) & 1;
  if (kernel_trap)
    return &mips_mdebug_frame_unwind;

  /* In any frame other than the innermost or a frame interrupted by a
     signal, we assume that all registers have been saved.  This
     assumes that all register saves in a function happen before the
     first function call.  */
  if (!in_prologue (pc, PROC_LOW_ADDR (proc_desc)))
    return &mips_mdebug_frame_unwind;

  return NULL;
}

static CORE_ADDR
mips_mdebug_frame_base_address (struct frame_info *next_frame,
				void **this_cache)
{
  struct mips_frame_cache *info = mips_mdebug_frame_cache (next_frame,
							   this_cache);
  return info->base;
}

static const struct frame_base mips_mdebug_frame_base = {
  &mips_mdebug_frame_unwind,
  mips_mdebug_frame_base_address,
  mips_mdebug_frame_base_address,
  mips_mdebug_frame_base_address
};

static const struct frame_base *
mips_mdebug_frame_base_sniffer (struct frame_info *next_frame)
{
  if (mips_mdebug_frame_sniffer (next_frame) != NULL)
    return &mips_mdebug_frame_base;
  else
    return NULL;
}

void
mips_mdebug_append_sniffers (struct gdbarch *gdbarch)
{
  frame_unwind_append_sniffer (gdbarch, mips_mdebug_frame_sniffer);
  frame_base_append_sniffer (gdbarch, mips_mdebug_frame_base_sniffer);
}


extern void _initialize_mips_mdebug_tdep (void);
void
_initialize_mips_mdebug_tdep (void)
{
  mips_pdr_data = register_objfile_data ();
}
