/* readable.c: check if a filename is a readable regular file.

Copyright (C) 1993 Karl Berry.

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

#include <kpathsea/c-stat.h>
#include <kpathsea/readable.h>
#include <kpathsea/truncate.h>


/* If access can read FN, run stat (assigning to stat buffer ST) and
   check that fn is a regular file.  */

#define READABLE(fn, st) \
  (access (fn, R_OK) == 0 && stat (fn, &(st)) == 0 && S_ISREG (st.st_mode))


/* POSIX invented the brain-damage of not necessarily truncating
   filename components; the system's behavior is defined by the value of
   the symbol _POSIX_NO_TRUNC, but you can't change it dynamically!
   
   Generic const return warning.  See extend-fname.c.  */

string
kpse_readable_file P1C(const_string, name)
{
  struct stat st;
  string ret;
  
  if (READABLE (name, st))
    ret = (string) name;

#ifdef ENAMETOOLONG
  else if (errno == ENAMETOOLONG)
    {
      ret = kpse_truncate_filename (name);

      /* Perhaps some other error will occur with the truncated name, so
         let's call access again.  */
      if (!READABLE (ret, st))
        { /* Failed.  */
          if (ret != name) free (ret);
          ret = NULL;
        }
    }
#endif

  else /* Some other error.  */
    { 
      if (errno == EACCES) /* Warn them if permissions are bad.  */
        perror (name);
      ret = NULL;
    }
  
  return ret;
}
