/* xrealloc.c: realloc with error checking.

Copyright (C) 1992, 93 Free Software Foundation, Inc.

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

/* Don't include config.h or all our other usual includes, since
   it's useful to just throw this file into other programs.  */

#include <stdio.h>
extern char *realloc ();


extern void *xmalloc ();

void *
xrealloc (old_ptr, size)
    void *old_ptr;
    unsigned size;
{
  void *new_mem;

  if (old_ptr == NULL)
    new_mem = xmalloc (size);
  else
    {
      new_mem = (void *) realloc (old_ptr, size);
      if (new_mem == NULL)
        {
          /* We used to print OLD_PTR here using %x, and casting its
             value to unsigned, but that lost on the Alpha, where
             pointers and unsigned had different sizes.  Since the info
             is of little or no value anyway, just don't print it.  */
          fprintf (stderr, "fatal: memory exhausted (realloc of %u bytes).\n",
                   size);
          /* 1 means success on VMS, so pick a random number (ASCII `B').  */
          exit (66);
        }
    }

  return new_mem;
}
