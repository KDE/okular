/* xstat.c: stat and (maybe) lstat with error checking.

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

#include <kpathsea/xstat.h>


struct stat
xstat P1C(const_string, path)
{
  struct stat s;
  
  if (stat (path, &s) != 0)
    FATAL_PERROR (path);
  
  return s;
}


/* If we don't have symbolic links, lstat is the same as stat, and
   a #define is made in the include file.  We declare lstat to avoid an
   implicit declaration warning for development; sigh.  */

#ifdef S_ISLNK
extern int lstat ();
struct stat
xlstat P1C(const_string, path)
{
  struct stat s;
  
  if (lstat (path, &s) != 0)
    FATAL_PERROR (path);
  
  return s;
}
#endif
