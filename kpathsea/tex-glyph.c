/* tex-glyph.c: Search for GF/PK files.

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

#include <kpathsea/absolute.h>
#include <kpathsea/expand.h>
#include <kpathsea/fontmap.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/tex-glyph.h>
#include <kpathsea/tex-make.h>
#include <kpathsea/variable.h>

/* Routines are in bottom-up order.  */

/* Support both cmr10.300pk and dpi300/cmr10.pk.  (Use the latter
   instead of dpi300\cmr10.pk since DOS supports /'s, but Unix doesn't
   support \'s.  */
#define UNIX_BITMAP_SPEC "$KPATHSEA_NAME.$KPATHSEA_DPI$KPATHSEA_FORMAT"
#define DPI_BITMAP_SPEC  "dpi$KPATHSEA_DPI/$KPATHSEA_NAME.$KPATHSEA_FORMAT"

/* Look up FONT_NAME at resolution DPI in PATH, with filename suffix
   EXTENSION.  Return file found or NULL.  */

static string
try_format P3C(const_string, font_name,  unsigned, dpi,
               kpse_file_format_type,  format)
{
  static const_string bitmap_specs[]
    = { UNIX_BITMAP_SPEC, DPI_BITMAP_SPEC, NULL };
  const_string *spec;
  boolean must_exist;
  string ret = NULL;
  const_string path = kpse_format_info[format].path;
  if (!path)
    path = kpse_init_format (format);
  
  /* Set the suffix on the name we'll be searching for.  */
  xputenv ("KPATHSEA_FORMAT", kpse_format_info[format].suffix);

  /* OK, the limits on this for loop are a little hokey, but it saves
     having to repeat the body.  We want to do it once with `must_exist'
     false to avoid looking on the disk for cmr10.600pk if
     dpi600/cmr10.pk is in ls-R.  (The time spent in the extra variable
     expansions and db searches is negligible.)  */
  for (must_exist = false; !ret && must_exist <= true; must_exist++)
    {
      for (spec = bitmap_specs; !ret && *spec; spec++)
        {
          string name = kpse_var_expand (*spec);
          ret = kpse_path_search (path, name, must_exist);
          if (name != ret)
            free (name);
        }
    }
    
  return ret;
}

/* Look for FONT_NAME at resolution DPI in format FORMAT.  Search the
   (entire) PK path first, then the GF path, if we're looking for both.
   Return any filename found, and (if we succeeded) fill in GLYPH_FILE.  */

static string
try_size P4C(const_string, font_name,  unsigned, dpi,
             kpse_file_format_type, format,
             kpse_glyph_file_type *, glyph_file)
{
  kpse_file_format_type format_found;
  string ret;
  boolean try_gf = format == kpse_gf_format || format == kpse_any_glyph_format;
  boolean try_pk = format == kpse_pk_format || format == kpse_any_glyph_format;

  xputenv_int ("KPATHSEA_DPI", dpi);
  
  /* Look for PK first (since it's more likely to be found), then GF.  */
  ret = try_pk ? try_format (font_name, dpi, kpse_pk_format) : NULL;

  if (ret != NULL)
    format_found = kpse_pk_format;
  else
    {
      if (try_gf)
        {
          ret = try_format (font_name, dpi, kpse_gf_format);
          format_found = kpse_gf_format;
        }
    }
  
  if (ret != NULL && glyph_file)
    { /* Success.  Fill in the return info.  Discard const.  */
      glyph_file->name = (string) font_name;
      glyph_file->dpi = dpi;
      glyph_file->format = format_found;
    }
    
  return ret;
}

/* Look for FONT_NAME at resolution DPI, then at the resolutions within
   KPSE_BITMAP_TOLERANCE of DPI.  */

static string
try_resolution P4C(const_string, font_name,  unsigned, dpi,
                   kpse_file_format_type, format,
                   kpse_glyph_file_type *, glyph_file)
{
  string ret = try_size (font_name, dpi, format, glyph_file);
  
  if (!ret)
    {
      unsigned r;
      unsigned tolerance = KPSE_BITMAP_TOLERANCE (dpi);
      unsigned lower_bound = (int) (dpi - tolerance) < 0 ? 0 : dpi - tolerance;
      unsigned upper_bound = dpi + tolerance;
      
      /* Prefer scaling up to scaling down, since scaling down can omit
         character features (Tom did this in dvips).  */
      for (r = lower_bound; !ret && r <= upper_bound; r++)
        if (r != dpi)
          ret = try_size (font_name, r, format, glyph_file);
    }
  
  return ret;
}

/* Look up FONT_NAME in format FORMAT at DPI in the texfonts.map files
   that we can find, returning the filename found and GLYPH_FILE.  */

static string
try_fontmap P4C(const_string, font_name,  unsigned, dpi,
                kpse_file_format_type, format,
                kpse_glyph_file_type *, glyph_file)
{
  static hash_table_type fontmap;
  string *mapped_names;
  string ret = NULL;

  if (fontmap.size == 0)
    { /* If we wanted to complicate our lives, we could handle separate
         maps for GF and PK ones.  I don't see that this has any
         practical utility, though, because if someone wants an alias,
         most likely the alias should apply to non-glyphs as well as
         glyphs (let alone to only GF format or PK format).  */
      const_string map_path = kpse_init_format (kpse_any_glyph_format);
      fontmap = map_create (map_path);
    }

  mapped_names = map_lookup (fontmap, font_name);
  if (mapped_names)
    {
      string mapped_name;
      while ((mapped_name = *mapped_names++) && !ret)
        {
          xputenv ("KPATHSEA_NAME", mapped_name);
          ret = try_resolution (mapped_name, dpi, format, glyph_file);
        }
    }

  return ret;
}

/* Look for FONT_NAME in `kpse_fallback_resolutions', omitting DPI if we
   happen across it.  Return NULL if nothing found.  Pass GLYPH_FILE
   along as usual.  Assume `kpse_fallback_resolutions' is sorted.  */

static string
try_fallback_resolutions P4C(const_string, font_name,  unsigned, dpi,
                             kpse_file_format_type, format,
                             kpse_glyph_file_type *, glyph_file)
{
  unsigned s;
  int loc, max_loc;
  int lower_loc, upper_loc;
  unsigned lower_diff, upper_diff;
  unsigned closest_diff = UINT_MAX;
  string ret = NULL; /* In case the only fallback resolution is DPI.  */

  /* First find the fallback size closest to DPI.  */
  for (s = 0; kpse_fallback_resolutions[s] != 0; s++)
    {
      unsigned this_diff = abs (kpse_fallback_resolutions[s] - dpi);
      if (this_diff < closest_diff)
        {
          closest_diff = this_diff;
          loc = s;
        }
    }
  if (s == 0)
    return ret; /* If nothing in list, quit now.  */
  
  max_loc = s;
  lower_loc = loc - 1;
  upper_loc = loc + 1;
  
  for (;;)
    {
      unsigned fallback = kpse_fallback_resolutions[loc];
      /* Don't bother to try DPI itself again.  */
      if (fallback != dpi)
        {
          ret = try_resolution (font_name, fallback, format, glyph_file);
          if (ret)
            break;
        }
      
      /* That didn't work. How far away are the locs above or below?  */
      lower_diff = lower_loc > -1
                   ? dpi - kpse_fallback_resolutions[lower_loc] : INT_MAX;
      upper_diff = upper_loc < max_loc
                   ? kpse_fallback_resolutions[upper_loc] - dpi : INT_MAX;
      
      /* But if we're at the end in both directions, quit.  */
      if (lower_diff == INT_MAX && upper_diff == INT_MAX)
        break;
      
      /* Go in whichever direction is closest.  */
      if (lower_diff < upper_diff)
        {
          loc = lower_loc;
          lower_loc--;
        }
      else
        {
          loc = upper_loc;
          upper_loc++;
        }
    }

  return ret;
}

/* See the .h file for description.  This is the entry point.  */

string
kpse_find_glyph P4C(const_string, font_name,  unsigned, dpi,
                    kpse_file_format_type, format,
                    kpse_glyph_file_type *, glyph_file)
{
  string ret;
  kpse_glyph_source_type source;
  
  /* Start the search: try the name we're given.  */
  source = kpse_glyph_source_normal;
  xputenv ("KPATHSEA_NAME", font_name);
  ret = try_resolution (font_name, dpi, format, glyph_file);
  
  /* Try all the various possibilities in order of preference.  */
  if (!ret)
    {
      /* Maybe FONT_NAME was an alias.  */
      source = kpse_glyph_source_alias;
      ret = try_fontmap (font_name, dpi, format, glyph_file);

      /* If not an alias, try creating it on the fly with MakeTeXPK,
         unless FONT_NAME is absolute or explicitly relative.  */
      if (!ret && !kpse_absolute_p (font_name, true))
        {
          source = kpse_glyph_source_maketex;
          /* `try_resolution' leaves the envvar set randomly.  */
          xputenv_int ("KPATHSEA_DPI", dpi);
          ret = kpse_make_tex (format, font_name);
        }
       
      /* If MakeTeX... succeeded, set return struct.  Doesn't make sense for
         `kpse_make_tex' to set it, since it can only succeed or fail,
         unlike the other routines.  */
      if (ret)
        {
          KPSE_GLYPH_FILE_DPI (*glyph_file) = dpi;
          /* Have to discard const here.  */
          KPSE_GLYPH_FILE_NAME (*glyph_file) = (string) font_name;
        }

      /* If MakeTeX... failed, try any fallback resolutions.  */
      else
        {
          if (kpse_fallback_resolutions)
            ret = try_fallback_resolutions(font_name, dpi, format, glyph_file);

          /* We're down to the font of last resort.  */
          if (!ret && kpse_fallback_font)
            {
              const_string name = kpse_fallback_font;
              source = kpse_glyph_source_fallback;
              xputenv ("KPATHSEA_NAME", name);

              /* As before, first try it at the given size.  */
              ret = try_resolution (name, dpi, format, glyph_file);
              
              /* The fallback font at the fallback resolutions.  */
              if (!ret && kpse_fallback_resolutions)
                ret = try_fallback_resolutions (name, dpi, format, glyph_file);
            }
        }
    }
  
  /* If RET is null, then the caller is not supposed to look at GLYPH_FILE,
     so it doesn't matter if we assign something incorrect.  */
  KPSE_GLYPH_FILE_SOURCE (*glyph_file) = source;
  
  return ret;
}

/* The tolerances change whether we base things on DPI1 or DPI2.  */

boolean
kpse_bitmap_tolerance P2C(double, dpi1,  double, dpi2)
{
  unsigned tolerance = KPSE_BITMAP_TOLERANCE (dpi2);
  unsigned lower_bound = (int) (dpi2 - tolerance) < 0 ? 0 : dpi2 - tolerance;
  unsigned upper_bound = dpi2 + tolerance;

  return lower_bound <= dpi1 && dpi1 <= upper_bound;
}

#ifdef TEST

void
test_find_glyph (const_string font_name, unsigned dpi)
{
  string answer;
  kpse_glyph_file_type ret;
  
  printf ("\nSearch for %s@%u:\n\t", font_name, dpi);

  answer = kpse_find_glyph_format (font_name, dpi,
                                   kpse_any_glyph_format, &ret);
  if (answer)
    {
      string format = ret.format == kpse_pk_format ? "pk" : "gf";
      if (!ret.name)
        ret.name = "(null)";
      printf ("%s\n\t(%s@%u, %s)\n", answer, ret.name, ret.dpi, format);
    }
  else
    puts ("(null)");
}


int
main ()
{
  test_find_glyph ("/usr/local/lib/tex/fonts/cm/cmr10", 300); /* absolute */
  test_find_glyph ("cmr10", 300);     /* normal */
  test_find_glyph ("logo10", 300);    /* find gf */
  test_find_glyph ("cmr10", 299);     /* find 300 */
  test_find_glyph ("circle10", 300);  /* in fontmap */
  test_find_glyph ("none", 300);      /* do not find */
  kpse_fallback_font = "cmr10";
  test_find_glyph ("fallback", 300);  /* find fallback font cmr10 */
  kpse_init_fallback_resolutions ("KPATHSEA_TEST_SIZES");
  test_find_glyph ("fallbackdpi", 759); /* find fallback font cmr10@300 */
  
  xputenv ("GFFONTS", ".");
  test_find_glyph ("cmr10", 300);     /* different GFFONTS/TEXFONTS */
  
  return 0;
}

#endif /* TEST */


/*
Local variables:
test-compile-command: "gcc -g -I. -I.. -DTEST tex-glyph.c kpathsea.a"
End:
*/
