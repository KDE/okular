/* line.h: read an arbitrary-length input line.

Copyright (C) 1992 Free Software Foundation, Inc.

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

#ifndef LINE_H
#define LINE_H

#include <stdio.h>
#include <kpathsea/types.h>


/* Return NULL if we are at EOF, else the next line of F.  The newline
   character at the end of string is removed.  The string is allocated
   with malloc.  */
extern KPSEDLL string read_line P1H(FILE *f);

#endif /* not LINE_H */
