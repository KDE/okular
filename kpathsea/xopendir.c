/* xopendir.c: opendir and closedir with error checking.

Copyright (C) 1992, 93, 94 Free Software Foundation, Inc.

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

#include <kpathsea/xopendir.h>


DIR *
xopendir P1C(string, dirname)
{
  DIR *d = opendir (dirname);

  if (d == NULL)
    FATAL_PERROR (dirname);

  return d;
}


void
xclosedir P1C(DIR *, d)
{
#ifdef CLOSEDIR_VOID
  closedir (d);
#else
  int ret = closedir (d);
  
  if (ret != 0)
    FATAL ("closedir failed");
#endif
}
