/* str-list.c: define routines for string lists.

Copyright (C) 1993 Karl Berry.

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

#include <kpathsea/str-list.h>


/* See the .h file for comments.  */


str_list_type
str_list_init P1H(void)
{
  str_list_type ret;
  
  STR_LIST_LENGTH (ret) = 0;
  STR_LIST (ret) = NULL;
  
  return ret;
}


void
str_list_add P2C(str_list_type *, l,  string, s)
{
  STR_LIST_LENGTH (*l)++;
  XRETALLOC (STR_LIST (*l), STR_LIST_LENGTH (*l), string);
  STR_LIST_LAST_ELT (*l) = s;
}


/* May as well save some reallocations and do everything in a chunk
   instead of calling str_list_add on each element.  */
   
void
str_list_concat P2C(str_list_type *, target,  str_list_type, more)
{
  unsigned e;
  unsigned prev_len = STR_LIST_LENGTH (*target);

  STR_LIST_LENGTH (*target) += STR_LIST_LENGTH (more);
  XRETALLOC (STR_LIST (*target), STR_LIST_LENGTH (*target), string);
  
  for (e = 0; e < STR_LIST_LENGTH (more); e++)
    STR_LIST_ELT (*target, prev_len + e) = STR_LIST_ELT (more, e);
}


/* Free the list (but not the elements within it).  */

void
str_list_free P1C(str_list_type *, l)
{
  if (STR_LIST (*l))
    {
      free (STR_LIST (*l));
      STR_LIST (*l) = NULL;
    }
}
