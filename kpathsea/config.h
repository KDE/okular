/* config.h: master configuration file, included first by all compilable
   source files (not headers).

Copyright (C) 1993 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef KPATHSEA_CONFIG_H
#define KPATHSEA_CONFIG_H

/* System dependencies that are figured out by `configure'.  If we are
   compiling standalone, we get our c-auto.h.  Otherwise, the package
   containing us must provide this (unless it can somehow generate ours
   from c-auto.h.in).  We use <...> instead of "..." so that the current
   cpp directory (i.e., kpathsea/) won't be searched. */
#include <c-auto.h>

/* ``Standard'' system headers.  */
#include <kpathsea/c-std.h>

/* Macros to discard or keep prototypes.  */
#include <kpathsea/c-proto.h>

/* Our own definitions of supposedly always-useful stuff.  */
#include <kpathsea/lib.h>
#include <kpathsea/types.h>

/* Support extra runtime tracing.  */
#include <kpathsea/debug.h>

/* If you want to find subdirectories in a directory with non-Unix
   semantics (specifically, if a directory with no subdirectories does
   not have exactly two links), define this.  */
#if !defined (DOS) && !defined (VMS) && !defined (VMCMS)
#define UNIX_ST_NLINK
#endif /* not DOS and not VMS and not VMCMS */

#endif /* not KPATHSEA_CONFIG_H */
