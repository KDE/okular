/* fn.h: arbitrarily long filenames (or just strings).

Copyright (C) 1993 Karl Berry.

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

#ifndef KPATHSEA_FN_H
#define KPATHSEA_FN_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Arbitrarily long filenames; it's inconvenient to use obstacks here,
   because we want to maintain a null terminator.  Also used for
   dynamically growing strings even when the null byte isn't necessary,
   e.g., in `variable.c', since I don't want to pass obstacks around
   everywhere, and one can't free parts of an obstack arbitrarily.  */

typedef struct
{
  string str;
  unsigned allocated;
  unsigned length; /* includes the terminating null byte, if any */
} fn_type;

#define FN_STRING(fn) ((fn).str)
#define FN_ALLOCATED(fn) ((fn).allocated)
#define FN_LENGTH(fn) ((fn).length)


/* Create a new empty fn.  */
extern fn_type fn_init P1H(void);

/* Create a new fn from the first LEN characters from S and a null.  */
extern fn_type fn_copy0 P2H(const_string s,  unsigned len);

/* Free what's been allocated.  Can also just free the string if it's
   been extracted out.  Fatal error if nothing allocated in F.  */
extern void fn_free P1H(fn_type *f);

/* Append the character C to the fn F.  Don't append trailing null.  */
extern void fn_1grow P2H(fn_type *f, char c);

/* Append LENGTH bytes from SOURCE to F.  */
extern void fn_grow P3H(fn_type *f, address source, unsigned length);

/* Concatenate the component S to the fn F.  Assumes string currently in
   F is null terminated.  */
extern void fn_str_grow P2H(fn_type *f, const_string s);

/* Add a null to F's string at position LOC, and update its length.
   Fatal error if LOC is past the end of the string.  */
extern void fn_shrink_to P2H(fn_type *f, unsigned loc);

#endif /* not KPATHSEA_FN_H */
