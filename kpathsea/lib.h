/* lib.h: declarations for common, low-level routines in kpathsea.

Copyright (C) 1992, 93, 94, 95, 96 Free Software Foundation, Inc.

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

#ifndef KPATHSEA_LIB_H
#define KPATHSEA_LIB_H

#include <kpathsea/types.h>

/* Define common sorts of messages.  */

/* This should be called only after a system call fails.  Don't exit
   with status `errno', because that might be 256, which would mean
   success (exit statuses are truncated to eight bits).  */
#define FATAL_PERROR(str) do { \
  fprintf (stderr, "%s: ", program_invocation_name); \
  perror (str); exit (EXIT_FAILURE); } while (0)

#define START_FATAL() do { \
  fprintf (stderr, "%s: fatal: ", program_invocation_name);
#define END_FATAL() fputs (".\n", stderr); exit (1); } while (0)

#define FATAL(str)							\
  START_FATAL (); fputs (str, stderr); END_FATAL ()
#define FATAL1(str, e1)							\
  START_FATAL (); fprintf (stderr, str, e1); END_FATAL ()
#define FATAL2(str, e1, e2)						\
  START_FATAL (); fprintf (stderr, str, e1, e2); END_FATAL ()
#define FATAL3(str, e1, e2, e3)						\
  START_FATAL (); fprintf (stderr, str, e1, e2, e3); END_FATAL ()
#define FATAL4(str, e1, e2, e3, e4)					\
  START_FATAL (); fprintf (stderr, str, e1, e2, e3, e4); END_FATAL ()
#define FATAL5(str, e1, e2, e3, e4, e5)					\
  START_FATAL (); fprintf (stderr, str, e1, e2, e3, e4, e5); END_FATAL ()
#define FATAL6(str, e1, e2, e3, e4, e5, e6)				\
  START_FATAL (); fprintf (stderr, str, e1, e2, e3, e4, e5, e6); END_FATAL ()


#define START_WARNING() do { fputs ("warning: ", stderr)
#define END_WARNING() fputs (".\n", stderr); fflush (stderr); } while (0)

#define WARNING(str)							\
  START_WARNING (); fputs (str, stderr); END_WARNING ()
#define WARNING1(str, e1)						\
  START_WARNING (); fprintf (stderr, str, e1); END_WARNING ()
#define WARNING2(str, e1, e2)						\
  START_WARNING (); fprintf (stderr, str, e1, e2); END_WARNING ()
#define WARNING3(str, e1, e2, e3)					\
  START_WARNING (); fprintf (stderr, str, e1, e2, e3); END_WARNING ()
#define WARNING4(str, e1, e2, e3, e4)					\
  START_WARNING (); fprintf (stderr, str, e1, e2, e3, e4); END_WARNING ()


/* I find this easier to read.  */
#define STREQ(s1, s2) (strcmp (s1, s2) == 0)
#define STRNEQ(s1, s2, n) (strncmp (s1, s2, n) == 0)
      
/* Support for FAT/ISO-9660 filesystems.  Theoretically this should be
   done at runtime, per filesystem, but that's painful to program.  */
#ifdef MONOCASE_FILENAMES
#define FILESTRCASEEQ(s1, s2) (strcasecmp (s1, s2) == 0)
#define FILESTRNCASEEQ(s1, s2, l) (strncasecmp (s1, s2, l) == 0)
#define FILECHARCASEEQ(c1, c2) (toupper (c1) == toupper (c2))
#else
#define FILESTRCASEEQ STREQ
#define FILESTRNCASEEQ STRNEQ
#define FILECHARCASEEQ(c1, c2) ((c1) == (c2))
#endif

/* This is the maximum number of numerals that result when a 64-bit
   integer is converted to a string, plus one for a trailing null byte,
   plus one for a sign.  */
#define MAX_INT_LENGTH 21

/* If the environment variable TEST is set, return it; otherwise,
   DEFAULT.  This is useful for paths that use more than one envvar.  */
#define ENVVAR(test, default) (getenv (test) ? (test) : (default))

/* Return a fresh copy of S1 followed by S2, et al.  */
extern KPSEDLL string concat P2H(const_string s1, const_string s2);
extern KPSEDLL string concat3 P3H(const_string, const_string, const_string);
/* `concatn' is declared in its own include file, to avoid pulling in
   all the varargs stuff.  */

/* A fresh copy of just S.  */
extern KPSEDLL string xstrdup P1H(const_string s);

/* Convert all lowercase characters in S to uppercase.  */
extern KPSEDLL string uppercasify P1H(const_string s);

/* Like `atoi', but disallow negative numbers.  */
extern KPSEDLL unsigned atou P1H(const_string);

/* True if FILENAME1 and FILENAME2 are the same file.  If stat fails on
   either name, return false, no error message.
   Cf. `SAME_FILE_P' in xstat.h.  */
extern KPSEDLL boolean same_file_p P2H(const_string filename1,
                                         const_string filename2);

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  */
extern KPSEDLL const_string xbasename P1H(const_string name);

/* Return directory part of NAME. This returns a new string. */
extern KPSEDLL string xdirname P1H(const_string name);

#ifndef HAVE_STRSTR
extern string strstr P2H(const_string haystack, const_string needle);
#endif

/* If NAME has a suffix, return a pointer to its first character (i.e.,
   the one after the `.'); otherwise, return NULL.  */
extern KPSEDLL string find_suffix P1H(const_string name);

/* Return NAME with any suffix removed.  */
extern KPSEDLL string remove_suffix P1H(const_string name);

/* Return S with the suffix SUFFIX, removing any suffix already present.
   For example, `make_suffix ("/foo/bar.baz", "quux")' returns
   `/foo/bar.quux'.  Returns a string allocated with malloc.  */
extern KPSEDLL string make_suffix P2H(const_string s,  const_string suffix);

/* Return NAME with STEM_PREFIX prepended to the stem. For example,
   `make_prefix ("/foo/bar.baz", "x")' returns `/foo/xbar.baz'.
   Returns a string allocated with malloc.  */
extern KPSEDLL string make_prefix P2H(string stem_prefix, string name);

/* If NAME has a suffix, simply return it; otherwise, return
   `NAME.SUFFIX'.  */
extern KPSEDLL string extend_filename P2H(const_string name,
                                            const_string suffix);

/* Call putenv with the string `VAR=VALUE' and abort on error.  */
extern KPSEDLL void xputenv P2H(const_string var, const_string value);
extern KPSEDLL void xputenv_int P2H(const_string var, int value);

/* Return the current working directory.  */
extern KPSEDLL string xgetcwd P1H(void);

/* Returns true if FN is a directory or a symlink to a directory.  */
extern KPSEDLL boolean dir_p P1H(const_string fn);

/* If FN is a readable directory, return the number of links it has.
   Otherwise, return -1.  */
extern KPSEDLL int dir_links P1H(const_string fn);

/* Like their stdio counterparts, but abort on error, after calling
   perror(3) with FILENAME as its argument.  */
extern KPSEDLL FILE *xfopen P2H(const_string filename, const_string mode);
extern KPSEDLL void xfclose P2H(FILE *, const_string filename);
extern KPSEDLL void xfseek P4H(FILE *, long, int, string filename);
extern KPSEDLL unsigned long xftell P2H(FILE *, string filename);

/* These call the corresponding function in the standard library, and
   abort if those routines fail.  Also, `xrealloc' calls `xmalloc' if
   OLD_ADDRESS is null.  */
extern KPSEDLL address xmalloc P1H(unsigned size);
extern KPSEDLL address xrealloc P2H(address old_address, unsigned new_size);
extern KPSEDLL address xcalloc P2H(unsigned nelem, unsigned elsize);

/* (Re)Allocate N items of type T using xmalloc/xrealloc.  */
#define XTALLOC(n, t) ((t *) xmalloc ((n) * sizeof (t)))
#define XTALLOC1(t) XTALLOC (1, t)
#define XRETALLOC(addr, n, t) ((addr) = (t *) xrealloc (addr, (n) * sizeof(t)))

#endif /* not KPATHSEA_LIB_H */
