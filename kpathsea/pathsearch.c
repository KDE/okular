/* pathsearch.c: look up a filename in a path.

Copyright (C) 1993, 94, 95, 97 Karl Berry.

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
#include <kpathsea/c-pathch.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/absolute.h>
#include <kpathsea/expand.h>
#include <kpathsea/db.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/readable.h>
#include <kpathsea/str-list.h>
#include <kpathsea/str-llist.h>
#include <kpathsea/variable.h>

#include <time.h> /* for `time' */

#ifdef __DJGPP__
#include <sys/stat.h>	/* for stat bits */
#endif

/* The very first search is for texmf.cnf, called when someone tries to
   initialize the TFM path or whatever.  init_path calls kpse_cnf_get
   which calls kpse_all_path_search to find all the texmf.cnf's.  We
   need to do various special things in this case, since we obviously
   don't yet have the configuration files when we're searching for the
   configuration files.  */
static boolean first_search = true;



/* This function is called after every search (except the first, since
   we definitely want to allow enabling the logging in texmf.cnf) to
   record the filename(s) found in $TEXMFLOG.  */

static void
log_search P1C(str_list_type, filenames)
{
  static FILE *log_file = NULL;
  static boolean first_time = true; /* Need to open the log file?  */
  
  if (first_time) {
    /* Get name from either envvar or config file.  */
    string log_name = kpse_var_value ("TEXMFLOG");
    first_time = false;
    if (log_name) {
      log_file = fopen (log_name, FOPEN_A_MODE);
      if (!log_file)
        perror (log_name);
      free (log_name);
    }
  }

  if (KPSE_DEBUG_P (KPSE_DEBUG_SEARCH) || log_file) {
    unsigned e;

    /* FILENAMES should never be null, but safety doesn't hurt.  */
    for (e = 0; e < STR_LIST_LENGTH (filenames) && STR_LIST_ELT (filenames, e);
         e++) {
      string filename = STR_LIST_ELT (filenames, e);

      /* Only record absolute filenames, for privacy.  */
      if (log_file && kpse_absolute_p (filename, false))
        fprintf (log_file, "%lu %s\n", (long unsigned) time (NULL),
                 filename);

      /* And show them online, if debugging.  We've already started
         the debugging line in `search', where this is called, so
         just print the filename here, don't use DEBUGF.  */
      if (KPSE_DEBUG_P (KPSE_DEBUG_SEARCH)) {
        putc (' ', stderr);
        fputs (filename, stderr);
      }
    }
  }
}

/* Concatenate each element in DIRS with NAME (assume each ends with a
   /, to save time).  If SEARCH_ALL is false, return the first readable
   regular file.  Else continue to search for more.  In any case, if
   none, return a list containing just NULL.

   We keep a single buffer for the potential filenames and reallocate
   only when necessary.  I'm not sure it's noticeably faster, but it
   does seem cleaner.  (We do waste a bit of space in the return
   value, though, since we don't shrink it to the final size returned.)  */

#define INIT_ALLOC 75  /* Doesn't much matter what this number is.  */

static str_list_type
dir_list_search P3C(str_llist_type *, dirs,  const_string, name,
                    boolean, search_all)
{
  str_llist_elt_type *elt;
  str_list_type ret;
  unsigned name_len = strlen (name);
  unsigned allocated = INIT_ALLOC;
  string potential = xmalloc (allocated);

  ret = str_list_init ();
  
  for (elt = *dirs; elt; elt = STR_LLIST_NEXT (*elt))
    {
      const_string dir = STR_LLIST (*elt);
      unsigned dir_len = strlen (dir);
      
      while (dir_len + name_len + 1 > allocated)
        {
          allocated += allocated;
          XRETALLOC (potential, allocated, char);
        }
      
      strcpy (potential, dir);
      strcat (potential, name);
      
      if (kpse_readable_file (potential))
        { 
          str_list_add (&ret, potential);
          
          /* Move this element towards the top of the list.  */
          str_llist_float (dirs, elt);
          
          /* If caller only wanted one file returned, no need to
             terminate the list with NULL; the caller knows to only look
             at the first element.  */
          if (!search_all)
            return ret;

          /* Start new filename.  */
          allocated = INIT_ALLOC;
          potential = xmalloc (allocated);
        }
    }
  
  /* If we get here, either we didn't find any files, or we were finding
     all the files.  But we're done with the last filename, anyway.  */
  free (potential);
  
  return ret;
}

/* This is called when NAME is absolute or explicitly relative; if it's
   readable, return (a list containing) it; otherwise, return NULL.  */

static str_list_type
absolute_search P1C(string, name)
{
  str_list_type ret_list;
  string found = kpse_readable_file (name);
  
  /* Some old compilers can't initialize structs.  */
  ret_list = str_list_init ();

  /* If NAME wasn't found, free the expansion.  */
  if (name != found)
    free (name);

  /* Add `found' to the return list even if it's null; that tells
     the caller we didn't find anything.  */
  str_list_add (&ret_list, found);
  
  return ret_list;
}

/* This is the hard case -- look for NAME in PATH.  If ALL is false,
   return the first file found.  Otherwise, search all elements of PATH.  */

static str_list_type
path_search P4C(const_string, path,  string, name,
                boolean, must_exist,  boolean, all)
{
  string elt;
  str_list_type ret_list;
  boolean done = false;
  ret_list = str_list_init (); /* some compilers lack struct initialization */

  for (elt = kpse_path_element (path); !done && elt;
       elt = kpse_path_element (NULL)) {
    str_list_type *found;
    boolean allow_disk_search = true;

    if (*elt == '!' && *(elt + 1) == '!') {
      /* Those magic leading chars in a path element means don't search the
         disk for this elt.  And move past the magic to get to the name.  */
      allow_disk_search = false;
      elt += 2;
    }

#if 0 /* We strip devices in the caller. */
    /* Do not touch the device if present */
    if (NAME_BEGINS_WITH_DEVICE (elt)) {
      while (IS_DIR_SEP (*(elt + 2)) && IS_DIR_SEP (*(elt + 3))) {
	*(elt + 2) = *(elt + 1);
	*(elt + 1) = *elt;
	elt++;
      }
    } else {
      /* We never want to search the whole disk.  */
      while (IS_DIR_SEP (*elt) && IS_DIR_SEP (*(elt + 1)))
        elt++;
    }
#endif
    
    /* Try ls-R, unless we're searching for texmf.cnf.  Our caller
       (search), also tests first_search, and does the resetting.  */
    found = first_search ? NULL : kpse_db_search (name, elt, all);

    /* Search the filesystem if (1) the path spec allows it, and either
         (2a) we are searching for texmf.cnf ; or
         (2b) no db exists; or 
         (2c) no db's are relevant to this elt; or
         (3) MUST_EXIST && NAME was not in the db.
       In (2*), `found' will be NULL.
       In (3),  `found' will be an empty list. */
    if (allow_disk_search && (!found || (must_exist && !STR_LIST (*found)))) {
      str_llist_type *dirs = kpse_element_dirs (elt);
      if (dirs && *dirs) {
        if (!found)
          found = XTALLOC1 (str_list_type);
        *found = dir_list_search (dirs, name, all);
      }
    }

    /* Did we find anything anywhere?  */
    if (found && STR_LIST (*found))
      if (all)
        str_list_concat (&ret_list, *found);
      else {
        str_list_add (&ret_list, STR_LIST_ELT (*found, 0));
        done = true;
      }

    /* Free the list space, if any (but not the elements).  */
    if (found) {
      str_list_free (found);
      free (found);
    }
  }

  /* Free the expanded name we were passed.  It can't be in the return
     list, since the path directories got unconditionally prepended.  */
  free (name);
  
  return ret_list;
}      

/* Search PATH for ORIGINAL_NAME.  If ALL is false, or ORIGINAL_NAME is
   absolute_p, check ORIGINAL_NAME itself.  Otherwise, look at each
   element of PATH for the first readable ORIGINAL_NAME.
   
   Always return a list; if no files are found, the list will
   contain just NULL.  If ALL is true, the list will be
   terminated with NULL.  */

static string *
search P4C(const_string, path,  const_string, original_name,
           boolean, must_exist,  boolean, all)
{
  str_list_type ret_list;
  string name;
  boolean absolute_p;

#ifdef __DJGPP__
  /* We will use `stat' heavily, so let's request for
     the fastest possible version of `stat', by telling
     it what members of struct stat do we really need.

     We need to set this on each call because this is a
     library function; the caller might need other options
     from `stat'.  Thus save the flags and restore them
     before exit.

     This call tells `stat' that we do NOT need to recognize
     executable files (neither by an extension nor by a magic
     signature); that we do NOT need time stamp of root directories;
     and that we do NOT need the write access bit in st_mode.

     Note that `kpse_set_progname' needs the EXEC bits,
     but it was already called by the time we get here.  */
  unsigned short save_djgpp_flags  = _djstat_flags;

  _djstat_flags = _STAT_EXEC_MAGIC | _STAT_EXEC_EXT
		  | _STAT_ROOT_TIME | _STAT_WRITEBIT;
#endif

  /* Make a leading ~ count as an absolute filename, and expand $FOO's.  */
  name = kpse_expand (original_name);
  
  /* If the first name is absolute or explicitly relative, no need to
     consider PATH at all.  */
  absolute_p = kpse_absolute_p (name, true);
  
  if (KPSE_DEBUG_P (KPSE_DEBUG_SEARCH))
    DEBUGF4 ("start search(file=%s, must_exist=%d, find_all=%d, path=%s).\n",
             name, must_exist, all, path);

  /* Find the file(s). */
  ret_list = absolute_p ? absolute_search (name)
                        : path_search (path, name, must_exist, all);
  
  /* Append NULL terminator if we didn't find anything at all, or we're
     supposed to find ALL and the list doesn't end in NULL now.  */
  if (STR_LIST_LENGTH (ret_list) == 0
      || (all && STR_LIST_LAST_ELT (ret_list) != NULL))
    str_list_add (&ret_list, NULL);

  /* The very first search is for texmf.cnf.  We can't log that, since
     we want to allow setting TEXMFLOG in texmf.cnf.  */
  if (first_search) {
    first_search = false;
  } else {
    /* Record the filenames we found, if desired.  And wrap them in a
       debugging line if we're doing that.  */
    if (KPSE_DEBUG_P (KPSE_DEBUG_SEARCH))
      DEBUGF1 ("search(%s) =>", original_name);
    log_search (ret_list);
    if (KPSE_DEBUG_P (KPSE_DEBUG_SEARCH))
      putc ('\n', stderr);
  }  

#ifdef __DJGPP__
  /* Undo any side effects.  */
  _djstat_flags = save_djgpp_flags;
#endif

  return STR_LIST (ret_list);
}

/* Search PATH for the first NAME.  */

string
kpse_path_search P3C(const_string, path,  const_string, name,
                     boolean, must_exist)
{
  string *ret_list = search (path, name, must_exist, false);
  string ret = *ret_list;
  free (ret_list);
  return ret;
}


/* Search all elements of PATH for files named NAME.  Not sure if it's
   right to assert `must_exist' here, but it suffices now.  */

string *
kpse_all_path_search P2C(const_string, path,  const_string, name)
{
  string *ret = search (path, name, true, true);
  return ret;
}

#ifdef TEST

void
test_path_search (const_string path, const_string file)
{
  string answer;
  string *answer_list;
  
  printf ("\nSearch %s for %s:\t", path, file);
  answer = kpse_path_search (path, file);
  puts (answer ? answer : "(nil)");

  printf ("Search %s for all %s:\t", path, file);
  answer_list = kpse_all_path_search (path, file);
  putchar ('\n');
  while (*answer_list)
    {
      putchar ('\t');
      puts (*answer_list);
      answer_list++;
    }
}

#define TEXFONTS "/usr/local/lib/tex/fonts"

int
main ()
{
  /* All lists end with NULL.  */
  test_path_search (".", "nonexistent");
  test_path_search (".", "/nonexistent");
  test_path_search ("/k" ENV_SEP_STRING ".", "kpathsea.texi");
  test_path_search ("/k" ENV_SEP_STRING ".", "/etc/fstab");
  test_path_search ("." ENV_SEP_STRING TEXFONTS "//", "cmr10.tfm");
  test_path_search ("." ENV_SEP_STRING TEXFONTS "//", "logo10.tfm");
  test_path_search (TEXFONTS "//times" ENV_SEP_STRING "."
                    ENV_SEP_STRING ENV_SEP_STRING, "ptmr.vf");
  test_path_search (TEXFONTS "/adobe//" ENV_SEP_STRING
                    "/usr/local/src/TeX+MF/typefaces//", "plcr.pfa");
  
  test_path_search ("~karl", ".bashrc");
  test_path_search ("/k", "~karl/.bashrc");

  xputenv ("NONEXIST", "nonexistent");
  test_path_search (".", "$NONEXISTENT");
  xputenv ("KPATHSEA", "kpathsea");
  test_path_search ("/k" ENV_SEP_STRING ".", "$KPATHSEA.texi");  
  test_path_search ("/k" ENV_SEP_STRING ".", "${KPATHSEA}.texi");  
  test_path_search ("$KPATHSEA" ENV_SEP_STRING ".", "README");  
  test_path_search ("." ENV_SEP_STRING "$KPATHSEA", "README");  
  
  return 0;
}

#endif /* TEST */


/*
Local variables:
test-compile-command: "gcc -posix -g -I. -I.. -DTEST pathsearch.c kpathsea.a"
End:
*/
