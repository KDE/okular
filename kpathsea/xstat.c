/* xstat.c: stat and (maybe) lstat with error checking.

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
