/* xdirname.c: return the directory part of a path.

Copyright (C) 1999 Free Software Foundation, Inc.

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

/* Return directory for NAME.  This is "." if NAME contains no directory
   separators (should never happen for selfdir), else whatever precedes
   the final directory separator, but with multiple separators stripped.
   For example, `xdirname ("/foo//bar.baz")' returns "/foo".  Always
   return a new string.  */

#include <kpathsea/config.h>
#include <kpathsea/c-pathch.h>

string
xdirname P1C(const_string, name)
{
  string ret;
  unsigned limit = 0, loc;

  /* Ignore a NULL name. */
  if (!name)
      return NULL;
  
  if (NAME_BEGINS_WITH_DEVICE(name)
#ifdef WIN32
      || IS_UNC_NAME(name)
#endif
      ) {
    limit = 2;
  }

  for (loc = strlen (name); loc > limit && !IS_DIR_SEP (name[loc-1]); loc--)
    ;

  if (loc == limit) {
    ret = xmalloc(limit+2);
    if (limit > 0) {
      ret[0] = name[0];
      ret[1] = name[1];
    }
    ret[limit+0] = '.';
    ret[limit+1] = '\0';
  }
  else {
    /* If have ///a, must return /, so don't strip off everything.  */
    while (loc > limit+1 && IS_DIR_SEP (name[loc-1])) {
      loc--;
    }
    ret = xmalloc(loc+1);
    strncpy(ret, name, loc);
    ret[loc] = '\0';
  }
    
  return ret;
}

#ifdef TEST

char *tab[] = {
  "\\\\neuromancer\\fptex\\bin\\win32\\kpsewhich.exe",
  "\\\\neuromancer\\fptex\\win32\\kpsewhich.exe",
  "\\\\neuromancer\\fptex\\kpsewhich.exe",
  "\\\\neuromancer\\kpsewhich.exe",
  "p:\\bin\\win32\\kpsewhich.exe",
  "p:\\win32\\kpsewhich.exe",
  "p:\\kpsewhich.exe",
  "p:bin\\win32\\kpsewhich.exe",
  "p:win32\\kpsewhich.exe",
  "p:kpsewhich.exe",
  "p:///kpsewhich.exe",
  "/usr/bin/win32/kpsewhich.exe",
  "/usr/bin/kpsewhich.exe",
  "/usr/kpsewhich.exe",
  "///usr/kpsewhich.exe"
  "///kpsewhich.exe",
  NULL 
};

int main()
{
  char **p;
  for (p = tab; *p; p++)
    printf("name %s, dirname %s\n", *p, xdirname(*p));
  return 0;
}
#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST variable.c kpathsea.a"
End:
*/

