/* concatn.h: concatenate a variable number of strings.
   This is a separate include file only because I don't see the point of
   having every source file include <c-vararg.h>.  The declarations for
   the other concat routines are in lib.h.

Copyright (C) 1993, 96 Karl Berry.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef KPATHSEA_CONCATN_H
#define KPATHSEA_CONCATN_H

#include <kpathsea/c-proto.h>
#include <kpathsea/c-vararg.h>
#include <kpathsea/types.h>

/* Concatenate a null-terminated list of strings and return the result
   in malloc-allocated memory.  */
extern KPSEDLL string concatn PVAR1H(const_string str1);

#endif /* not KPATHSEA_CONCATN_H */
