/* types.h: general types.

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

#ifndef KPATHSEA_TYPES_H
#define KPATHSEA_TYPES_H

/* Booleans.  */
typedef enum { false = 0, true = 1 } boolean;

/* The X11 library defines `FALSE' and `TRUE', and so we only want to
   define them if necessary.  */
#ifndef FALSE
#define FALSE false
#define TRUE true
#endif /* FALSE */

/* The usual null-terminated string.  */
typedef char *string;

/* A pointer to constant data.  (ANSI says `const string' is
   `char const *', which is a constant pointer, which is not what we
   typically want.)  */
typedef const char *const_string;

/* A generic pointer.  */
typedef void *address;

#endif /* not KPATHSEA_TYPES_H */
