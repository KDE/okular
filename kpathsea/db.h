/* db.h: lookups in an externally built db file.

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

#ifndef KPATHSEA_DB_H
#define KPATHSEA_DB_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>
#include <kpathsea/str-list.h>

/* It's not an error if this doesn't exist; we just go ahead and search
   the actual directories.  See the `Filename database' node in the
   kpathsea documentation for details. The variable reference here is
   expanded by kpathsea. This lets you have different databases for
   different TeX hierarchies (only one at a time, though!) without
   having to change anything.  */
#ifndef KPSE_DB_DIR
#define KPSE_DB_DIR "$TEXMF"
#endif
#ifndef KPSE_DB_NAME
#define KPSE_DB_NAME "ls-R"
#endif

/* The expansion of DB_DIR; set by `read_files' in cnf.c, used by
   `elt_in_db' in pathsearch.c.  */
extern string kpse_db_dir;

/* Return list of matches for NAME in the ls-R file matching PATH.  If
   ALL is set, return (null-terminated list) of all matches, else just
   the first.  If no matches, return a pointer to an empty list.  If the
   database can't be read, returns NULL.  */
extern str_list_type *kpse_db_search P3H(const_string name, 
                                         const_string path, boolean all);

/* Insert the filename FNAME into the database.  */
extern void db_insert P1H(const_string fname);

#endif /* not KPATHSEA_DB_H */
