/* line.c: return the next line from a file, or NULL.

Copyright (C) 1992, 93, 95, 96 Free Software Foundation, Inc.

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
#include <kpathsea/line.h>

/* Allocate in increments of this size.  */
#define BLOCK_SIZE 75

char *
read_line (f)
    FILE *f;
{
  int c;
  unsigned limit = BLOCK_SIZE;
  unsigned loc = 0;
  char *line = xmalloc (limit);
  
  while ((c = getc (f)) != EOF && c != '\n' && c != '\r')
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
      /* Absorb LF of a CRLF pair. */
      if (c == '\r') {
          c = getc (f);
          if (c != '\n')
              ungetc (c, f);
      }
    }
  else /* At end of file.  */
    {
      free (line);
      line = NULL;
    }

  return line;
}

