/* fontmap.c: read a file for additional font names.

Copyright (C) 1993, 94 Free Software Foundation, Inc.

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
#include <kpathsea/fontmap.h>
#include <kpathsea/hash.h>
#include <kpathsea/line.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/str-list.h>


/* Look up KEY in MAP; if it's not found, remove any suffix from KEY and
   try again.  */

string *
map_lookup P2C(hash_table_type, map,  const_string, key)
{
  string suffix = find_suffix (key);
  string *ret = hash_lookup (map, key);
  
  if (!ret)
    {
      /* OK, the original KEY didn't work.  Let's check for the KEY without
         an extension -- perhaps they gave foobar.tfm, but the mapping only
         defines `foobar'.  */
      if (suffix)
        {
          string base_key = remove_suffix (key);
          
          ret = hash_lookup (map, base_key);

          free (base_key);
        }
    }

  /* Append the original suffix, if we had one.  */
  if (ret && suffix)
    while (*ret)
      {
       *ret = extend_filename (*ret, suffix);
       ret++;
      }

  return ret;
}

/* Open and read the mapping file MAP_FILENAME, putting its entries into
   MAP. Comments begin with % and continue to the end of the line.  Each
   line of the file defines an entry: the first word is the real
   filename (e.g., `ptmr'), the second word is the alias (e.g.,
   `Times-Roman'), and any subsequent words are ignored.  .tfm is added
   if either the filename or the alias have no extension.  This is the
   same order as in Dvips' psfonts.map; unfortunately, we can't have TeX
   read that same file, since most of the real filenames start with an
   `r', because of the virtual fonts Dvips uses.  */

static void
map_file_parse P2C(hash_table_type *, map,  const_string, map_filename)
{
  extern FILE *xfopen ();	/* In xfopen.c.  */
  char *l;
  unsigned map_lineno = 0;
  FILE *f = xfopen (map_filename, FOPEN_R_MODE);
  
  while ((l = read_line (f)) != NULL)
    {
      string filename;
      string comment_loc = strchr (l, '%');
      
      map_lineno++;
      
      /* Ignore anything after a %.  */
      if (comment_loc)
        *comment_loc = 0;
      
      /* If we don't have any filename, that's ok, the line is blank.  */
      filename = strtok (l, " \t");
      if (filename)
        {
          string alias = strtok (NULL, " \t");
          
          /* But if we have a filename and no alias, something's wrong.  */
          if (alias == NULL || *alias == 0)
            fprintf (stderr, "%s:%u: Alias missing for filename `%s'.\n",
                     map_filename, map_lineno, filename);
          else
            { /* We've got everything.  Insert the new entry.  */
              hash_insert (map, xstrdup (alias), xstrdup (filename));
            }
        }
      
      free (l);
    }
  
  xfclose (f, map_filename);
}

/* Search for all the MAP_NAME's in PATH.  */

#define MAP_NAME "texfonts.map"

hash_table_type
map_create P1C(const_string, path)
{
  string *filenames = kpse_all_path_search (path, MAP_NAME);
  hash_table_type map; /* some old compilers ... */
  map = hash_create (751);
  
  while (*filenames)
    {
      map_file_parse (&map, *filenames);
      filenames++;
    }

  return map;
}
