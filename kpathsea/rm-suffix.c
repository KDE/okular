/* rm-suffix.c: remove any suffix.

Copyright (C) 1992, 93, 95 Free Software Foundation, Inc.

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


/* Generic const warning -- see extend-fname.c.  */

string
remove_suffix P1C(const_string, s)
{
  string ret;
  const_string suffix = find_suffix (s);
  
  if (suffix)
    {
      /* Back up to before the dot.  */
      suffix--;
      ret = (string) xmalloc (suffix - s + 1);
      strncpy (ret, s, suffix - s);
      ret[suffix - s] = 0;
    }
  else
    ret = (string) s;
    
  return ret;
}
