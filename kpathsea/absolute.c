/* absolute.c: Test if a filename is absolute or explicitly relative.

Copyright (C) 1993, 94, 95 Karl Berry.

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

#include <kpathsea/absolute.h>
#include <kpathsea/c-pathch.h>

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
#ifdef DOSISH
                     /* Novell allows non-alphanumeric drive letters. */
                     || (*filename && IS_DEVICE_SEP (filename[1]))
#endif /* DOSISH */
#ifdef WIN32
                     /* UNC names */
                     || (*filename == '\\' && filename[1] == '\\')
#endif
#ifdef AMIGA
		     /* Colon anywhere means a device.  */
		     || strchr (filename, ':')
#endif /* AMIGA */
		      ;
  boolean explicit_relative
    = relative_ok
#ifdef AMIGA
      /* Leading / is like `../' on Unix and DOS.  Allow Unix syntax,
         too, though, because of possible patch programs like
         `UnixDirsII' by Martin Scott.  */
      && IS_DIR_SEP (*filename) || 0
#endif /* AMIGA */
      && (*filename == '.' && (IS_DIR_SEP (filename[1])
                         || (filename[1] == '.' && IS_DIR_SEP (filename[2]))));

  /* FIXME: On UNIX an IS_DIR_SEP of any but the last character in the name
     implies relative.  */
  return absolute || explicit_relative;
#endif /* not VMS */
}
