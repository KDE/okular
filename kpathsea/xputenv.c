/* xputenv.c: set an environment variable without return.

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

/* Avoid implicit declaration warning.  */
extern int putenv ();

/* This `x' function is different from the others in that it takes
   different parameters than the standard function; but I find it much
   more convenient to pass the variable and the value separately.  Also,
   this way we can guarantee that the environment value won't become
   garbage.  Also, putenv just overwrites old entries with
   the new, and we want to reclaim that space -- this may be called
   hundreds of times on a run.
   
   But naturally, some systems do it differently. In this case, it's
   net2 that is smart and does its own saving/freeing. If you can write
   a test for configure to check this, please send it to me. Until then,
   you'll have to define SMART_PUTENV yourself. */

void
xputenv P2C(const_string, var_name,  const_string, value)
{
  static const_string *saved_env_items = NULL;
  static unsigned saved_len;
  string old_item = NULL;
  string new_item = concat3 (var_name, "=", value);

#ifndef SMART_PUTENV
  /* Check if we have saved anything yet.  */
  if (!saved_env_items)
    {
      saved_env_items = XTALLOC1 (const_string);
      saved_env_items[0] = var_name;
      saved_len = 1;
    }
  else
    {
      /* Check if we've assigned VAR_NAME before.  */
      unsigned i;
      unsigned len = strlen (var_name);
      for (i = 0; i < saved_len && !old_item; i++)
        {
          if (STREQ (saved_env_items[i], var_name))
            {
              old_item = getenv (var_name);
              assert (old_item);
              /* Back up to the `NAME=' in the environment before the
                 value that getenv returns.  */
              old_item -= (len + 1);
            }
        }
      
      if (!old_item)
        {
          /* If we haven't seen VAR_NAME before, save it.  Assume it is
             in safe storage.  */
          saved_len++;
          XRETALLOC (saved_env_items, saved_len, const_string);
          saved_env_items[saved_len - 1] = var_name;
        }
    }
#endif /* not SMART_PUTENV */

  /* As far as I can see there's no way to distinguish between the
     various errors; putenv doesn't have errno values.  */
  if (putenv (new_item) < 0)
    FATAL1 ("putenv (%s) failed", new_item);
  
#ifndef SMART_PUTENV
  /* Can't free `new_item' because its contained value is now in
     `environ', but we can free `old_item', since it's been replaced.  */
  if (old_item)
    free (old_item);
#endif /* not SMART_PUTENV */
}


/* A special case for setting a variable to a numeric value
   (specifically, KPATHSEA_DPI).  We don't need to dynamically allocate
   and free the string for the number, since it's saved as part of the
   environment value.  */

void
xputenv_int P2C(const_string, var_name,  int, num)
{
  char str[MAX_INT_LENGTH];
  sprintf (str, "%d", num);
  
  xputenv (var_name, str);
}
