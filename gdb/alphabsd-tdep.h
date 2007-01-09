/* Common target dependent code for Alpha BSD's.

   Copyright (C) 2002, 2006, 2007 Free Software Foundation, Inc.

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

#ifndef ALPHABSD_TDEP_H
#define ALPHABSD_TDEP_H

void alphabsd_supply_reg (char *, int);
void alphabsd_fill_reg (char *, int);

void alphabsd_supply_fpreg (char *, int);
void alphabsd_fill_fpreg (char *, int);


/* Functions exported from alphanbsd-tdep.c.  */

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */
extern const struct regset *
  alphanbsd_regset_from_core_section (struct gdbarch *gdbarch,
				      const char *sect_name, size_t len);

#endif /* alphabsd-tdep.h */
