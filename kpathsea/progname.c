/* progname.c: the executable name we were invoked as; general initialization.

Copyright (C) 1994, 96, 97 Karl Berry.

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
#include <kpathsea/absolute.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/pathsearch.h>
/* For kpse_reset_progname */
#include <kpathsea/tex-file.h>

#ifdef WIN32
#include <kpathsea/c-pathmx.h>
#endif

/* NeXT does not define the standard macros, but has the equivalent.
   WIN32 doesn't define them either, and doesn't have them.
   From: Gregor Hoffleit <flight@mathi.uni-heidelberg.de>.  */
#ifndef S_IXUSR
#ifdef WIN32
#define S_IXUSR 0
#define S_IXGRP 0
#define S_IXOTH 0
#else /* not WIN32 */
#define S_IXUSR 0100
#endif /* not WIN32 */
#endif /* not S_IXUSR */
#ifndef S_IXGRP
#define S_IXGRP 0010
#endif
#ifndef S_IXOTH
#define S_IXOTH 0001
#endif

#ifndef HAVE_PROGRAM_INVOCATION_NAME
/* Don't redefine the variables if glibc already has.  */
string program_invocation_name = NULL;
string program_invocation_short_name = NULL;
#endif
/* And the variable for the program we pretend to be. */
string kpse_program_name = NULL;

#ifndef WIN32
/* From a standalone program `ll' to expand symlinks written by Kimbo Mundy.
   Don't bother to compile if we don't have symlinks; thus we can assume
   / as the separator.  Also don't try to use basename, etc., or
   handle arbitrary filename length.  Mixed case function names because
   that's what kimbo liked.  */

#ifdef S_ISLNK
static int ll_verbose = 0;
static int ll_loop = 0;

#undef BSIZE
#define BSIZE 2048 /* sorry */


/* Read link FN into SYM.  */

static void
ReadSymLink (fn, sym)
     char *fn, *sym;
{
  register int n = readlink (fn, sym, BSIZE);
  if (n < 0) {
    perror (fn);
    exit (1);
  }
  sym[n] = 0;
}


/* Strip first component from S, and also return it in a static buffer.  */

static char *
StripFirst (s)
     register char *s;
{
  static char buf[BSIZE];
  register char *s1;

  /* Find the end of the first path element */
  for (s1 = s; *s1 && (*s1 != '/' || s1 == s); s1++)
    ;

  /* Copy it into buf and null-terminate it. */
  strncpy (buf, s, s1 - s);
  buf[s1 - s] = 0;

  /* Skip over the leading / (if any) */
  if (*s1 == '/')
    ++s1;

  /* Squeeze out the element */
  while ((*s++ = *s1++) != 0)
    ;

  return buf;
}


/* Strip last component from S, and also return it in a static buffer.  */

static char *
StripLast (s)
     register char *s;
{
  static char buf[BSIZE];
  register char *s1;

  for (s1 = s + strlen (s); s1 > s && *s1 != '/'; s1--)
    ;
  strcpy (buf, s1 + (*s1 == '/'));
  *s1 = 0;

  return buf;
}


/* Copy first path element from B to A, removing it from B.  */

static void 
CopyFirst (a, b)
     register char *a;
     char *b;
{
  register int length = strlen (a);

   if (length > 0 && a[length - 1] != '/') {
   a[length] = '/';
    a[length + 1] = 0;
  }
  strcat (a, StripFirst (b));
}

/* Returns NULL on error.  Prints intermediate results if global
   `ll_verbose' is nonzero.  */

#define EX(s)		(strlen (s) && strcmp (s, "/") ? "/" : "")
#define EXPOS		EX(post)
#define EXPRE		EX(pre)

static char *
expand_symlinks (s)
     char *s;
{
  static char pre[BSIZE];	/* return value */
  char post[BSIZE], sym[BSIZE], tmp[BSIZE], before[BSIZE];
  char *cp;
  char a;
  struct stat st;
  int done;

  /* Check for symlink loops.  It's difficult to check for all the
     possibilities ourselves, so let the kernel do it.  And make it
     conditional so that people can see where the infinite loop is
     being caused (see engtools#1536).  */
  if (!ll_loop) {
    FILE *f = fopen (s, "r");
    if (!f && errno == ELOOP) {
      /* Not worried about other errors, we'll get to them in due course.  */
      perror (s);
      return NULL;
    }
    if (f) fclose (f);
  }

  strcpy (post, s);
  strcpy (pre, "");

  while (strlen (post) != 0) {
    CopyFirst (pre, post);

    if (lstat (pre, &st) != 0) {
      fprintf (stderr, "lstat(%s) failed ...\n", pre);
      perror (pre);
      return NULL;
    }

    if (S_ISLNK (st.st_mode)) {
      ReadSymLink (pre, sym);

      if (!strncmp (sym, "/", 1)) {
        if (ll_verbose)
          printf ("[%s]%s%s -> [%s]%s%s\n", pre, EXPOS, post, sym, EXPOS,post);
        strcpy (pre, "");

      } else {
        a = pre[0];	/* handle links through the root */
        strcpy (tmp, StripLast (pre));
        if (!strlen (pre) && a == '/')
          strcpy (pre, "/");

        if (ll_verbose) {
          sprintf (before, "%s%s[%s]%s%s", pre, EXPRE, tmp, EXPOS, post);
          printf ("%s -> %s%s[%s]%s%s\n", before, pre, EXPRE, sym, EXPOS,post);
        }

        /* Strip "../" path elements from the front of sym; print
           new result if there were any such elements.  */
        done = 0;
        a = pre[0];	/* handle links through the root */
        while (!strncmp (sym, "..", 2)
               && (sym[2] == 0 || sym[2] == '/')
               && strlen (pre) != 0
               && strcmp (pre, ".")
               && strcmp (pre, "..")
               && (strlen (pre) < 3
                   || strcmp (pre + strlen (pre) - 3, "/.."))) {
          done = 1;
          StripFirst (sym);
          StripLast (pre);
        }

        if (done && ll_verbose) {
          for (cp = before; *cp;)
            *cp++ = ' ';
          if (strlen (sym))
            printf ("%s == %s%s%s%s%s\n", before, pre, EXPRE, sym, EXPOS,post);
          else
            printf ("%s == %s%s%s\n", before, pre, EXPOS, post);
        }
        if (!strlen (pre) && a == '/')
          strcpy (pre, "/");
      }

      if (strlen (post) != 0 && strlen (sym) != 0)
        strcat (sym, "/");

      strcat (sym, post);
      strcpy (post, sym);
    }
  }

  return pre;
}
#else /* not S_ISLNK */
#define expand_symlinks(s) (s)
#endif /* not S_ISLNK */

/* Remove .'s and ..'s in DIR, to avoid problems with relative symlinks
   as the program name, etc.  This does not canonicalize symlinks.  */

static string
remove_dots P1C(string, dir)
{
#ifdef AMIGA
  return dir;
#else
  string c;
  unsigned len;
  string ret = (string) ""; /* We always reassign.  */
  
  for (c = kpse_filename_component (dir); c;
       c = kpse_filename_component (NULL)) {
    if (STREQ (c, ".")) {
      /* If leading ., replace with cwd.  Else ignore.  */
      if (*ret == 0) {
        ret = xgetcwd ();
      }

    } else if (STREQ (c, "..")) {
      /* If leading .., start with xdirname (cwd).  Else remove last
         component from ret, if any.  */
      if (*ret == 0) {
        string dot = xgetcwd ();
        ret = xdirname (dot);
        free (dot);
      } else {
        unsigned last;
        for (last = strlen (ret);
             last > (NAME_BEGINS_WITH_DEVICE (ret) ? 2 : 0);
             last--) {
          if (IS_DIR_SEP (ret[last - 1])) {
            /* If we have `/../', that's the same as `/'.  */
            if (last > 1) {
              ret[last] = 0;
            }
            break;
          }
        }
      }

    } else {
      /* Not . or ..; just append.  Include a directory separator unless
         our string already ends with one.  This also changes all directory
         separators into the canonical DIR_SEP_STRING.  */
      string temp;
      len = strlen (ret);
      temp = concat3 (ret, ((len > 0 && ret[len - 1] == DIR_SEP)
                            || (NAME_BEGINS_WITH_DEVICE (c) && *ret == 0))
                           ? "" : DIR_SEP_STRING,
                      c);
      if (*ret)
        free (ret);
      ret = temp;
    }
  }
  
  /* Remove a trailing /, just in case it snuck in.  */
  len = strlen (ret);
  if (len > 0 && ret[len - 1] == DIR_SEP) {
    ret[len - 1] = 0;
  }

  return ret;
#endif /* not AMIGA */
}

/* Return directory ARGV0 comes from.  Check PATH if ARGV0 is not
   absolute.  */

static string
selfdir P1C(const_string, argv0)
{
  string ret = NULL;
  string self = NULL;
  
  if (kpse_absolute_p (argv0, true)) {
    self = xstrdup (argv0);
  } else {
#ifdef AMIGA
#include <dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
    BPTR lock;
    struct DosLibrary *DOSBase
      = (struct DosLibrary *) OpenLibrary ("dos.library", 0L);
    assert (DOSBase);

    self = xmalloc (BUFSIZ);
    lock = findpath (argv0);
    if (lock != ((BPTR) -1)) {
      if (getpath (lock, self) == -1) {
        *self = '\0';
      } else {
        strcat (self,DIR_SEP_STRING);
        strcat (self,argv0); 
      }
      UnLock (lock);
    }
    CloseLibrary((struct Library *) DOSBase);
#else /* not AMIGA */
    string elt;
    struct stat s;

    /* Have to check PATH.  But don't call kpse_path_search since we don't
       want to search any ls-R's or do anything special with //'s.  */
    for (elt = kpse_path_element (getenv ("PATH")); !self && elt;
         elt = kpse_path_element (NULL)) {
      string name;
      
      /* UNIX tradition interprets the empty path element as "." */
      if (*elt == 0) elt = ".";
      
      name = concat3 (elt, DIR_SEP_STRING, argv0);

      /* In order to do this perfectly, we'd have to check the owner bits only
         if we are the file owner, and the group bits only if we belong
         to the file group.  That's a lot of work, though, and it's not
         likely that kpathsea will ever be used with a program that's
         only executable by some classes and not others.  See the
         `file_status' function in execute_cmd.c in bash for what's
         necessary if we were to do it right.  */
      if (stat (name, &s) == 0 && s.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) {
        /* Do not stop at directories. */
        if (!S_ISDIR(s.st_mode)) 
          self = name;
      }
    }
#endif /* not AMIGA */
  }
  
  /* If argv0 is somehow dir/exename, `self' will still be NULL.  */
  if (!self)
    self = concat3 (".", DIR_SEP_STRING, argv0);
    
  ret = xdirname (remove_dots (expand_symlinks (self)));

  free (self);
  
  return ret;
}
#endif /* not WIN32 */

void
kpse_set_program_name P2C(const_string, argv0, const_string, progname)
{
  string ext, sdir, sdir_parent, sdir_grandparent;
  string s = getenv ("KPATHSEA_DEBUG");
#ifdef WIN32
  string debug_output = getenv("KPATHSEA_DEBUG_OUTPUT");
  int err, olderr;
#endif
  
  /* Set debugging stuff first, in case we end up doing debuggable stuff
     during this initialization.  */
  if (s) {
    kpathsea_debug |= atoi (s);
  }

#ifndef HAVE_PROGRAM_INVOCATION_NAME
#if defined(WIN32)
  /* redirect stderr to debug_output. Easier to send logfiles. */
  if (debug_output) {
    if ((err = _open(debug_output, _O_CREAT | _O_TRUNC | _O_RDWR,
                     _S_IREAD | _S_IWRITE)) == -1) {
      WARNING1("Can't open %s for stderr redirection!\n", debug_output);
      perror(debug_output);
    } else if ((olderr = _dup(fileno(stderr))) == -1) {
      WARNING("Can't dup() stderr!\n");
      close(err);
    } else if (_dup2(err, fileno(stderr)) == -1) {
      WARNING1("Can't redirect stderr to %s!\n", debug_output);
      close(olderr);
      close(err);
    } else {
      close(err);
    }
  }
  /* Win95 always gives the short filename for argv0, not the long one.
     There is only this way to catch it. It makes all the selfdir stuff
     useless for win32. */
  {
    char path[PATH_MAX], *fp;
    HANDLE hnd;
    WIN32_FIND_DATA ffd;

    /* SearchPath() always gives back an absolute directory */
    if (SearchPath(NULL, argv0, ".exe", PATH_MAX, path, &fp) == 0)
        FATAL1("Can't determine where the executable %s is.\n", argv0);
    if ((hnd = FindFirstFile(path, &ffd)) == NULL)
        FATAL1("The following path points to an invalid file : %s\n", path);
    FindClose(hnd);
    /* slashify the dirname */
    for (fp = path; fp && *fp; fp++)
        if (IS_DIR_SEP(*fp)) *fp = DIR_SEP;
    /* sdir will be the directory where the executable resides, ie: c:/TeX/bin */
    sdir = xdirname(path);
    program_invocation_name = xstrdup(ffd.cFileName);
  }

#elif defined(__DJGPP__)

  /* DJGPP programs support long filenames on Windows 95, but ARGV0 there
     is always made with the short 8+3 aliases of all the pathname elements.
     If long names are supported, we need to convert that to a long name.

     All we really need is to call `_truename', but most of the code
     below is required to deal with the special case of networked drives.  */
  if (pathconf (argv0, _PC_NAME_MAX) > 12) {
    char long_progname[PATH_MAX];

    if (_truename (argv0, long_progname)) {
      char *fp;

      if (long_progname[1] != ':') {
	/* A complication: `_truename' returns network-specific string at
	   the beginning of `long_progname' when the program resides on a
	   networked drive, and DOS calls cannot grok such pathnames.  We
	   need to convert the filesystem name back to a drive letter.  */
	char rootname[PATH_MAX], rootdir[4];

	if (argv0[0] && argv0[1] == ':')
	  rootdir[0] = argv0[0]; /* explicit drive in `argv0' */
	else
	  rootdir[0] = getdisk () + 'A';
	rootdir[1] = ':';
	rootdir[2] = '\\';
	rootdir[3] = '\0';
	if (_truename (rootdir, rootname)) {
	  /* Find out where `rootname' ends in `long_progname' and replace
	     it with the drive letter.  */
	  int root_len = strlen (rootname);

 	  if (IS_DIR_SEP (rootname[root_len - 1]))
            root_len--;	/* keep the trailing slash */
	  long_progname[0] = rootdir[0];
	  long_progname[1] = ':';
	  memmove (long_progname + 2, long_progname + root_len,
		   strlen (long_progname + root_len) + 1);
	}
      }

      /* Convert everything to canonical form.  */
      if (long_progname[0] >= 'A' && long_progname[0] <= 'Z')
	long_progname[0] += 'a' - 'A'; /* make drive lower case, for beauty */
      for (fp = long_progname; *fp; fp++)
	if (IS_DIR_SEP (*fp))
	  *fp = DIR_SEP;

      program_invocation_name = xstrdup (long_progname);
    }
    else
      /* If `_truename' failed, God help them, because we won't...  */
      program_invocation_name = xstrdup (argv0);
  }
  else
    program_invocation_name = xstrdup (argv0);

#else /* !WIN32 && !__DJGPP__ */

  program_invocation_name = xstrdup (argv0);

#endif
#endif /* not HAVE_PROGRAM_INVOCATION_NAME */

  /* We need to find SELFAUTOLOC *before* removing the ".exe" suffix from
     the program_name, otherwise the PATH search inside selfdir will fail,
     since `prog' doesn't exists as a file, there's `prog.exe' instead.  */
#ifndef WIN32
  sdir = selfdir (program_invocation_name);
#endif
  /* SELFAUTODIR is actually the parent of the invocation directory,
     and SELFAUTOPARENT the grandparent.  This is how teTeX did it.  */
  xputenv ("SELFAUTOLOC", sdir);
  sdir_parent = xdirname (sdir);
  xputenv ("SELFAUTODIR", sdir_parent);
  sdir_grandparent = xdirname (sdir_parent);
  xputenv ("SELFAUTOPARENT", sdir_grandparent);

  free (sdir);
  free (sdir_parent);
  free (sdir_grandparent);

#ifndef HAVE_PROGRAM_INVOCATION_NAME
  program_invocation_short_name = (string)xbasename (program_invocation_name);
#endif

  if (progname) {
    kpse_program_name = xstrdup (progname);
  } else {
    /* If configured --enable-shared and running from the build directory
       with the wrapper scripts (e.g., for make check), the binaries will
       be named foo.exe instead of foo.  Or possibly if we're running on a
       DOSISH system.  */
    ext = find_suffix (program_invocation_short_name);
    if (ext && FILESTRCASEEQ (ext, "exe")) {
      kpse_program_name = remove_suffix (program_invocation_short_name);
    } else {
      kpse_program_name = xstrdup (program_invocation_short_name);
    }
  }
}

/* This function is deprecated, because when we pretend to have a different
   name it will look for _that_ name in the PATH if program_invocation_name
   is not defined.  */
void
kpse_set_progname P1C(const_string, argv0)
{
  kpse_set_program_name (argv0, NULL);
}

#ifdef TEST
void
main (int argc, char **argv)
{
  puts (remove_dots ("/w/kpathsea"));
  puts (remove_dots ("/w//kpathsea"));
  puts (remove_dots ("/w/./kpathsea"));
  puts (remove_dots ("."));
  puts (remove_dots ("./"));
  puts (remove_dots ("./."));
  puts (remove_dots ("../kpathsea"));
  puts (remove_dots ("/../w/kpathsea"));
  puts (remove_dots ("/../w/kpathsea/."));
}
/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST progname.c STATIC/libkpathsea.a"
End:
*/
#endif /* TEST */
