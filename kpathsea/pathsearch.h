/* pathsearch.h: generic path searching.

Copyright (C) 1993, 94 Karl Berry.

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

#ifndef KPATHSEA_PATHSEARCH_H
#define KPATHSEA_PATHSEARCH_H

#include <kpathsea/c-proto.h>
#include <kpathsea/str-llist.h>
#include <kpathsea/types.h>

/* If PATH is non-null, return its first element (as defined by
   IS_ENV_SEP).  If it's NULL, return the next element in the previous
   path, a la strtok.  Leading, trailing, or doubled colons result in
   the empty string.  When at the end of PATH, return NULL.  In any
   case, return a pointer to an area that may be overwritten on
   subsequent calls.  */
extern string kpse_path_element P1H(const_string path);


/* Given a path element ELT, return a pointer to a NULL-terminated list
   of the corresponding (existing) directory or directories, with
   trailing slashes, or NULL.  If ELT is the empty string, check the
   current working directory.
   
   It's up to the caller to expand ELT.  This is because this routine is
   most likely only useful to be called from `kpse_path_search', which
   has already assumed expansion has been done.  */
extern str_llist_type *kpse_element_dirs P1H(const_string elt);


/* Call `kpse_expand' on NAME.  If the result is an absolute or
   explicitly relative filename, check whether it is a readable
   (regular) file.
   
   Otherwise, look in each of the directories specified in PATH (also do
   tilde and variable expansion on elements in PATH), using a prebuilt
   db (see db.h) if it's relevant for a given path element.
   
   If the prebuilt db doesn't exist, or if MUST_EXIST is true and NAME
   isn't found in the prebuilt db, look on the filesystem.  (I.e., if
   MUST_EXIST is false, and NAME isn't found in the db, do *not* look on
   the filesystem.)
   
   The caller must expand PATH. This is because it makes more sense to
   do this once, in advance, instead of for every search using it.
   
   In any case, return the complete filename if found, otherwise NULL.  */
extern string kpse_path_search P3H(const_string path, const_string name,
                                   boolean must_exist);


/* Like `kpse_path_search' with MUST_EXIST true, but return a list of
   all the filenames (or NULL if none), instead of taking the first.  */
extern string *kpse_all_path_search P2H(const_string path, const_string name);

#endif /* not KPATHSEA_PATHSEARCH_H */
