/* path-elt.c: Return the stuff between colons.

Copyright (C) 1993, 96 Karl Berry.

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

#include <kpathsea/c-pathch.h>
#include <kpathsea/pathsearch.h>


/* The static (but dynamically allocated) area we return the answer in,
   and how much we've currently allocated for it.  */
static string elt = NULL;
static unsigned elt_alloc = 0;

/* The path we're currently working on.  */
static const_string path = NULL;


/* Upon entry, the static `path' is at the first (and perhaps last)
   character of the return value, or else NULL if we're at the end (or
   haven't been called).  I make no provision for caching the results;
   thus, we parse the same path over and over, on every lookup.  If that
   turns out to be a significant lose, it can be fixed, but I'm guessing
   disk accesses overwhelm everything else.  If ENV_P is true, use
   IS_ENV_SEP; else use IS_DIR_SEP.  */

static string
element P2C(const_string, passed_path,  boolean, env_p)
{
  const_string p;
  string ret;
  int brace_level;
  unsigned len;
  
  if (passed_path)
    path = passed_path;
  /* Check if called with NULL, and no previous path (perhaps we reached
     the end).  */
  else if (!path)
    return NULL;
  
  /* OK, we have a non-null `path' if we get here.  */
  assert (path);
  p = path;
  
  /* Find the next colon not enclosed by braces (or the end of the path).  */
  brace_level = 0;
  while (*p != 0  && !(brace_level == 0
                       && (env_p ? IS_ENV_SEP (*p) : IS_DIR_SEP (*p)))) {
    if (*p == '{') ++brace_level;
    else if (*p == '}') --brace_level;
    ++p;
  }
   
  /* Return the substring starting at `path'.  */
  len = p - path;

  /* Make sure we have enough space (including the null byte).  */
  if (len + 1 > elt_alloc)
    {
      elt_alloc = len + 1;
      elt = xrealloc (elt, elt_alloc);
    }

  strncpy (elt, path, len);
  elt[len] = 0;
  ret = elt;

  /* If we are at the end, return NULL next time.  */
  if (path[len] == 0)
    path = NULL;
  else
    path += len + 1;

  return ret;
}

string
kpse_path_element P1C(const_string, p)
{
  return element (p, true);
}

string
kpse_filename_component P1C(const_string, p)
{
  return element (p, false);
}

#ifdef TEST

void
print_path_elements (const_string path)
{
  string elt;
  printf ("Elements of `%s':", path ? path : "(null)");
  
  for (elt = kpse_path_element (path); elt != NULL;
       elt = kpse_path_element (NULL))
    {
      printf (" %s", *elt ? elt : "`'");
    }
  
  puts (".");
}

int
main ()
{
  /* All lists end with NULL.  */
  print_path_elements (NULL);	/* */
  print_path_elements ("");	/* "" */
  print_path_elements ("a");	/* a */
  print_path_elements (ENV_SEP_STRING);	/* "", "" */
  print_path_elements (ENV_SEP_STRING ENV_SEP_STRING);	/* "", "", "" */
  print_path_elements ("a" ENV_SEP_STRING);	/* a, "" */ 
  print_path_elements (ENV_SEP_STRING "b");	/* "", b */ 
  print_path_elements ("a" ENV_SEP_STRING "b");	/* a, b */ 
  
  return 0;
}

#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST path-elt.c kpathsea.a"
End:
*/
