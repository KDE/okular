/* xcalloc.c: calloc with error checking.

Copyright (C) 1992, 93 Free Software Foundation, Inc.

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

#include <kpathsea/config.h>


address
xcalloc P2C(unsigned, nelem,  unsigned, elsize)
{
  address new_mem = (address) calloc (nelem, elsize);
  
  if (new_mem == NULL)
    {
      fprintf (stderr, "xcalloc: request for %u elements of size %u failed.\n",
               nelem, elsize);
      abort ();
    }
  
  return new_mem;
}
