/* absolute.c: Test if a filename is absolute or explicitly relative.

Copyright (C) 1993, 94 Karl Berry.

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

#include <kpathsea/absolute.h>
#include <kpathsea/c-pathch.h>

#ifdef DOS
#include <kpathsea/c-ctype.h> /* For ISALPHA */
#endif /* DOS */


/* Sorry this is such a system-dependent mess, but I can't see any way
   to usefully generalize.  */

boolean
kpse_absolute_p P2C(const_string, filename,  boolean, relative_ok)
{
#ifdef VMS
#include <string.h>
  return strcspn (filename, "]>:") != strlen (filename);
#else /* not VMS */
  boolean absolute = IS_DIR_SEP (*filename)
#ifdef DOS
                      || ISALPHA (*filename) && filename[1] == ':'
#endif /* DOS */
		      ;
  boolean explicit_relative
    = relative_ok && (*filename == '.'
       && (IS_DIR_SEP (filename[1])
           || (filename[1] == '.' && IS_DIR_SEP (filename[2]))));

  return absolute || explicit_relative;
#endif /* not VMS */
}
