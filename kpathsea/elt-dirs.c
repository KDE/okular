/* elt-dirs.c: Translate a path element to its corresponding director{y,ies}.

Copyright (C) 1993, 94 Karl Berry.

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

#include <kpathsea/c-pathch.h>
#include <kpathsea/expand.h>
#include <kpathsea/fn.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/xopendir.h>

/* To avoid giving prototypes for all the routines and then their real
   definitions, we give all the subroutines first.  The entry point is
   the last routine in the file.  */

/* Make a copy of DIR (unless it's null) and save it in L.  Ensure that
   DIR ends with a DIR_SEP for the benefit of later searches.  */

static void
dir_list_add P2C(str_llist_type *, l,  const_string, dir)
{
  string saved_dir
    = IS_DIR_SEP (dir[strlen (dir) - 1])
      ? xstrdup (dir)
      : concat (dir, DIR_SEP_STRING);
  
  str_llist_add (l, saved_dir);
}


/* If DIR is a directory, add it to the list L.  */

static void
checked_dir_list_add P2C(str_llist_type *, l,  const_string, dir)
{
  if (dir_p (dir))
    dir_list_add (l, dir);
}

/* The cache.  Typically, several paths have the same element; for
   example, /usr/local/lib/texmf/fonts//.  We don't want to compute the
   expansion of such a thing more than once.  Even though we also cache
   the dir_links call, that's not enough -- without this path element
   caching as well, the execution time doubles.  */

typedef struct
{
  const_string key;
  str_llist_type *value;
} cache_entry;

static cache_entry *the_cache = NULL;
static unsigned cache_length = 0;


/* Associate KEY with VALUE.  We implement the cache as a simple linear
   list, since it's unlikely to ever be more than a dozen or so elements
   long.  We don't bother to check here if PATH has already been saved;
   we always add it to our list.  We copy KEY but not VALUE; not sure
   that's right, but it seems to be all that's needed.  */

static void
cache P2C(const_string, key,  str_llist_type *, value)
{
  cache_length++;
  XRETALLOC (the_cache, cache_length, cache_entry);
  the_cache[cache_length - 1].key = xstrdup (key);
  the_cache[cache_length - 1].value = value;
}


/* To retrieve, just check the list in order.  */

static str_llist_type *
cached P1C(const_string, key)
{
  unsigned p;
  
  for (p = 0; p < cache_length; p++)
    {
      if (STREQ (the_cache[p].key, key))
        return the_cache[p].value;
    }
  
  return NULL;
}

/* Handle the magic path constructs.  */

/* Declare recursively called routine.  */
static void expand_elt P3H(str_llist_type *, const_string, unsigned);


/* POST is a pointer into the original element (which may no longer be
   ELT) to just after the doubled DIR_SEP, perhaps to the null.  Append
   subdirectories of ELT (up to ELT_LENGTH, which must be a /) to
   STR_LIST_PTR.  */

static void
do_subdir P4C(str_llist_type *, str_list_ptr,  const_string, elt,
              unsigned, elt_length,  const_string, post)
{
  DIR *dir;
  struct dirent *e;
  fn_type name;
  
  /* Some old compilers don't allow aggregate initialization.  */
  name = fn_copy0 (elt, elt_length);
  
  assert (IS_DIR_SEP (elt[elt_length - 1]));
  
  /* If we can't open it, quit.  */
  dir = opendir (FN_STRING (name));
  if (dir == NULL)
    {
      fn_free (&name);
      return;
    }
  
  /* Include top level before subdirectories, if nothing to match.  */
  if (*post == 0)
    dir_list_add (str_list_ptr, FN_STRING (name));
  else
    { /* If we do have something to match, see if it exists.  For
         example, POST might be `pk/ljfour', and they might have a
         directory `$TEXMF/fonts/pk/ljfour' that we should find.  */
      fn_str_grow (&name, post);
      if (dir_p (FN_STRING (name)))
        dir_list_add (str_list_ptr, FN_STRING (name));
      fn_shrink_to (&name, elt_length);
    }

  while ((e = readdir (dir)) != NULL)
    { /* If it begins with a `.', never mind.  (This allows ``hidden''
         directories that the algorithm won't find.)  */
      if (e->d_name[0] != '.')
        {
          int links;
          
          /* Construct the potential subdirectory name.  */
          fn_str_grow (&name, e->d_name);
          
          /* If we can't stat it, or if it isn't a directory, continue.  */
          links = dir_links (FN_STRING (name));

          if (links >= 0)
            { 
              unsigned potential_len = FN_LENGTH (name);
              
              /* It's a directory, so append the separator.  */
              fn_str_grow (&name, DIR_SEP_STRING);
              
              if (*post != 0)
                { 
                  fn_str_grow (&name, post);
                  /* Unfortunately we can't check if the new element is
                     a leaf directory, because we don't have a directory
                     name here, we just have a path spec. This means we
                     may descend into a leaf directory cm/pk, if the
                     spec is ...fonts//pk//.  */
                  expand_elt (str_list_ptr, FN_STRING (name), potential_len);
                  fn_shrink_to (&name, potential_len);
                }
              
              /* Should we recurse?  To see if the subdirectory is a
                 leaf, check if it has two links (one for . and one for
                 ..).  This means that symbolic links to directories do
                 not affect the leaf-ness.  This is arguably wrong, but
                 the only alternative I know of is to stat every entry
                 in the directory, and that is unacceptably slow.
                 
                 The #ifdef here makes all this configurable at
                 compile-time, so that if we're using VMS directories or
                 some such, we can still find subdirectories, even if it
                 is much slower.  */
#ifdef UNIX_ST_NLINK
              if (links > 2)
#endif
                /* All criteria are met; find subdirectories.  */
                do_subdir (str_list_ptr, FN_STRING (name),
                           potential_len, post);
#ifdef UNIX_ST_NLINK
              else if (*post == 0)
                /* Nothing to match, no recursive subdirectories to
                   look for: we're done with this branch.  Add it.  */
                dir_list_add (str_list_ptr, FN_STRING (name));
#endif
            }

          /* Remove the directory entry we just checked from `name'.  */
          fn_shrink_to (&name, elt_length);
        }
    }
  
  fn_free (&name);
  xclosedir (dir);
}


/* Assume ELT is non-empty and non-NULL.  Return list of corresponding
   directories (with no terminating NULL entry) in STR_LIST_PTR.  Start
   looking for magic constructs at START.  */

static void
expand_elt P3C(str_llist_type *, str_list_ptr,  const_string, elt,
               unsigned, start)
{
  boolean found_special = false;
  const_string dir = elt + start;
  
  while (*dir != 0)
    {
      if (IS_DIR_SEP (*dir))
        {
          /* If two consecutive directory separators, find subdirectories.  */
          if (IS_DIR_SEP (dir[1]))
            {
              do_subdir (str_list_ptr, elt, dir - elt + 1, dir + 2);
              found_special = true;
            }
#if 0
/* Maybe eventually I'll implement this, but probably not.  */
          /* If /?, make following component optional.  */
          else if (dir[1] == '?')
            do_optional (str_list_ptr, elt, dir - elt + 1, dir + 2);
#endif
          /* No special stuff at this slash.  Keep going.  */
        }
      
      dir++;
    }
  
  if (!found_special)
    /* When we reach the end of ELT, it will be a normal filename.  */
    checked_dir_list_add (str_list_ptr, elt);
}

/* Here is the entry point.  Returns directory list for ELT.  */

str_llist_type *
kpse_element_dirs P1C(const_string, elt)
{
  str_llist_type *ret;

  /* If given nothing, return nothing.  */
  if (!elt)
    return NULL;

  /* If we've already cached the answer for ELT, return it.  */
  ret = cached (elt);
  if (ret)
    return ret;

  /* We're going to have a real directory list to return.  */
  ret = XTALLOC1 (str_llist_type);
  *ret = NULL;
  
  /* If ELT is the empty string, just return cwd.  */
  if (*elt == 0)
    { /* Some old compilers do not support aggregate initialization.  */
      char cwd[3];
      cwd[0] = '.';
      cwd[1] = DIR_SEP;
      cwd[2] = 0;
      
      checked_dir_list_add (ret, cwd);
    }

  /* OK, so much for the trivial cases.  We handle the hard case in
     a subroutine.  */
  else
    expand_elt (ret, elt, 0);

  /* Remember the directory list we just found, in case future calls are
     made with the same ELT.  */
  cache (elt, ret);

#ifdef DEBUG
  if (KPSE_DEBUG_P (KPSE_DEBUG_EXPAND))
    {
      DEBUGF1 ("path element %s =>", elt);
      if (ret)
        {
          str_llist_elt_type *e;
          for (e = *ret; e; e = STR_LLIST_NEXT (*e))
            fprintf (stderr, " %s", STR_LLIST (*e));
        }
      putc ('\n', stderr);
      fflush (stderr);
    }
#endif

  return ret;
}

#ifdef TEST

void
print_element_dirs (const_string elt)
{
  str_llist_type *dirs;
  
  printf ("Directories of %s:\t", elt ? elt : "(null)");
  fflush (stdout);
  
  dirs = kpse_element_dirs (elt);
  
  if (!dirs)
    printf ("(null)");
  else
    {
      str_llist_elt_type *dir;
      for (dir = *dirs; dir; dir = STR_LLIST_NEXT (*dir))
        {
          string d = STR_LLIST (*dir);
          printf ("%s ", *d ? d : "`'");
        }
    }
  
  putchar ('\n');
}

int
main ()
{
  /* DEBUG_SET (DEBUG_STAT); */

  /* All lists end with NULL.  */
  print_element_dirs (NULL);	/* */
  print_element_dirs ("");	/* ./ */
  print_element_dirs ("/k");	/* */
  print_element_dirs (".//");	/* ./ ./archive/ */
  print_element_dirs (".//archive");	/* ./ ./archive/ */
  print_element_dirs ("/tmp/fonts//");	/* no need to stat anything */
  print_element_dirs ("/usr/local/lib/tex/fonts//");      /* lots */
  print_element_dirs ("/usr/local/lib/tex/fonts//times"); /* just one */
  print_element_dirs ("/usr/local/lib/tex/fonts//"); /* lots again [cache] */
  print_element_dirs ("~karl");		/* tilde expansion */
  print_element_dirs ("$karl");		/* variable expansion */  
  print_element_dirs ("~${LOGNAME}");	/* both */  
  
  return 0;
}

#endif /* TEST */


/*
Local variables:
test-compile-command: "gcc -g -I. -I.. -DTEST elt-dirs.c kpathsea.a"
End:
*/
