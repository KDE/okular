/* hash.h: declarations for a hash table.

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

#ifndef HASH_H
#define HASH_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* A single (key,value) pair.  */
typedef struct hash_element_struct
{
  const_string key;
  const_string value;
  struct hash_element_struct *next;
} hash_element_type;

/* The usual arrangement of buckets initialized to null.  */
typedef struct
{
  hash_element_type **buckets;
  unsigned size;
} hash_table_type;

#ifdef DEBUG
/* How to print the hash results when debugging.  */
extern boolean kpse_debug_hash_lookup_int;
#endif

/* Create a hash table of size SIZE.  */
extern hash_table_type hash_create P1H(unsigned size);

/* Insert the (KEY,VALUE) association into TABLE.  KEY may have more
   than one VALUE.  Neither KEY nor VALUE is copied.  */
extern void hash_insert P3H(hash_table_type *table,  const_string key,
                            const_string value);

/* Look up KEY in MAP, and return NULL-terminated list of all matching
   values (not copies), in insertion order.  If none, return NULL.  */
extern string *hash_lookup P2H(hash_table_type table, const_string key);

/* Print TABLE to stdout.  */
extern void hash_print P1H(hash_table_type table);

#endif /* not HASH_H */
