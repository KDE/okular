/* basename.c: return the last element in a path.

Copyright (C) 1992, 94, 95 Free Software Foundation, Inc.

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

/* Have to include this first to get c-auto.h.  */
#include <kpathsea/config.h>

#ifndef HAVE_BASENAME /* rest of file */

#include <kpathsea/c-pathch.h>

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  For example, `basename ("/foo/bar.baz")'
   returns "bar.baz".  */

const_string
basename P1C(const_string, name)
{
  const_string base = NULL;
  unsigned len = strlen (name);
  
  for (len = strlen (name); len > 0; len--)
    {
      if (IS_DIR_SEP (name[len - 1]))
        {
          base = name + len;
          break;
        }
    }

  if (!base)
    base = name;
  
  return base;
}

#endif /* not HAVE_BASENAME */
