/* variable.c: variable expansion.

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

#include <kpathsea/c-ctype.h>
#include <kpathsea/cnf.h>
#include <kpathsea/fn.h>
#include <kpathsea/variable.h>


/* Append the result of value of `var' to EXPANSION, where `var' begins
   at START and ends at END.  If `var' is not set, do not complain.  */

static void
expand P3C(fn_type *, expansion,  const_string, start,  const_string, end)
{
  string value;
  unsigned len = end - start + 1;
  string var = xmalloc (len + 1);
  strncpy (var, start, len);
  var[len] = 0;
  
  /* First check the environment variable.  */
  value = getenv (var);
  
  /* If no envvar, check the config files.  */
  if (!value)
    value = kpse_cnf_get (var);
    
  if (value)
    {
      value = kpse_var_expand (value);
      fn_grow (expansion, value, strlen (value));
      free (value);
    }
  
  free (var);
}

/* Can't think of when it would be useful to change these (and the
   diagnostic messages assume them), but ... */
#ifndef IS_VAR_START /* starts all variable references */
#define IS_VAR_START(c) ((c) == '$')
#endif
#ifndef IS_VAR_CHAR  /* variable name constituent */
#define IS_VAR_CHAR(c) (ISALNUM (c) || (c) == '_')
#endif
#ifndef IS_VAR_BEGIN_DELIMITER /* start delimited variable name (after $) */
#define IS_VAR_BEGIN_DELIMITER(c) ((c) == '{')
#endif
#ifndef IS_VAR_END_DELIMITER
#define IS_VAR_END_DELIMITER(c) ((c) == '}')
#endif


/* Maybe we should generalize to allow some or all of the various shell
   ${...} constructs, especially ${var-value}.  */

string
kpse_var_expand P1C(const_string, src)
{
  const_string s;
  string ret;
  fn_type expansion;
  expansion = fn_init ();
  
  /* Copy everything but variable constructs.  */
  for (s = src; *s; s++)
    {
      if (IS_VAR_START (*s))
        {
          s++;
          
          /* Three cases: `$VAR', `${VAR}', `$<anything-else>'.  */
          
          if (IS_VAR_CHAR (*s))
            { /* $V: collect name constituents, then expand.  */
              const_string var_end = s;
              
              do
                var_end++;
              while (IS_VAR_CHAR (*var_end));
              
              var_end--; /* had to go one past */
              expand (&expansion, s, var_end);
              s = var_end;
            }

          else if (IS_VAR_BEGIN_DELIMITER (*s))
            { /* ${: scan ahead for matching delimiter, then expand.  */
              const_string var_end = ++s;
              
              while (*var_end && !IS_VAR_END_DELIMITER (*var_end))
                var_end++;
              
              if (! *var_end)
                {
                  WARNING1 ("%s: No matching right brace for ${", src);
                  s = var_end - 1; /* will incr to null at top of loop */
                }
              else
                {
                  expand (&expansion, s, var_end - 1);
                  s = var_end; /* will incr past } at top of loop*/
                }
            }


          else
            { /* $<something-else>: error.  */
              WARNING2 ("%s: Unrecognized variable construct `$%c'", src, *s);
              /* Just ignore those chars and keep going.  */
            }
        }
      else
        fn_1grow (&expansion, *s);
    }
  fn_1grow (&expansion, 0);
          
  ret = FN_STRING (expansion);
  return ret;
}

#ifdef TEST

static void
test_var (string test, string right_answer)
{
  string result = kpse_var_expand (test);
  
  printf ("expansion of `%s'\t=> %s", test, result);
  if (!STREQ (result, right_answer))
    printf (" [should be `%s']", right_answer);
  putchar ('\n');
}


int
main ()
{
   test_var ("a", "a");
   test_var ("$foo", "");
   test_var ("a$foo", "a");
   test_var ("$foo a", " a");
   test_var ("a$foo b", "a b");

   xputenv ("FOO", "foo value");
   test_var ("a$FOO", "afoo value");
   
   xputenv ("Dollar", "$");
   test_var ("$Dollar a", "$ a");
   
   test_var ("a${FOO}b", "afoo valueb");
   test_var ("a${}b", "ab");
   
   test_var ("$$", ""); /* and error */
   test_var ("a${oops", "a"); /* and error */
   
   return 0;
}

#endif /* TEST */


/*
Local variables:
standalone-compile-command: "gcc -g -I. -I.. -DTEST variable.c kpathsea.a"
End:
*/
