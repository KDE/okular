/* str-list.c: define routines for string lists.

Copyright (C) 1993 Karl Berry.

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


/* Concatenate the elements of more to each element of target.  This
   _must_ be done with the first index varying fastest. */
/* Note that we free the old elements of target as well. */

void
str_list_concat_elements P2C(str_list_type *, target,  str_list_type, more)
{
    if (STR_LIST_LENGTH(more) == 0) {
        return;
    } else if (STR_LIST_LENGTH(*target) == 0) {
        unsigned int i;
        STR_LIST_LENGTH(*target) = STR_LIST_LENGTH(more);
        STR_LIST(*target) = xmalloc(STR_LIST_LENGTH(more)*sizeof(char*));
        for (i=0;i!=STR_LIST_LENGTH(more);++i) {
            STR_LIST_ELT(*target,i)=xstrdup(STR_LIST_ELT(more,i));
        }
        return;
    } else {
        unsigned new_len;
        char ** new_list;
        unsigned int i,j;
        new_list = xmalloc(STR_LIST_LENGTH (*target)
                           * STR_LIST_LENGTH (more) * sizeof(char*));

        new_len = 0;
        for (j = 0; j != STR_LIST_LENGTH(more); ++j) {
            for (i = 0; i != STR_LIST_LENGTH(*target); ++i) {
                new_list[new_len] = concat(STR_LIST_ELT(*target,i),
                                           STR_LIST_ELT(more,j));
                ++new_len;
            }
        }
        for (i = 0; i != STR_LIST_LENGTH(*target); ++i)
            free(STR_LIST_ELT(*target, i));
        free(STR_LIST(*target));
        STR_LIST_LENGTH(*target) = new_len;
        STR_LIST(*target) = new_list;
    }
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
