/* fontmap.c: read files for additional font names.

Copyright (C) 1993, 94, 95, 96, 97 Free Software Foundation, Inc.

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

#include <kpathsea/c-ctype.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/fontmap.h>
#include <kpathsea/hash.h>
#include <kpathsea/line.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/str-list.h>
#include <kpathsea/tex-file.h>

/* We have one and only one fontmap, so may as well make it static
   instead of passing it around.  */
static hash_table_type map;
#ifndef MAP_NAME
#define MAP_NAME "texfonts.map"
#endif
#ifndef MAP_HASH_SIZE
#define MAP_HASH_SIZE 4001
#endif

static const_string map_path; /* Only want to create this once. */

/* Return next whitespace-delimited token in STR or NULL if none.  */

static string
token P1C(const_string, str)
{
  unsigned len;
  const_string start;
  string ret;
  
  while (*str && ISSPACE (*str))
    str++;
  
  start = str;
  while (*str && !ISSPACE (*str))
    str++;
  
  len = str - start;
  ret = xmalloc (len + 1);
  strncpy (ret, start, len);
  ret[len] = 0;
  
  return ret;
}

/* Open and read the mapping file MAP_FILENAME, putting its entries into
   MAP. Comments begin with % and continue to the end of the line.  Each
   line of the file defines an entry: the first word is the real
   filename (e.g., `ptmr'), the second word is the alias (e.g.,
   `Times-Roman'), and any subsequent words are ignored.  .tfm is added
   if either the filename or the alias have no extension.  This is the
   same order as in Dvips' psfonts.map.  Perhaps someday the programs
   will both read the same file.  */

static void
map_file_parse P1C(const_string, map_filename)
{
  char *orig_l;
  unsigned map_lineno = 0;
  FILE *f = xfopen (map_filename, FOPEN_R_MODE);
  
  while ((orig_l = read_line (f)) != NULL) {
    string filename;
    string l = orig_l;
    string comment_loc = strrchr (l, '%');
    if (!comment_loc) {
      comment_loc = strstr (l, "@c");
    }
    
    /* Ignore anything after a % or @c.  */
    if (comment_loc)
      *comment_loc = 0;

    map_lineno++;

    /* Skip leading whitespace so we can use strlen below.  Can't use
       strtok since this routine is recursive.  */
    while (*l && ISSPACE (*l))
      l++;
      
    /* If we don't have any filename, that's ok, the line is blank.  */
    filename = token (l);
    if (filename) {
      string alias = token (l + strlen (filename));

      if (STREQ (filename, "include")) {
        if (alias == NULL) {
          WARNING2 ("%s:%u: Filename argument for include directive missing",
                    map_filename, map_lineno);
        } else {
          string include_fname = kpse_path_search (map_path, alias, false);
          if (include_fname) {
            map_file_parse (include_fname);
            if (include_fname != alias)
              free (include_fname);
          } else {
            WARNING3 ("%s:%u: Can't find fontname include file `%s'",
                      map_filename, map_lineno, alias);
          }
          free (alias);
          free (filename);
        }

      /* But if we have a filename and no alias, something's wrong.  */
      } else if (alias == NULL) {
        WARNING3 ("%s:%u: Fontname alias missing for filename `%s'",
                  map_filename, map_lineno, filename);
        free (filename);

      } else {
        /* We've got everything.  Insert the new entry.  They were
           already dynamically allocated, so don't bother with xstrdup.  */
        hash_insert (&map, alias, filename);
      }
    }

    free (l);
  }
  
  xfclose (f, map_filename);
}

/* Parse the file MAP_NAME in each of the directories in PATH and
   return the resulting structure.  Entries in earlier files override
   later files.  */

static void
read_all_maps P1H(void)
{
  string *filenames;
  
  map_path = kpse_init_format (kpse_fontmap_format);
  filenames = kpse_all_path_search (map_path, MAP_NAME);
  
  map = hash_create (MAP_HASH_SIZE);

  while (*filenames) {
    map_file_parse (*filenames);
    filenames++;
  }
}

/* Look up KEY in texfonts.map's; if it's not found, remove any suffix
   from KEY and try again.  Create the map if necessary.  */

string *
kpse_fontmap_lookup P1C(const_string, key)
{
  string *ret;
  string suffix = find_suffix (key);
  
  if (map.size == 0) {
    read_all_maps ();
  }

  ret = hash_lookup (map, key);
  if (!ret) {
    /* OK, the original KEY didn't work.  Let's check for the KEY without
       an extension -- perhaps they gave foobar.tfm, but the mapping only
       defines `foobar'.  */
    if (suffix) {
      string base_key = remove_suffix (key);
      ret = hash_lookup (map, base_key);
      free (base_key);
    }
  }

  /* Append any original suffix.  */
  if (ret && suffix) {
    string *elt;
    for (elt = ret; *elt; elt++) {
      *elt = extend_filename (*elt, suffix);
    }
  }

  return ret;
}
