/* cnf.c: read config files.

Copyright (C) 1994, 95, 96, 97 Karl Berry.

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
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/cnf.h>
#include <kpathsea/db.h>
#include <kpathsea/hash.h>
#include <kpathsea/line.h>
#include <kpathsea/paths.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/progname.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/variable.h>

/* By using our own hash table, instead of the environment, we
   complicate variable expansion (because we have to look in two
   places), but we don't bang so much on the system.  DOS and System V
   have very limited environment space.  Also, this way
   `kpse_init_format' can distinguish between values originating from
   the cnf file and ones from environment variables, which can be useful
   for users trying to figure out what's going on.  */
static hash_table_type cnf_hash;
#define CNF_HASH_SIZE 751
#define CNF_NAME "texmf.cnf"

/* Do a single line in a cnf file: if it's blank or a comment, skip it.
   Otherwise, parse <variable>[.<program>] [=] <value>.  Do
   this even if the <variable> is already set in the environment, since
   the envvalue might contain a trailing :, in which case we'll be
   looking for the cnf value.  */

static void
do_line P1C(string, line)
{
  unsigned len;
  string start;
  string value, var;
  string prog = NULL;
  
  /* Skip leading whitespace.  */
  while (ISSPACE (*line))
    line++;
  
  /* More to do only if we have non-comment material left.  */
  if (*line == 0 || *line == '%' || *line == '#')
    return;
  
  /* The variable name is everything up to the next space or = or `.'.  */
  start = line;
  while (!ISSPACE (*line) && *line != '=' && *line != '.')
    line++;

  /* `line' is now one character past the end of the variable name.  */
  len = line - start;
  var = xmalloc (len + 1);
  strncpy (var, start, len);
  var[len] = 0;
  
  /* If the variable is qualified with a program name, find out which. */
  while (ISSPACE (*line))
    line++;
  if (*line == '.') {
    /* Skip spaces, then everything up to the next space or =.  */
    line++;
    while (ISSPACE (*line))
      line++;
    start = line;
    while (!ISSPACE (*line) && *line != '=')
      line++;

    /* It's annoying to repeat all this, but making a tokenizing
       subroutine would be just as long and annoying.  */
    len = line - start;
    prog = xmalloc (len + 1);
    strncpy (prog, start, len);
    prog[len] = 0;
  }

  /* Skip whitespace, an optional =, more whitespace.  */
  while (ISSPACE (*line))
    line++;
  if (*line == '=') {
    line++;
    while (ISSPACE (*line))
      line++;
  }
  
  /* The value is whatever remains.  Remove trailing whitespace.  */
  start = line;
  len = strlen (start);
  while (ISSPACE (start[len - 1]) && len > 0)
    len--;
  
  value = xmalloc (len + 1);
  strncpy (value, start, len);
  value[len] = 0;

  /* Suppose we want to write a single texmf.cnf that can be used under
     both NT and Unix.  This is feasible except for the path separators
     : on Unix, ; on NT.  We can't switch NT to allowing :'s, since :
     is the drive separator.  So we switch Unix to allowing ;'s.  On the
     other hand, we don't want to change IS_ENV_SEP and all the rest.
     
     So, simply translate all ;'s in the path
     values to :'s if we are a Unix binary.  (Fortunately we don't use ;
     in other kinds of texmf.cnf values.)
     
     If you really want to put ; in your filenames, add
     -DALLOW_SEMICOLON_IN_FILENAMES.  (And there's no way to get :'s in
     your filenames, sorry.)  */
     
/* gcc -ansi doesn't predefine `unix', since ANSI forbids it.  And AIX
   generally doesn't predefine unix, who knows why.  HP-UX is, of course,
   also different.  Apple's MacOsX is also unix-like.  */
#ifndef unix
#if defined (__unix__) || defined (_AIX) || defined (_HPUX_SOURCE)
#define unix
#elif defined (__APPLE__) && defined (__MACH__)
#define unix
#endif
#endif

/* DJGPP defines `unix' (for portability), but generates MSDOS programs.  */
#ifndef __DJGPP__
#if !defined (ALLOW_SEMICOLON_IN_FILENAMES) && defined (unix)
  {
    string loc;
    for (loc = value; *loc; loc++) {
      if (*loc == ';')
        *loc = ':';
    }
  }
#endif
#endif

  /* We want TEXINPUTS.prog to override plain TEXINPUTS.  The simplest
     way is to put both in the hash table (so we don't have to write
     hash_delete and hash_replace, and keep track of values' sources),
     and then look up the .prog version first in `kpse_cnf_get'.  */
  if (prog) {
    string lhs = concat3 (var, ".", prog);
    free (var);
    free (prog);
    var = lhs;
  }
  hash_insert (&cnf_hash, var, value);
  
  /* We could check that anything remaining is preceded by a comment
     character, but let's not bother.  */
}

/* Read all the configuration files in the path.  */

static void
read_all_cnf P1H(void)
{
  string *cnf_files;
  const_string cnf_path = kpse_init_format (kpse_cnf_format);

  cnf_hash = hash_create (CNF_HASH_SIZE);

  for (cnf_files = kpse_all_path_search (cnf_path, CNF_NAME);
       cnf_files && *cnf_files; cnf_files++) {
    string line;
    string cnf_filename = *cnf_files;
    FILE *cnf_file = xfopen (cnf_filename, FOPEN_R_MODE);

    while ((line = read_line (cnf_file)) != NULL) {
      unsigned len = strlen (line);
      /* Strip trailing spaces. */
      while (len > 0 && ISSPACE(line[len-1])) {
        line[len - 1] = 0;
        --len;
      }
      /* Concatenate consecutive lines that end with \.  */
      while (len > 0 && line[len - 1] == '\\') {
        string next_line = read_line (cnf_file);
        line[len - 1] = 0;
        if (!next_line) {
          WARNING1 ("%s: Last line ends with \\", cnf_filename);
        } else {
          string new_line;
          new_line = concat (line, next_line);
          free (line);
          line = new_line;
          len = strlen (line);
        }
      }

      do_line (line);
      free (line);
    }

    xfclose (cnf_file, cnf_filename);
  }
}

/* Read the cnf files on the first call.  Return the first value in the
   returned list -- this will be from the last-read cnf file.  */

string
kpse_cnf_get P1C(const_string, name)
{
  string ret, try;
  string *ret_list;
  static boolean doing_cnf_init = false;

  /* When we expand the compile-time value for DEFAULT_TEXMFCNF,
     we end up needing the value for TETEXDIR and other variables,
     so kpse_var_expand ends up calling us again.  No good.  */
  if (doing_cnf_init)
    return NULL;
    
  if (cnf_hash.size == 0) {
    doing_cnf_init = true;
    read_all_cnf ();
    doing_cnf_init = false;
    
    /* Here's a pleasant kludge: Since `kpse_init_dbs' recursively calls
       us, we must call it from outside a `kpse_path_element' loop
       (namely, the one in `read_all_cnf' above): `kpse_path_element' is
       not reentrant.  */
    kpse_init_db ();
  }
  
  /* First look up NAME.`kpse_program_name', then NAME.  */
  assert (kpse_program_name);
  try = concat3 (name, ".", kpse_program_name);
  ret_list = hash_lookup (cnf_hash, try);
  free (try);
  if (ret_list) {
    ret = *ret_list;
    free (ret_list);
  } else {
    ret_list = hash_lookup (cnf_hash, name);
    if (ret_list) {
      ret = *ret_list;
      free (ret_list);
    } else {
      ret = NULL;
    }
  }
  
  return ret;

}
