/* str-llist.c: Implementation of a linked list of strings.

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

#include <kpathsea/str-llist.h>


/* Add the new string STR to the end of the list L.  */

void
str_llist_add P2C(str_llist_type *, l,  string, str)
{
  str_llist_elt_type *e;
  str_llist_elt_type *new_elt = XTALLOC1 (str_llist_elt_type);
  
  /* The new element will be at the end of the list.  */
  STR_LLIST (*new_elt) = str;
  STR_LLIST_MOVED (*new_elt) = false;
  STR_LLIST_NEXT (*new_elt) = NULL;
  
  /* Find the current end of the list.  */
  for (e = *l; e && STR_LLIST_NEXT (*e); e = STR_LLIST_NEXT (*e))
    ;
  
  if (!e)
    *l = new_elt;
  else
    STR_LLIST_NEXT (*e) = new_elt;
}

/* Move an element towards the top. The idea is that when a file is
   found in a given directory, later files will likely be in that same
   directory, and looking for the file in all the directories in between
   is thus a waste.  */

void
str_llist_float P2C(str_llist_type *, l,  str_llist_elt_type *, mover)
{
  str_llist_elt_type *last_moved, *unmoved;
  
  /* If we've already moved this element, never mind.  */
  if (STR_LLIST_MOVED (*mover))
    return;
  
  /* Find the first unmoved element (to insert before).  We're
     guaranteed this will terminate, since MOVER itself is currently
     unmoved, and it must be in L (by hypothesis).  */
  for (last_moved = NULL, unmoved = *l; STR_LLIST_MOVED (*unmoved);
       last_moved = unmoved, unmoved = STR_LLIST_NEXT (*unmoved))
    ;

  /* If we are the first unmoved element, nothing to relink.  */
  if (unmoved != mover)
    { /* Remember `mover's current successor, so we can relink `mover's
         predecessor to it.  */
      str_llist_elt_type *before_mover;
      str_llist_elt_type *after_mover = STR_LLIST_NEXT (*mover);
      
      /* Find `mover's predecessor.  */
      for (before_mover = unmoved; STR_LLIST_NEXT (*before_mover) != mover;
           before_mover = STR_LLIST_NEXT (*before_mover))
        ;
      
      /* `before_mover' now links to `after_mover'.  */
      STR_LLIST_NEXT (*before_mover) = after_mover;

      /* Insert `mover' before `unmoved' and after `last_moved' (or at
         the head of the list).  */
      STR_LLIST_NEXT (*mover) = unmoved;
      if (!last_moved)
        *l = mover;
      else
        STR_LLIST_NEXT (*last_moved) = mover;
    }

  /* We've moved it.  */
  STR_LLIST_MOVED (*mover) = true;
}
