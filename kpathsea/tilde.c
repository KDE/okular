/* tilde.c: Expand user's home directories.

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

#include <kpathsea/c-pathch.h>
#include <kpathsea/tilde.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif


/* If NAME has a leading ~ or ~user, Unix-style, expand it to the user's
   home directory, and return a new malloced string.  If no ~, or no
   <pwd.h>, just return NAME.  */

string
kpse_tilde_expand P1C(const_string, name)
{
#ifdef HAVE_PWD_H
  const_string expansion;
  const_string home;
  
  assert (name);
  
  /* If no leading tilde, do nothing.  */
  if (*name != '~')
    expansion = name;
  
  /* If `~' or `~/', use $HOME if it exists, or `.' if it doesn't.  */
  else if (IS_DIR_SEP (name[1]) || name[1] == 0)
    {
      home = getenv ("HOME");
      if (!home)
        home = ".";
        
      expansion = name[1] == 0
                  ? xstrdup (home) : concat3 (home, DIR_SEP_STRING, name + 2);
    }
  
  /* If `~user' or `~user/', look up user in the passwd database.  */
  else
    {
      struct passwd *p;
      string user;
      unsigned c = 2;
      while (!IS_DIR_SEP (name[c]) && name[c] != 0)
        c++;
      
      user = (string) xmalloc (c);
      strncpy (user, name + 1, c - 1);
      user[c - 1] = 0;
      
      /* We only need the cast here for those (deficient) systems
         which do not declare `getpwnam' in <pwd.h>.  */
      p = (struct passwd *) getpwnam (user);
      free (user);

      /* If no such user, just use `.'.  */
      home = p == NULL ? "." : p->pw_dir;
      
      expansion = name[c] == 0 ? xstrdup (home) : concat (home, name + c);
    }
  
  /* We may return the same thing as the original, and then we might not
     be returning a malloc-ed string.  */
  return (string) expansion;
#else
  return name;
#endif /* not HAVE_PWD_H */
}

#ifdef TEST

void
test_expand_tilde (const_string filename)
{
  string answer;
  
  printf ("Tilde expansion of `%s':\t", filename ? filename : "(null)");
  answer = kpse_expand_tilde (filename);
  puts (answer);
}

int
main ()
{
  string tilde_path = "tilde";

  test_expand_tilde ("");
  test_expand_tilde ("none");
  test_expand_tilde ("~root");
  test_expand_tilde ("~");
  test_expand_tilde ("foo~bar");
  
  return 0;
}

#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST tilde.c kpathsea.a"
End:
*/
