/* hash.c: hash table operations.

Copyright (C) 1994, 95, 96, 97 Karl Berry & Olaf Weber.

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
#include <kpathsea/c-ctype.h>

#include <kpathsea/hash.h>
#include <kpathsea/str-list.h>


/* The hash function.  We go for simplicity here.  */

/* All our hash tables are related to filenames.  */
#ifdef MONOCASE_FILENAMES
#ifdef WIN32
/* This is way faster under Win32. */
#define TRANSFORM(x) ((unsigned)CharUpper((char*)(x)))
#else
#define TRANSFORM(x) (toupper(x))
#endif
#else
#define TRANSFORM(x) (x)
#endif

static unsigned
hash P2C(hash_table_type, table,  const_string, key)
{
  unsigned n = 0;
  
  /* Our keys aren't often anagrams of each other, so no point in
     weighting the characters.  */
  while (*key != 0)
    n = (n + n + TRANSFORM (*key++)) % table.size;
  
  return n;
}

hash_table_type
hash_create P1C(unsigned, size) 
{
  /* hash_table_type ret; changed into "static ..." to work around gcc
     optimizer bug for Alpha.  */
  static hash_table_type ret;
  unsigned b;
  ret.buckets = XTALLOC (size, hash_element_type *);
  ret.size = size;
  
  /* calloc's zeroes aren't necessarily NULL, so be safe.  */
  for (b = 0; b <ret.size; b++)
    ret.buckets[b] = NULL;
    
  return ret;
}

/* Whether or not KEY is already in MAP, insert it and VALUE.  Do not
   duplicate the strings, in case they're being purposefully shared.  */

void
hash_insert P3C(hash_table_type *, table,  const_string, key,
                const_string, value)
{
  unsigned n = hash (*table, key);
  hash_element_type *new_elt = XTALLOC1 (hash_element_type);

  new_elt->key = key;
  new_elt->value = value;
  new_elt->next = NULL;
  
  /* Insert the new element at the end of the list.  */
  if (!table->buckets[n])
    /* first element in bucket is a special case.  */
    table->buckets[n] = new_elt;
  else
    {
      hash_element_type *loc = table->buckets[n];
      while (loc->next)		/* Find the last element.  */
        loc = loc->next;
      loc->next = new_elt;	/* Insert the new one after.  */
    }
}

/* Remove a (KEY, VALUE) pair.  */

void
hash_remove P3C(hash_table_type *, table,  const_string, key,
                const_string, value)
{
  hash_element_type *p;
  hash_element_type *q;
  unsigned n = hash (*table, key);

  /* Find pair.  */
  for (q = NULL, p = table->buckets[n]; p != NULL; q = p, p = p->next)
    if (FILESTRCASEEQ (key, p->key) && STREQ (value, p->value))
      break;
  if (p) {
    /* We found something, remove it from the chain.  */
    if (q) q->next = p->next; else table->buckets[n] = p->next;
    /* We cannot dispose of the contents.  */
    free (p);
  }
}

/* Look up STR in MAP.  Return a (dynamically-allocated) list of the
   corresponding strings or NULL if no match.  */ 

#ifdef KPSE_DEBUG
/* Print the hash values as integers if this is nonzero.  */
boolean kpse_debug_hash_lookup_int = false; 
#endif

string *
hash_lookup P2C(hash_table_type, table,  const_string, key)
{
  hash_element_type *p;
  str_list_type ret;
  unsigned n = hash (table, key);
  ret = str_list_init ();
  
  /* Look at everything in this bucket.  */
  for (p = table.buckets[n]; p != NULL; p = p->next)
    if (FILESTRCASEEQ (key, p->key))
      /* Cast because the general str_list_type shouldn't force const data.  */
      str_list_add (&ret, (string) p->value);
  
  /* If we found anything, mark end of list with null.  */
  if (STR_LIST (ret))
    str_list_add (&ret, NULL);

#ifdef KPSE_DEBUG
  if (KPSE_DEBUG_P (KPSE_DEBUG_HASH))
    {
      DEBUGF1 ("hash_lookup(%s) =>", key);
      if (!STR_LIST (ret))
        fputs (" (nil)\n", stderr);
      else
        {
          string *r;
          for (r = STR_LIST (ret); *r; r++)
            {
              putc (' ', stderr);
              if (kpse_debug_hash_lookup_int)
                fprintf (stderr, "%ld", (long) *r);
              else
                fputs (*r, stderr);
            }
          putc ('\n', stderr);
        }
      fflush (stderr);
    }
#endif

  return STR_LIST (ret);
}

/* We only print nonempty buckets, to decrease output volume.  */

void
hash_print P2C(hash_table_type, table,  boolean, summary_only)
{
  unsigned b;
  unsigned total_elements = 0, total_buckets = 0;
  
  for (b = 0; b < table.size; b++) {
    hash_element_type *bucket = table.buckets[b];

    if (bucket) {
      unsigned len = 1;
      hash_element_type *tb;

      total_buckets++;
      if (!summary_only) fprintf (stderr, "%4d ", b);

      for (tb = bucket->next; tb != NULL; tb = tb->next)
        len++;
      if (!summary_only) fprintf (stderr, ":%-5d", len);
      total_elements += len;

      if (!summary_only) {
        for (tb = bucket; tb != NULL; tb = tb->next)
          fprintf (stderr, " %s=>%s", tb->key, tb->value);
        putc ('\n', stderr);
      }
    }
  }
  
  fprintf (stderr,
          "%u buckets, %u nonempty (%u%%); %u entries, average chain %.1f.\n",
          table.size,
          total_buckets,
          100 * total_buckets / table.size,
          total_elements,
          total_buckets ? total_elements / (double) total_buckets : 0.0);
}
