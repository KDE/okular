/* kpsewhich -- standalone path lookup and variable expansion for Kpathsea.
   Ideas from Tom Esser and Pierre MacKay.

Copyright (C) 1995 Karl Berry.

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
#include <kpathsea/getopt.h>
#include <kpathsea/line.h>
#include <kpathsea/proginit.h>
#include <kpathsea/progname.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/tex-glyph.h>
#include <kpathsea/variable.h>


/* Base resolution. (-D, -dpi) */
unsigned dpi = 300;

/* A construct to do variable expansion on.  (-expand) */
string expand = NULL;

/* The file type for lookups.  (-format) */
kpse_file_format_type format = kpse_last_format;

/* Ask interactively?  (-interactive) */
boolean interactive = false;

/* The device name, for $MAKETEX_MODE.  (-mode) */
string mode = NULL;

/* The program name, for `.program' in texmf.cnf.  (-program) */
string progname = NULL;

/* Return the <number> substring in `<name>.<number><stuff>', if S has
   that form.  If it doesn't, return 0.  */

static unsigned
find_dpi P1C(string, s)
{
  unsigned dpi_number = 0;
  string extension = find_suffix (s);
  
  if (extension != NULL)
    sscanf (extension, "%u", &dpi_number);

  return dpi_number;
}

/* Use the -format if specified, else guess dynamically from NAME.  */

/* We could partially initialize this array from `kpse_format_info',
   but it doesn't seem worth the trouble.  */
static const_string suffix[]
  = { "gf", "pk", "", ".base", ".bib", ".bst", ".cnf", ".fmt", ".mf",
      ".pool", ".eps", ".tex", ".pool", ".tfm", ".vf", ".ps", ".pfa" };

static kpse_file_format_type
find_format P1C(string, name)
{
  kpse_file_format_type ret;
  
  if (format != kpse_last_format)
    ret = format;
  else
    {
      unsigned name_len = strlen (name);
      for (ret = 0; ret < kpse_last_format; ret++)
        {
          const_string suffix_try = suffix[ret];
          unsigned try_len = strlen (suffix_try);
          if (try_len && try_len < name_len
              && STREQ (suffix_try, name + name_len - try_len))
            break;
        }
      if (ret == kpse_last_format)
        {
          fprintf (stderr,
                   "kpsewhich: Can't guess format for %s, using tex.\n", name);
          ret = kpse_tex_format;
        }
    }
  
  return ret;
}

/* Look up a single filename NAME.  Return 0 if success, 1 if failure.  */

static unsigned
lookup P1C(string, name)
{
  string ret;
  unsigned local_dpi;
  kpse_glyph_file_type glyph_ret;
  kpse_file_format_type fmt = find_format (name);
  
  switch (fmt)
    {
    case kpse_pk_format:
    case kpse_gf_format:
    case kpse_any_glyph_format:
      /* Try to extract the resolution from the name.  */
      local_dpi = find_dpi (name);
      if (!local_dpi)
        local_dpi = dpi;
      ret = kpse_find_glyph (remove_suffix (name), local_dpi, fmt, &glyph_ret);
      break;
    default:
      ret = kpse_find_file (name, fmt, false);
    }
  
  if (ret)
    puts (ret);
  
  return ret == NULL;
}

/* Reading the options.  */

#define USAGE "Options:\n\
  You can use `--' or `-' to start an option.\n\
  You can use any unambiguous abbreviation for an option name.\n\
  You can separate option names and values with `=' or ` '.\n\
debug <integer>: set debugging flags.\n\
D, dpi <unsigned>: use a base resolution of <unsigned>; default 300.\n\
expand <string>: do variable expansion on <string>.\n\
help: print this message and exit.\n\
interactive: ask for additional filenames to look up.\n\
mode <string>: set device name for $MAKETEX_MODE to <string>; no default.\n\
version: print version number and exit.\n\
"

/* Test whether getopt found an option ``A''.
   Assumes the option index is in the variable `option_index', and the
   option table in a variable `long_options'.  */
#define ARGUMENT_IS(a) STREQ (long_options[option_index].name, a)

/* SunOS cc can't initialize automatic structs.  */
static struct option long_options[]
  = { { "debug",		1, 0, 0 },
      { "dpi",			1, 0, 0 },
      { "D",			1, 0, 0 },
      { "expand",		1, 0, 0 },
      { "format",		1, 0, 0 },
      { "help",                 0, 0, 0 },
      { "interactive",		0, (int *) &interactive, 1 },
      { "mode",			1, 0, 0 },
      { "program",		1, 0, 0 },
      { "version",              0, 0, 0 },
      { 0, 0, 0, 0 } };


/* We return the name of the font to process.  */

static void
read_command_line P2C(int, argc,  string *, argv)
{
  int g;   /* `getopt' return code.  */
  int option_index;

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("debug"))
        kpathsea_debug |= atoi (optarg);

      else if (ARGUMENT_IS ("dpi") || ARGUMENT_IS ("D"))
        dpi = atoi (optarg);

      else if (ARGUMENT_IS ("expand"))
        expand = optarg;

      else if (ARGUMENT_IS ("format"))
        format = atoi (optarg);

      else if (ARGUMENT_IS ("help"))
        {
          printf ("Usage: %s [options] [filenames].\n", argv[0]);
          fputs (USAGE, stdout);
          exit (0);
        }

      else if (ARGUMENT_IS ("mode"))
        mode = optarg;

      else if (ARGUMENT_IS ("program"))
        progname = optarg;

      else if (ARGUMENT_IS ("version"))
        {
          extern char *kpathsea_version_string; /* from version.c */
          printf ("%s.\n", kpathsea_version_string);
          exit (0);
        }
      
      /* Else it was just a flag; getopt has already done the assignment.  */
    }
}

int
main P2C(int, argc,  string *, argv)
{
  unsigned unfound = 0;
  
  read_command_line (argc, argv);
  
  if (!progname)
    progname = (string) "";

  kpse_set_progname (progname);
  
  /* false for no MakeTeXPK, NULL for no fallback font.  */
  kpse_init_prog (uppercasify (progname), dpi, mode, false, NULL);
  
  /* No extra frills on expansion output, to make it easy to use.  */
  if (expand)
    puts (kpse_var_expand (expand));

  for (; optind < argc; optind++)
    {
      unfound += lookup (argv[optind]);
    }

  if (interactive)
    {
      for (;;)
        {
          string name = read_line (stdin);
          if (!name || STREQ (name, "q") || STREQ (name, "quit")) break;
          unfound += lookup (name);
          free (name);
        }
    }
  
  exit (unfound);
}
