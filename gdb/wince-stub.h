/* wince-stub.h -- Definitions for commnicating with the WinCE stub.

   Copyright (C) 1999, 2000, 2007 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions, A Red Hat Company.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without eve nthe implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

/* by Christopher Faylor (cgf@cygnus.com) */

enum win_func
  {
    GDB_CREATEPROCESS = 42,
    GDB_TERMINATEPROCESS,
    GDB_WAITFORDEBUGEVENT,
    GDB_GETTHREADCONTEXT,
    GDB_SETTHREADCONTEXT,
    GDB_READPROCESSMEMORY,
    GDB_WRITEPROCESSMEMORY,
    GDB_THREADALIVE,
    GDB_SUSPENDTHREAD,
    GDB_RESUMETHREAD,
    GDB_CONTINUEDEBUGEVENT,
    GDB_CLOSEHANDLE,
    GDB_STOPSTUB,
    GDB_SINGLESTEP,
    GDB_SETBREAK,
    GDB_INVALID
  };

typedef unsigned char gdb_wince_id;
typedef unsigned short gdb_wince_len;
typedef short gdb_wince_result;
