/* line.c: return the next line from a file, or NULL.

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
extern void free ();

/* From xmalloc.c and xrealloc.c.  This saves having to include config.h.  */
extern void *xmalloc (), *xrealloc ();


/* Allocate in increments of this size.  */
#define BLOCK_SIZE 40

char *
read_line (f)
    FILE *f;
{
  int c;
  unsigned limit = BLOCK_SIZE;
  unsigned loc = 0;
  char *line = xmalloc (limit);
  
  while ((c = getc (f)) != EOF && c != '\n')
    {
      line[loc] = c;
      loc++;
      
      /* By testing after the assignment, we guarantee that we'll always
         have space for the null we append below.  We know we always
         have room for the first char, since we start with BLOCK_SIZE.  */
      if (loc == limit)
        {
          limit += BLOCK_SIZE;
          line = xrealloc (line, limit);
        }
    }
  
  /* If we read anything, return it.  This can't represent a last
     ``line'' which doesn't end in a newline, but so what.  */
  if (c != EOF)
    {
      /* Terminate the string.  We can't represent nulls in the file,
         either.  Again, it doesn't matter.  */
      line[loc] = 0;
    }
  else /* At end of file.  */
    {
      free (line);
      line = NULL;
    }

  return line;
}
