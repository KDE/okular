/* str-list.h: Declarations for string lists.

Copyright (C) 1993, 94 Karl Berry.

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

#ifndef KPATHSEA_STR_LIST_H
#define KPATHSEA_STR_LIST_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* Lists of strings; used for, e.g., directory lists.  */

typedef struct
{
  unsigned length;
  string *list;
} str_list_type;

#define STR_LIST_LENGTH(l) ((l).length)
#define STR_LIST(l) ((l).list)
#define STR_LIST_ELT(l, n) STR_LIST (l)[n]
#define STR_LIST_LAST_ELT(l) STR_LIST_ELT (l, STR_LIST_LENGTH (l) - 1)

/* Return a new, empty, list.  */
extern str_list_type str_list_init P1H(void);

/* Append the string S to the list L.  It's up to the caller to not
   deallocate S; we don't copy it.  Also up to the caller to terminate
   the list with a null entry.  */
extern void str_list_add P2H(str_list_type *l, string s);

/* Append all the elements from MORE to TARGET.  */
extern void str_list_concat P2H(str_list_type * target, str_list_type more);

/* Free the space for the list elements (but not the list elements
   themselves).  */
extern void str_list_free P1H(str_list_type *l);

/* Append each element of more to each element of target. */
extern void str_list_concat_elements P2H(str_list_type * target, str_list_type more);

#endif /* not KPATHSEA_STR_LIST_H */
