/* line.h: read an arbitrary-length input line.

Copyright (C) 1992 Free Software Foundation, Inc.

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

#ifndef LINE_H
#define LINE_H

#include <stdio.h>
#include <kpathsea/types.h>


/* Return NULL if we are at EOF, else the next line of F.  The newline
   character at the end of string is removed.  The string is allocated
   with malloc.  */
extern string read_line P1H(FILE *f);

#endif /* not LINE_H */
