/* find-suffix.c: return the stuff after a dot.

Copyright (C) 1992, 93 Free Software Foundation, Inc.

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

#include <kpathsea/c-pathch.h>


/* Return pointer to first character after `.' in last directory element
   of NAME.  If the name is `foo' or `/foo.bar/baz', we have no extension.  */

string
find_suffix P1C(const_string, name)
{
  const_string slash_pos;
  string dot_pos = strrchr (name, '.');
  
  if (dot_pos == NULL)
    return NULL;
  
  for (slash_pos = name + strlen (name);
       slash_pos > dot_pos && !IS_DIR_SEP (*slash_pos);
       slash_pos--)
    ;
  
  return slash_pos > dot_pos ? NULL : dot_pos + 1;
}



