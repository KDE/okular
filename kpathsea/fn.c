/* fn.c: arbitrarily long filenames (or just strings).

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

#include <kpathsea/fn.h>


/* /usr/local/lib/texmf/fonts/public/cm/pk/ljfour/cmr10.300pk is 58
   chars, so ASCII `K' seems a good choice. */
#define CHUNK_SIZE 75


fn_type
fn_init P1H(void)
{
  fn_type ret;
  
  FN_ALLOCATED (ret) = FN_LENGTH (ret) = 0;
  FN_STRING (ret) = NULL;
  
  return ret;
}


fn_type
fn_copy0 P2C(const_string, s,  unsigned, len)
{
  fn_type ret;
  
  FN_ALLOCATED (ret) = CHUNK_SIZE > len ? CHUNK_SIZE : len + 1;
  FN_STRING (ret) = xmalloc (FN_ALLOCATED (ret));
  
  strncpy (FN_STRING (ret), s, len);
  FN_STRING (ret)[len] = 0;
  FN_LENGTH (ret) = len + 1;
  
  return ret;
}

/* Don't think we ever try to free something that might usefully be
   empty, so give fatal error if nothing allocated.  */

void
fn_free P1C(fn_type *, f)
{
  assert (FN_STRING (*f) != NULL);
  free (FN_STRING (*f));
  FN_STRING (*f) = NULL;
  FN_ALLOCATED (*f) = 0;
  FN_LENGTH (*f) = 0;
}

/* An arithmetic increase seems more reasonable than geometric.  We
   don't increase the length member since it may be more convenient for
   the caller to add than subtract when appending the stuff that will
   presumably follow.  */

static void
grow P2C(fn_type *, f,  unsigned, len)
{
  while (FN_LENGTH (*f) + len > FN_ALLOCATED (*f))
    {
      FN_ALLOCATED (*f) += CHUNK_SIZE;
      XRETALLOC (FN_STRING (*f), FN_ALLOCATED (*f), char);
    }
}


void
fn_1grow P2C(fn_type *, f,  char, c)
{
  grow (f, 1);
  FN_STRING (*f)[FN_LENGTH (*f)] = c;
  FN_LENGTH (*f)++;
}


void
fn_grow P3C(fn_type *, f,  address, source,  unsigned, len)
{
  grow (f, len);
  strncpy (FN_STRING (*f) + FN_LENGTH (*f), source, len);
  FN_LENGTH (*f) += len;
}


void
fn_str_grow P2C(fn_type *, f,  const_string, s)
{
  unsigned more_len = strlen (s);
  grow (f, more_len);
  strcat (FN_STRING (*f), s);
  FN_LENGTH (*f) += more_len;
}


void
fn_shrink_to P2C(fn_type *, f,  unsigned, loc)
{
  assert (FN_LENGTH (*f) > loc);
  FN_STRING (*f)[loc] = 0;
  FN_LENGTH (*f) = loc + 1;
}
