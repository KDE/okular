/* hash.c: hash table operations.

Copyright (C) 1994 Karl Berry.

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

#include <kpathsea/hash.h>
#include <kpathsea/str-list.h>


/* The hash function.  We go for simplicity here.  */

static unsigned
hash P2C(hash_table_type, table,  const_string, key)
{
  unsigned n = 0;
  
  /* Our keys aren't often anagrams of each other, so no point in
     weighting the characters.  */
  while (*key != 0)
    n = (n + n + *key++) % table.size;
  
  return n;
}

hash_table_type
hash_create P1C(unsigned, size) 
{
  unsigned b;
  hash_table_type ret;
  ret.buckets = XTALLOC (size, hash_element_type *);
  ret.size = size;
  
  /* calloc's zeroes aren't necessarily NULL, according to ANSI, so be
     safe.  (Not that I know of any exceptions in reality.)  */
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

/* Look up STR in MAP.  Return a (dynamically-allocated) list of the
   corresponding strings or NULL if no match.  */ 

#ifdef DEBUG
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
    if (STREQ (key, p->key))
      /* Cast because the general str_list_type shouldn't force const data.  */
      str_list_add (&ret, (string) p->value);
  
  /* If we found anything, mark end of list with null.  */
  if (STR_LIST (ret))
    str_list_add (&ret, NULL);

#ifdef DEBUG
  if (KPSE_DEBUG_P (KPSE_DEBUG_HASH))
    {
      DEBUGF1 ("hash_lookup(%s) =>", key);
      if (!STR_LIST (ret))
        fputs (" (null)\n", stderr);
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
hash_print P1C(hash_table_type, table)
{
  unsigned b;
  unsigned total_elements = 0, total_buckets = 0;
  
  for (b = 0; b < table.size; b++)
    {
      hash_element_type *bucket = table.buckets[b];
      
      if (bucket)
        {
          unsigned len = 1;
          hash_element_type *tb;
          
          total_buckets++;
          printf ("%4d ", b);
          
          for (tb = bucket->next; tb != NULL; tb = tb->next)
            len++;
          printf (":%-5d", len);
          total_elements += len;
          
          for (tb = bucket; tb != NULL; tb = tb->next)
            printf (" %s=>%s", tb->key, tb->value);
          
          putchar ('\n');
        }
    }
  
  printf ("%u buckets, %u nonempty (%u%%); %u entries, average chain %.1f.\n",
          table.size, total_buckets, 100 * total_buckets / table.size,
          total_elements,
          total_buckets ? total_elements / (double) total_buckets : 0.0);
}
