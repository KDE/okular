/* expand.c: general expansion.

Copyright (C) 1993, 94, 95, 96, 97, 98 Karl Berry & Olaf Weber.

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
#include <kpathsea/expand.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/tilde.h>
#include <kpathsea/variable.h>
#include <kpathsea/concatn.h>
#include <kpathsea/absolute.h>
#include <kpathsea/str-list.h>

/* Do variable expansion first so ~${USER} works.  (Besides, it's what the
   shells do.)  */

string
kpse_expand P1C(const_string, s)
{
  string var_expansion = kpse_var_expand (s);
  string tilde_expansion = kpse_tilde_expand (var_expansion);
  
  /* `kpse_var_expand' always gives us new memory; `kpse_tilde_expand'
     doesn't, necessarily.  So be careful that we don't free what we are
     about to return.  */
  if (tilde_expansion != var_expansion)
    free (var_expansion);
  
  return tilde_expansion;
}


/* Forward declarations of functions from the original expand.c  */
static str_list_type brace_expand P1H(const_string*);

/* If $KPSE_DOT is defined in the environment, prepend it to any relative
   path components. */

static string
kpse_expand_kpse_dot P1C(string, path)
{
  string ret, elt;
  string kpse_dot = getenv("KPSE_DOT");
#ifdef MSDOS
  boolean malloced_kpse_dot = false;
#endif
  
  if (kpse_dot == NULL)
    return path;
  ret = xmalloc(1);
  *ret = 0;

#ifdef MSDOS
  /* Some setups of ported Bash force $KPSE_DOT to have the //d/foo/bar
     form (when `pwd' is used), which is not understood by libc and the OS.
     Convert them back to the usual d:/foo/bar form.  */
  if (kpse_dot[0] == '/' && kpse_dot[1] == '/'
      && kpse_dot[2] >= 'A' && kpse_dot[2] <= 'z' && kpse_dot[3] == '/') {
    kpse_dot++;
    kpse_dot = xstrdup (kpse_dot);
    kpse_dot[0] = kpse_dot[1];  /* drive letter */
    kpse_dot[1] = ':';
    malloced_kpse_dot = true;
  }
#endif

  for (elt = kpse_path_element (path); elt; elt = kpse_path_element (NULL)) {
    string save_ret = ret;
    /* We assume that the !! magic is only used on absolute components.
       Single "." get special treatment, as does "./" or its equivalent. */
    if (kpse_absolute_p (elt, false) || (elt[0] == '!' && elt[1] == '!')) {
      ret = concat3(ret, elt, ENV_SEP_STRING);
    } else if (elt[0] == '.' && elt[1] == 0) {
      ret = concat3 (ret, kpse_dot, ENV_SEP_STRING);
#ifndef VMS
    } else if (elt[0] == '.' && IS_DIR_SEP(elt[1])) {
      ret = concatn (ret, kpse_dot, elt + 1, ENV_SEP_STRING, NULL);
    } else {
      ret = concatn (ret, kpse_dot, DIR_SEP_STRING, elt, ENV_SEP_STRING, NULL);
#endif
    }
    free (save_ret);
  }

#ifdef MSDOS
  if (malloced_kpse_dot) free (kpse_dot);
#endif

  ret[strlen (ret) - 1] = 0;
  return ret;
}

/* Do brace expansion on ELT; then do variable and ~ expansion on each
   element of the result; then do brace expansion again, in case a
   variable definition contained braces (e.g., $TEXMF).  Return a
   string comprising all of the results separated by ENV_SEP_STRING.  */

static string
kpse_brace_expand_element P1C(const_string, elt)
{
  unsigned i;
  str_list_type expansions = brace_expand (&elt);
  string ret = xmalloc (1);
  *ret = 0;

  for (i = 0; i != STR_LIST_LENGTH(expansions); i++) {
    /* Do $ and ~ expansion on each element.  */
    string x = kpse_expand (STR_LIST_ELT(expansions,i));
    string save_ret = ret;
    if (!STREQ (x, STR_LIST_ELT(expansions,i))) {
      /* If we did any expansions, do brace expansion again.  Since
         recursive variable definitions are not allowed, this recursion
         must terminate.  (In practice, it's unlikely there will ever be
         more than one level of recursion.)  */
      string save_x = x;
      x = kpse_brace_expand_element (x);
      free (save_x);
    }
    ret = concat3 (ret, x, ENV_SEP_STRING);
    free (save_ret);
    free (x);
  }
  for (i = 0; i != STR_LIST_LENGTH(expansions); ++i) {
      free(STR_LIST_ELT(expansions,i));
  }
  str_list_free(&expansions);
  ret[strlen (ret) - 1] = 0; /* waste the trailing null */
  return ret;
}

/* Be careful to not waste all the memory we allocate for each element.  */

string
kpse_brace_expand P1C(const_string, path)
{
  string kpse_dot_expansion;
  string elt;
  unsigned len;
  /* Must do variable expansion first because if we have
       foo = .:~
       TEXINPUTS = $foo
     we want to end up with TEXINPUTS = .:/home/karl.
     Since kpse_path_element is not reentrant, we must get all
     the path elements before we start the loop.  */
  string xpath = kpse_var_expand (path);
  string ret = xmalloc (1);
  *ret = 0;

  for (elt = kpse_path_element (xpath); elt; elt = kpse_path_element (NULL)) {
    string save_ret = ret;
    /* Do brace expansion first, so tilde expansion happens in {~ka,~kb}.  */
    string expansion = kpse_brace_expand_element (elt);
    ret = concat3 (ret, expansion, ENV_SEP_STRING);
    free (expansion);
    free (save_ret);
  }

  /* Waste the last byte by overwriting the trailing env_sep with a null.  */
  len = strlen (ret);
  if (len != 0)
    ret[len - 1] = 0;
  free (xpath);

  kpse_dot_expansion = kpse_expand_kpse_dot (ret);
  if (kpse_dot_expansion != ret)
    free (ret);

  return kpse_dot_expansion;
}

/* Expand all special constructs in a path, and include only the actually
   existing directories in the result. */
string
kpse_path_expand P1C(const_string, path)
{
  string ret;
  string xpath;
  string elt;
  unsigned len;

  /* Initialise ret to the empty string. */
  ret = xmalloc (1);
  *ret = 0;
  len = 0;
  
  /* Expand variables and braces first.  */
  xpath = kpse_brace_expand (path);

  /* Now expand each of the path elements, printing the results */
  for (elt = kpse_path_element (xpath); elt; elt = kpse_path_element (NULL)) {
    str_llist_type *dirs;

    /* Skip and ignore magic leading chars.  */
    if (*elt == '!' && *(elt + 1) == '!')
      elt += 2;

    /* Do not touch the device if present */
    if (NAME_BEGINS_WITH_DEVICE (elt)) {
      while (IS_DIR_SEP (*(elt + 2)) && IS_DIR_SEP (*(elt + 3))) {
        *(elt + 2) = *(elt + 1);
        *(elt + 1) = *elt;
        elt++;
      }
    } else {
      /* We never want to search the whole disk.  */
      while (IS_DIR_SEP (*elt) && IS_DIR_SEP (*(elt + 1)))
        elt++;
    }

    /* Search the disk for all dirs in the component specified.
       Be faster to check the database, but this is more reliable.  */
    dirs = kpse_element_dirs (elt); 
    if (dirs && *dirs) {
      str_llist_elt_type *dir;

      for (dir = *dirs; dir; dir = STR_LLIST_NEXT (*dir)) {
        string thedir = STR_LLIST (*dir);
        unsigned dirlen = strlen (thedir);
        string save_ret = ret;
        /* Retain trailing slash if that's the root directory.  */
        if (dirlen == 1 || (dirlen == 3 && NAME_BEGINS_WITH_DEVICE (thedir)
                            && IS_DIR_SEP (thedir[2]))) {
          ret = concat3 (ret, thedir, ENV_SEP_STRING);
          len += dirlen + 1;
          ret[len - 1] = ENV_SEP;
        } else {
          ret = concat (ret, thedir);
          len += dirlen;
          ret [len - 1] = ENV_SEP;
        }
        free (save_ret);
      }
    }
  }
  /* Get rid of trailing ':', if any. */
  if (len != 0)
    ret[len - 1] = 0;
  return ret;
}

/* ... */
static void expand_append P3C(str_list_type*, partial,
                              const_string, text, const_string, p)
{
    string new_string;
    unsigned len;
    str_list_type tmp;
    tmp = str_list_init();
    len = p - text;
    new_string = xmalloc(len+1);
    strncpy(new_string, text, len);
    new_string[len]=0;
    str_list_add(&tmp, new_string);
    str_list_concat_elements(partial, tmp);
}
                              


static str_list_type brace_expand P1C(const_string *, text)
{
    str_list_type result, partial, recurse;
    const_string p;
    result = str_list_init();
    partial = str_list_init();
    for (p = *text; *p && *p != '}'; ++p) {
        if (*p == ENV_SEP || *p == ',') {
            expand_append(&partial, *text, p);
            str_list_concat(&result, partial);
            str_list_free(&partial);
            *text = p+1;
            partial = str_list_init();
        } else if (*p == '{') {
            expand_append(&partial, *text, p);
            ++p;
            recurse = brace_expand(&p);
            str_list_concat_elements(&partial, recurse);
            str_list_free(&recurse);
            /* Check for missing closing brace. */
            if (*p != '}') {
                WARNING1 ("%s: Unmatched {", *text);
            }
            *text = p+1;
        } else if (*p == '$') {
            /* Skip ${VAR} */
            if (*(p+1) == '{')
                for (p+=2; *p!='}';++p);
        }
    }
    expand_append(&partial, *text, p);
    str_list_concat(&result, partial);
    str_list_free(&partial);
    *text = p;
    return result;
}



#if defined (TEST)
#include <stdio.h>

fatal_error (format, arg1, arg2)
     char *format, *arg1, *arg2;
{
  report_error (format, arg1, arg2);
  exit (1);
}

report_error (format, arg1, arg2)
     char *format, *arg1, *arg2;
{
  fprintf (stderr, format, arg1, arg2);
  fprintf (stderr, "\n");
}

main ()
{
  char example[256];

  for (;;)
    {
      char **result;
      int i;

      fprintf (stderr, "brace_expand> ");

      if ((!fgets (example, 256, stdin)) ||
	  (strncmp (example, "quit", 4) == 0))
	break;

      if (strlen (example))
	example[strlen (example) - 1] = 0;

      result = brace_expand (example);

      for (i = 0; result[i]; i++)
	printf ("%s\n", result[i]);

      free_array (result);
    }
}

/*
 * Local variables:
 * test-compile-command: "gcc -g -DTEST -I.. -I. -o brace_expand braces.c -L. -lkpathsea"
 * end:
 */

#endif /* TEST */
