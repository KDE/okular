/* dir.c: directory operations.

Copyright (C) 1992, 93, 94, 95 Free Software Foundation, Inc.

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

#include <kpathsea/c-dir.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/hash.h>


/* Return true if FN is a directory or a symlink to a directory,
   false if not. */

boolean
dir_p P1C(const_string, fn)
{
#ifdef WIN32
  int fa = GetFileAttributes(fn);
  return (fa != 0xFFFFFFFF && (fa & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat stats;
  return stat (fn, &stats) == 0 && S_ISDIR (stats.st_mode);
#endif
}

#ifndef WIN32

/* Return -1 if FN isn't a directory, else its number of links.
   Duplicate the call to stat; no need to incur overhead of a function
   call for that little bit of cleanliness. */

int
dir_links P1C(const_string, fn)
{
  static hash_table_type link_table;
  string *hash_ret;
  long ret;
  
  if (link_table.size == 0)
    link_table = hash_create (457);

#ifdef KPSE_DEBUG
  /* This is annoying, but since we're storing integers as pointers, we
     can't print them as strings.  */
  if (KPSE_DEBUG_P (KPSE_DEBUG_HASH))
    kpse_debug_hash_lookup_int = true;
#endif

  hash_ret = hash_lookup (link_table, fn);
  
#ifdef KPSE_DEBUG
  if (KPSE_DEBUG_P (KPSE_DEBUG_HASH))
    kpse_debug_hash_lookup_int = false;
#endif

  /* Have to cast the int we need to/from the const_string that the hash
     table stores for values. Let's hope an int fits in a pointer.  */
  if (hash_ret)
    ret = (long) *hash_ret;
  else
    {
      struct stat stats;
      ret = stat (fn, &stats) == 0 && S_ISDIR (stats.st_mode)
            ? stats.st_nlink : -1;

      /* It's up to us to copy the value.  */
      hash_insert (&link_table, xstrdup (fn), (const_string) ret);
      
#ifdef KPSE_DEBUG
      if (KPSE_DEBUG_P (KPSE_DEBUG_STAT))
        DEBUGF2 ("dir_links(%s) => %ld\n", fn, ret);
#endif
    }

  return ret;
}

#endif /* !WIN32 */
