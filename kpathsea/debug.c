/* debug.c: Help the user discover what's going on.

Copyright (C) 1993, 94 Karl Berry.

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

#include <kpathsea/config.h>

#ifdef KPSE_DEBUG /* whole file */

unsigned kpathsea_debug = 0;

/* If the real definitions of fopen or fclose are macros, we lose -- the
   #undef won't restore them. */

FILE *
fopen P2C(const_string, filename,  const_string, mode)
{
#undef fopen
  FILE *ret = fopen (filename, mode);

  if (KPSE_DEBUG_P (KPSE_DEBUG_FOPEN))
    DEBUGF3 ("fopen(%s, %s) => 0x%lx\n", filename, mode, (unsigned long) ret);

  return ret;
}

int
fclose P1C(FILE *, f)
{
#undef fclose
  int ret = fclose (f);
  
  if (KPSE_DEBUG_P (KPSE_DEBUG_FOPEN))
    DEBUGF2 ("fclose(0x%lx) => %d\n", (unsigned long) f, ret);

  return ret;
}

#endif /* KPSE DEBUG */
