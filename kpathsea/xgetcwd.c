/* xgetcwd.c: a from-scratch version of getwd.  Ideas from the tcsh 5.20
   source, apparently uncopyrighted.

Copyright (C) 1992, 94, 96 Free Software Foundation, Inc.

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

#if defined (HAVE_GETCWD) || defined (HAVE_GETWD)
#include <kpathsea/c-pathmx.h>
#else /* not HAVE_GETCWD && not HAVE_GETWD*/
#include <kpathsea/c-dir.h>
#include <kpathsea/xopendir.h>
#include <kpathsea/xstat.h>


static void
xchdir P1C(string, dirname)
{
  if (chdir (dirname) != 0)
    FATAL_PERROR (dirname);
}

#endif /* not HAVE_GETCWD && not HAVE_GETWD */


/* Return the pathname of the current directory, or give a fatal error.  */

string
xgetcwd P1H(void)
{
  /* If the system provides getcwd, use it.  If not, use getwd if
     available.  But provide a way not to use getcwd: on some systems
     getcwd forks, which is expensive and may in fact be impossible for
     large programs like tex.  If your system needs this define and it
     is not detected by configure, let me know.
                                       -- Olaf Weber <infovore@xs4all.nl */
#if defined (HAVE_GETCWD) && !defined (GETCWD_FORKS)
  string path = xmalloc (PATH_MAX + 1);
  
  if (getcwd (path, PATH_MAX + 1) == 0)
    {
      fprintf (stderr, "getcwd: %s", path);
      exit (1);
    }
  
  return path;
#elif defined (HAVE_GETWD)
  string path = xmalloc (PATH_MAX + 1);
  
  if (getwd (path) == 0)
    {
      fprintf (stderr, "getwd: %s", path);
      exit (1);
    }
  
  return path;
#else /* not HAVE_GETCWD && not HAVE_GETWD */
  struct stat root_stat, cwd_stat;
  string cwd_path = xmalloc (2); /* In case we assign "/" below.  */
  
  *cwd_path = 0;
  
  /* Find the inodes of the root and current directories.  */
  root_stat = xstat ("/");
  cwd_stat = xstat (".");

  /* Go up the directory hierarchy until we get to root, prepending each
     directory we pass through to `cwd_path'.  */
  while (!SAME_FILE_P (root_stat, cwd_stat))
    {
      struct dirent *e;
      DIR *parent_dir;
      boolean found = false;
      
      xchdir ("..");
      parent_dir = xopendir (".");

      /* Look through the parent directory for the entry with the same
         inode, so we can get its name.  */
      while ((e = readdir (parent_dir)) != NULL && !found)
        {
          struct stat test_stat;
          test_stat = xlstat (e->d_name);
          
          if (SAME_FILE_P (test_stat, cwd_stat))
            {
              /* We've found it.  Prepend the pathname.  */
              string temp = cwd_path;
              cwd_path = concat3 ("/", e->d_name, cwd_path);
              free (temp);
              
              /* Set up to test the next parent.  */
              cwd_stat = xstat (".");
              
              /* Stop reading this directory.  */
              found = true;
            }
        }
      if (!found)
        FATAL2 ("No inode %d/device %d in parent directory",
                cwd_stat.st_ino, cwd_stat.st_dev);
      
      xclosedir (parent_dir);
    }
  
  /* If the current directory is the root, cwd_path will be the empty
     string, and we will have not gone through the loop.  */
  if (*cwd_path == 0)
    strcpy (cwd_path, "/");
  else
    /* Go back to where we were.  */
    xchdir (cwd_path);

#ifdef DOSISH
  /* Prepend the drive letter to CWD_PATH, since this technique
     never tells us what the drive is.
 
     Note that on MS-DOS/MS-Windows, the branch that works around
     missing `getwd' will probably only work for DJGPP (which does
     have `getwd'), because only DJGPP reports meaningful
     st_ino numbers.  But someday, somebody might need this...  */
  {
    char drive[3];
    string temp = cwd_path;

    /* Make the drive letter lower-case, unless it is beyond Z: (yes,
       there ARE such drives, in case of Novell Netware on MS-DOS).  */
    drive[0] = root_stat.st_dev + (root_stat.st_dev < 26 ? 'a' : 'A');
    drive[1] = ':';
    drive[2] = '\0';

    cwd_path = concat (drive, cwd_path);
    free (temp);
  }
#endif

  return cwd_path;
#endif /* not HAVE_GETCWD && not HAVE_GETWD */
}
