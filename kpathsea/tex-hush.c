/* tex-hush.c: are we suppressing warnings?

Copyright (C) 1996 Karl Berry.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <kpathsea/config.h>

#include <kpathsea/pathsearch.h>
#include <kpathsea/tex-hush.h>
#include <kpathsea/variable.h>

boolean
kpse_tex_hush P1C(const_string, what)
{
  string h;
  string hush = kpse_var_value ("TEX_HUSH");
  if (hush) {
    for (h = kpse_path_element (hush); h; h = kpse_path_element (NULL)) {
      /* Don't do anything special with empty elements.  */
      if (STREQ (h, what) || STREQ (h, "all"))
        return true;
    }
  }
  
  return false;
}
