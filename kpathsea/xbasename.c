/* basename.c: return the last element in a path.

Copyright (C) 1992, 94, 95, 96 Free Software Foundation, Inc.

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

/* Have to include this first to get c-auto.h.  */
#include <kpathsea/config.h>

#include <kpathsea/c-pathch.h>

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  For example, `basename ("/foo/bar.baz")'
   returns "bar.baz".  */

const_string
xbasename P1C(const_string, name)
{
  const_string base = NULL;
  unsigned len = strlen (name);
  
  for (len = strlen (name); len > 0; len--) {
    if (IS_DIR_SEP (name[len - 1]) || IS_DEVICE_SEP (name[len - 1])) {
      base = name + len;
      break;
    }
  }

  if (!base)
    base = name;
  
  return base;
}
