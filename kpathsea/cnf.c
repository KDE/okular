/* cnf.c: read config files.

Copyright (C) 1994 Karl Berry.

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

#include <kpathsea/c-fopen.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/cnf.h>
#include <kpathsea/db.h>
#include <kpathsea/hash.h>
#include <kpathsea/line.h>
#include <kpathsea/paths.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/variable.h>


/* By using our own hash table, instead of the environment, we
   complicate variable expansion (because now we have to look in two
   places), but we don't bang so heavily on the system.  DOS and System
   V have very limited environment space.  Also, this way
   `kpse_init_format' can distinguish between values originating from
   the cnf file and ones from environment variables, which can be useful
   for users trying to figure out what's going on.  */
#ifndef CNF_HASH_SIZE
#define CNF_HASH_SIZE 751
#endif
static hash_table_type cnf_hash;

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
  
  /* If the variable is qualified with a program name, we might be
     ignoring it.  */
  while (ISSPACE (*line))
    line++;
  if (*line == '.') 
    { /* Skip spaces, then everything up to the next space or =.  */
      string prog;
      extern string program_invocation_short_name; /* must be set by main */
      
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
      
      /* If we are running `prog', fine; otherwise, we're done.  */
      assert (program_invocation_short_name);
      if (!STREQ (prog, program_invocation_short_name))
        {
          free (var);
          free (prog);
          return;
        }
    }

  /* Skip whitespace, an optional =, more whitespace.  */
  while (ISSPACE (*line))
    line++;
  if (*line == '=')
    {
      line++;
      while (ISSPACE (*line))
        line++;
    }
  
  /* Everything up to the next whitespace or eol is the value. */
  start = line;
  while (*line && !ISSPACE (*line))
    line++;
  len = line - start;
  
  value = xmalloc (len + 1);
  strncpy (value, start, len);
  value[len] = 0;
  
  /* For cnf files, multiple values for a single variable make no sense,
     but instead of removing them, we'll just take the most recent in
     `kpse_cnf_get'.  Thus, we are assuming here that `hash_insert' puts
     the most recent entries in first.  */
  hash_insert (&cnf_hash, var, value);
  
  /* We could check that anything remaining is preceded by a comment
     character, but let's not bother.  */
}

/* Read all the configuration files in the path.  */

static void
read_files P1H(void)
{
  string *cnf_files;
  const_string cnf_path = kpse_init_format (kpse_cnf_format);

  cnf_hash = hash_create (CNF_HASH_SIZE);

  for (cnf_files = kpse_all_path_search (cnf_path,
                                    kpse_format_info[kpse_cnf_format].program);
       cnf_files && *cnf_files; cnf_files++)
    {
      string line;
      string cnf_filename = *cnf_files;
      FILE *cnf_file = xfopen (cnf_filename, FOPEN_R_MODE);
      
      while ((line = read_line (cnf_file)) != NULL)
        {
          do_line (line);
          free (line);
        }

      xfclose (cnf_file, cnf_filename);
    }
  
  /* After (*after*) reading the cnf files, expand the db directory, for
     use by `elt_in_db' in pathsearch.c.  The default value of $TEXMF
     has to be able to get TEXMF from a cnf file, therefore in the
     `kpse_all_path_search' call above, we do not have DB_DIR.  */
  kpse_db_dir = kpse_var_expand (KPSE_DB_DIR);
  if (! *kpse_db_dir)
    kpse_db_dir = kpse_var_expand (DEFAULT_TEXMF);
}

/* Read the cnf files on the first call.  Return the first value in the
   returned list -- this will be from the last-read cnf file.  */

string
kpse_cnf_get P1C(const_string, name)
{
  string *ret_list;
  
  if (cnf_hash.size == 0)
    read_files ();
  
  ret_list = hash_lookup (cnf_hash, name);
  
  return ret_list ? *ret_list : NULL;
}
