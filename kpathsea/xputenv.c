/* xputenv.c: set an environment variable without return.

Copyright (C) 1993, 94, 95, 96, 97, 98 Karl Berry.

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

#ifdef WIN32
#include <stdlib.h>
#else
/* Avoid implicit declaration warning.  But since some systems do
   declare it, don't use a prototype, for fear of conflicts.  */
extern int putenv ();
#endif /* not WIN32 */

/* This `x' function is different from the others in that it takes
   different parameters than the standard function; but I find it much
   more convenient to pass the variable and the value separately.  Also,
   this way we can guarantee that the environment value won't become
   garbage.  Also, putenv just overwrites old entries with
   the new, and we want to reclaim that space -- this may be called
   hundreds of times on a run.
   
   But naturally, some systems do it differently. In this case, it's
   net2 that is smart and does its own saving/freeing.  configure tries
   to determine this.  */

void
xputenv P2C(const_string, var_name,  const_string, value)
{
  string old_item = NULL;
  string new_item = concat3 (var_name, "=", value);
  unsigned name_len = strlen (var_name);

#ifndef SMART_PUTENV

  static const_string *saved_env_items = NULL;
  static unsigned saved_len;
  boolean found = false;

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
      for (i = 0; i < saved_len && !found; i++)
        {
          if (STREQ (saved_env_items[i], var_name))
            {
              found = true;
              old_item = getenv (var_name);
#ifdef WIN32
	      /* win32 putenv() removes the variable if called with
		 "VAR=". So we have to cope with this case. Distinction
		 is not made between the value being "" or the variable
		 not set. */
	      if (old_item)
		old_item -= (name_len + 1);
#else
              assert (old_item);
              /* Back up to the `NAME=' in the environment before the
                 value that getenv returns.  */
              old_item -= (name_len + 1);
#endif
            }
        }

      if (!found)
        {
          /* If we haven't seen VAR_NAME before, save it.  Assume it is
             in safe storage.  */
          saved_len++;
          XRETALLOC (saved_env_items, saved_len, const_string);
          saved_env_items[saved_len - 1] = var_name;
        }
    }
#endif /* not SMART_PUTENV */

  /* If the old and the new values are identical, don't do anything.
     This is both more memory-efficient and safer as far as our
     assumptions (about how putenv is implemented in libc) go.  */
  if (!old_item || !STREQ (old_item, new_item))
    {
      char *new_val;
      /* As far as I can see there's no way to distinguish between the
         various errors; putenv doesn't have errno values.  */
      if (putenv (new_item) < 0)
        FATAL1 ("putenv (%s) failed", new_item);

      /* If their putenv copied `new_item', we can free it.  */
      new_val = getenv (var_name);
      if (new_val && new_val - name_len - 1 != new_item)
        free (new_item);

#ifndef SMART_PUTENV
      /* Can't free `new_item' because its contained value is now in
         `environ', but we can free `old_item', since it's been replaced.  */
#ifndef WIN32
      /* ... except on Win32, where old_item points to garbage.
         Or at least non free-able memory. Or at least, that's what
         BoundsChecker says... FP, 06/10/98) */
      if (old_item)
        free (old_item);
#endif
#endif /* not SMART_PUTENV */
    }
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
