/* readable.c: check if a filename is a readable non-directory file.

Copyright (C) 1993, 95, 96 Karl Berry.

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
#include <kpathsea/c-stat.h>
#include <kpathsea/readable.h>
#include <kpathsea/tex-hush.h>
#include <kpathsea/truncate.h>


/* If access can read FN, run stat (assigning to stat buffer ST) and
   check that fn is not a directory.  Don't check for just being a
   regular file, as it is potentially useful to read fifo's or some
   kinds of devices.  */

#ifdef __DJGPP__
/* `stat' is way too expensive for such a simple job.  */
#define READABLE(fn, st) \
  (access (fn, R_OK) == 0 && access (fn, D_OK) == -1)
#elif WIN32
#define READABLE(fn, st) \
  ((GetFileAttributes(fn) != 0xFFFFFFFF) && \
   !(GetFileAttributes(fn) & FILE_ATTRIBUTE_DIRECTORY))
#else
#define READABLE(fn, st) \
  (access (fn, R_OK) == 0 && stat (fn, &(st)) == 0 && !S_ISDIR (st.st_mode))
#endif

/* POSIX invented the brain-damage of not necessarily truncating
   filename components; the system's behavior is defined by the value of
   the symbol _POSIX_NO_TRUNC, but you can't change it dynamically!
   
   Generic const return warning.  See extend-fname.c.  */

string
kpse_readable_file P1C(const_string, name)
{
  string ret;

#ifdef WIN32
  unsigned int fa, err;

  fa = GetFileAttributes(name);
  if (fa == 0xFFFFFFFF) {
    err = GetLastError();
    switch (err) {
    case ERROR_ACCESS_DENIED:
      /* Maybe warn them if permissions are bad.  */
      if (!kpse_tex_hush ("readable")) {
       fprintf(stderr, "%s: access denied.\n", name);
      }
      ret = NULL;
      break;
    case ERROR_BUFFER_OVERFLOW:
      /* should be identical to ENAMETOOLONG */
      ret = kpse_truncate_filename (name);
      fa = GetFileAttributes(ret);
      if (fa == 0xFFFFFFFF 
          || ((fa & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
      {
        if (ret != name)
          free(ret);
        ret = NULL;
      }
      break;
    default:
      /* some other error */
      ret = NULL;
    }
  } else if ((fa & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
      ret = NULL;
  } else {
    ret = (string) name;
  }
#else /* ! WIN32 */
  struct stat st;
  
  if (READABLE (name, st)) {
    ret = (string) name;

#ifdef ENAMETOOLONG
  } else if (errno == ENAMETOOLONG) {
    ret = kpse_truncate_filename (name);

    /* Perhaps some other error will occur with the truncated name, so
       let's call access again.  */
    if (!READABLE (ret, st))
      { /* Failed.  */
        if (ret != name) free (ret);
        ret = NULL;
      }
#endif /* ENAMETOOLONG */

  } else { /* Some other error.  */
    if (errno == EACCES) { /* Maybe warn them if permissions are bad.  */
      if (!kpse_tex_hush ("readable")) {
        perror (name);
      }
    }
    ret = NULL;
  }
#endif
  return ret;
}
