/* concatn.c: Concatenate an arbitrary number of strings.

Copyright (C) 1993, 95 Karl Berry.

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

#include <kpathsea/concatn.h>


/* OK, it would be epsilon more efficient to compute the total length
   and then do the copying ourselves, but I doubt it matters in reality.  */

string
concatn PVAR1C(const_string, str1,  ap)
{
  string arg;
  string ret;

  if (!str1)
    return NULL;
  
  ret = xstrdup (str1);
  
  while ((arg = va_arg (ap, string)) != NULL)
    {
      string temp = concat (ret, arg);
      free (ret);
      ret = temp;
    }
  va_end (ap);
  
  return ret;
}}

#ifdef TEST
int
main ()
{
  printf ("null = \"%s\"\n", concatn (NULL));
  printf ("\"a\" = \"%s\"\n", concatn ("a", NULL));
  printf ("\"ab\" = \"%s\"\n", concatn ("a", "b", NULL));
  printf ("\"abc\" = \"%s\"\n", concatn ("a", "b", "c", NULL));
  printf ("\"abcd\" = \"%s\"\n", concatn ("ab", "cd", NULL));
  printf ("\"abcde\" = \"%s\"\n", concatn ("ab", "c", "de", NULL));
  printf ("\"abcdef\" = \"%s\"\n", concatn ("", "a", "", "bcd", "ef", NULL));
  return 0;
}

#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -posix -g -I. -I.. -DTEST concatn.c kpathsea.a"
End:
*/
