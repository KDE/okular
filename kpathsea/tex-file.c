/* tex-file.c: stuff for all TeX formats.

Copyright (C) 1993, 94, 95 Karl Berry.

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

#include <kpathsea/c-vararg.h>
#include <kpathsea/cnf.h>
#include <kpathsea/default.h>
#include <kpathsea/expand.h>
#include <kpathsea/paths.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/tex-file.h>


/* See tex-file.h.  */
const_string kpse_fallback_font = NULL;
unsigned *kpse_fallback_resolutions = NULL;
string kpse_font_override_path = NULL;
kpse_format_info_type kpse_format_info[kpse_last_format];

/* These are not in the structure
   because it's annoying to initialize lists in C.  */
#define KPSE_GF_ENVS "GFFONTS", KPSE_GLYPH_ENVS
#define KPSE_PK_ENVS "PKFONTS", "TEXPKS", KPSE_GLYPH_ENVS
#define KPSE_GLYPH_ENVS "GLYPHFONTS", "TEXFONTS"
#define KPSE_BASE_ENVS "MFBASES"
#define KPSE_BIB_ENVS "BIBINPUTS"
#define KPSE_BST_ENVS "BSTINPUTS"
#define KPSE_CNF_ENVS "TEXMFCNF"
#define KPSE_FMT_ENVS "TEXFORMATS"
#define KPSE_MEM_ENVS "MPMEMS"
#define KPSE_MF_ENVS "MFINPUTS"
#define KPSE_MFPOOL_ENVS "MFPOOL"
#define KPSE_MP_ENVS "MPINPUTS"
#define KPSE_MPPOOL_ENVS "MPPOOL"
#define KPSE_MPSUPPORT_ENVS "MPSUPPORT"
#define KPSE_PICT_ENVS "TEXPICTS", KPSE_TEX_ENVS
#define KPSE_TEX_ENVS "TEXINPUTS"
#define KPSE_TEXPOOL_ENVS "TEXPOOL"
#define KPSE_TFM_ENVS "TFMFONTS", "TEXFONTS"
#define KPSE_VF_ENVS "VFFONTS", "TEXFONTS"
#define KPSE_DVIPS_CONFIG_ENVS "TEXCONFIG"
#define KPSE_DVIPS_HEADER_ENVS "DVIPSHEADERS"
#define KPSE_TROFF_FONT_ENVS "TRFONTS"

/* The compiled-in default list, DEFAULT_FONT_SIZES, is intended to be
   set from the command line (presumably via the Makefile).  */

#ifndef DEFAULT_FONT_SIZES
#define DEFAULT_FONT_SIZES ""
#endif

void
kpse_init_fallback_resolutions P1C(string, envvar)
{
  const_string size_var = ENVVAR (envvar, "TEXSIZES");
  string size_str = getenv (size_var);
  unsigned *last_resort_sizes = NULL;
  unsigned size_count = 0;
  string size_list = kpse_expand_default (size_str, DEFAULT_FONT_SIZES);

  /* Initialize the list of last-resort sizes.  */
  for (size_str = kpse_path_element (size_list); size_str != NULL;
       size_str = kpse_path_element (NULL))
    {
      if (! *size_str)
        continue;

      size_count++;
      XRETALLOC (last_resort_sizes, size_count, unsigned);
      last_resort_sizes[size_count - 1] = atoi (size_str);
    }

  /* Add a zero to mark the end of the list.  */
  size_count++;
  XRETALLOC (last_resort_sizes, size_count, unsigned);
  last_resort_sizes[size_count - 1] = 0;

  kpse_fallback_resolutions = last_resort_sizes;
}

/* Find the final search path to use for the format entry INFO, given
   FONT_P (whether the font override path applies), the compile-time
   default (DEFAULT_PATH), and the environment variables to check (the
   remaining arguments, terminated with NULL).  We set the `path' and
   `path_source' members of INFO.  The `client_path' member must already
   be set upon entry.  */
   
#define EXPAND_DEFAULT(try_path, source_string)			\
if (try_path)							\
  {								\
    info->raw_path = try_path;					\
    info->path = kpse_expand_default (try_path, info->path);	\
    info->path_source = source_string;				\
  }

static void
init_path PVAR3C(kpse_format_info_type *, info,  boolean, font_p,
                 const_string, default_path,  ap)
{
  string env_name;
  string var = NULL;
  
  info->font_override_p = font_p;
  info->default_path = default_path;

  /* First envvar that's set to a nonempty value will exit the loop.  If
     none are set, we want the first cnf entry that matches.  Find the
     cnf entries simultaneously, to avoid having to go through envvar
     list twice -- because of the PVAR?C macro, that would mean having
     to create a str_list and then use it twice.  Yuck.  */
  while ((env_name = va_arg (ap, string)) != NULL)
    {
      if (!var)
        {
          string env_value = getenv (env_name);
          if (env_value && *env_value)
            var = env_name;
        }

      /* If we are initializing the cnf path, don't try to get any
         values from the cnf files; that's infinite loop time.  */
      if (!info->cnf_path && (!info->suffix || !STREQ (info->suffix, ".cnf")))
        info->cnf_path = kpse_cnf_get (env_name);
      
      if (var && info->cnf_path)
        break;
    }
  va_end (ap);
  
  /* Expand any extra :'s.  For each level (the font override path down
     through the compile-time default), an extra : should be replaced
     with the path at the next lower level.  For example, an extra : in
     a user-set envvar should be replaced with the path from the cnf
     file.  Things are complicated because none of the levels above the
     very bottom are guaranteed to exist.  */

  /* Assume we can reliably start with the compile-time default.  */
  info->path = info->raw_path = info->default_path;
  info->path_source = "compile-time paths.h";

  EXPAND_DEFAULT (info->cnf_path, "texmf.cnf");
  EXPAND_DEFAULT (info->client_path, "program config file");
  if (var)
    EXPAND_DEFAULT (getenv (var), concat (var, " environment variable"));
  if (info->font_override_p)
    EXPAND_DEFAULT (kpse_font_override_path, "font override variable");
  info->path = kpse_path_expand (info->path);
}}


/* The path spec we are defining, one element of the global array.  */
#define FMT_INFO kpse_format_info[format]

/* Call `init_path', including appending the trailing NULL to the envvar
   list. Also initialize the fields not needed in setting the path.
   Don't set `program_enabled' -- it'll be false via the compiler
   initialization, and programs may want to enable it beforehand.  */
#define INIT_FORMAT(ext, font_p, default_path, envs) \
  FMT_INFO.suffix = FMT_INFO.type = ext; \
  init_path (&FMT_INFO, font_p, default_path, envs, NULL)

/* A few file types allow for runtime generation by an external program.  */
#define INIT_MT(prog, args) \
  FMT_INFO.program = prog; FMT_INFO.program_args = args
#define MAKETEXPK_SPEC \
  "$KPATHSEA_DPI $MAKETEX_BASE_DPI $MAKETEX_MAG $MAKETEX_MODE"

/* Initialize everything for FORMAT.  */

const_string
kpse_init_format P1C(kpse_file_format_type, format)
{
  /* If we get called twice, don't redo all the work.  */
  if (FMT_INFO.path)
    return FMT_INFO.path;
    
  switch (format)
    { /* We could avoid this repetition by token pasting, but it doesn't
         seem worth it.  */
    case kpse_gf_format:
      INIT_FORMAT ("gf", true, DEFAULT_GFFONTS, KPSE_GF_ENVS);
      FMT_INFO.suffix_search_only = false;
      break;
    case kpse_pk_format:
      INIT_MT ("MakeTeXPK", MAKETEXPK_SPEC);
      INIT_FORMAT ("pk", true, DEFAULT_PKFONTS, KPSE_PK_ENVS);
      FMT_INFO.suffix_search_only = true;
      break;
    case kpse_any_glyph_format:
      INIT_MT ("MakeTeXPK", MAKETEXPK_SPEC);
      INIT_FORMAT ("bitmap", true, DEFAULT_GLYPHFONTS, KPSE_GLYPH_ENVS);
      FMT_INFO.suffix_search_only = true;
      break;
    case kpse_base_format:
      INIT_FORMAT (".base", false, DEFAULT_MFBASES, KPSE_BASE_ENVS);
      break;
    case kpse_bib_format:
      INIT_FORMAT (".bib", false, DEFAULT_BIBINPUTS, KPSE_BIB_ENVS);
      break;
    case kpse_bst_format:
      INIT_FORMAT (".bst", false, DEFAULT_BSTINPUTS, KPSE_BST_ENVS);
      break;
    case kpse_cnf_format:
      /* I admit this is ugly, but making another field just for
         this one file type seemed a waste.  We use this value in cnf.c
         as the filename to search for in the path.  */
      if (!FMT_INFO.program)
        FMT_INFO.program = "texmf.cnf";
      INIT_FORMAT (".cnf", false, DEFAULT_TEXMFCNF, KPSE_CNF_ENVS);
      break;
    case kpse_fmt_format:
      INIT_FORMAT (".fmt", false, DEFAULT_TEXFORMATS, KPSE_FMT_ENVS);
      break;
    case kpse_mem_format:
      INIT_FORMAT (".mem", false, DEFAULT_MPMEMS, KPSE_MEM_ENVS);
      break;
    case kpse_mf_format:
      INIT_MT ("MakeTeXMF", NULL);
      INIT_FORMAT (".mf", false, DEFAULT_MFINPUTS, KPSE_MF_ENVS);
      break;
    case kpse_mfpool_format:
      INIT_FORMAT (".pool", false, DEFAULT_MFPOOL, KPSE_MFPOOL_ENVS);
      break;
    case kpse_mp_format:
      INIT_FORMAT (".mp", false, DEFAULT_MPINPUTS, KPSE_MP_ENVS);
      break;
    case kpse_mppool_format:
      INIT_FORMAT (".pool", false, DEFAULT_MPPOOL, KPSE_MPPOOL_ENVS);
      break;
    case kpse_mpsupport_format:
      INIT_FORMAT (NULL, false, DEFAULT_MPSUPPORT, KPSE_MPSUPPORT_ENVS);
      FMT_INFO.type = "MetaPost/troff support";
      break;
    case kpse_tex_format:
      INIT_MT ("MakeTeXTeX", NULL);
      INIT_FORMAT (".tex", false, DEFAULT_TEXINPUTS, KPSE_TEX_ENVS);
      break;
    case kpse_texpool_format:
      INIT_FORMAT (".pool", false, DEFAULT_TEXPOOL, KPSE_TEXPOOL_ENVS);
      break;
    case kpse_pict_format:
      INIT_FORMAT (NULL, false, DEFAULT_TEXINPUTS, KPSE_PICT_ENVS);
      FMT_INFO.type = "graphic/figure";
      break;
    case kpse_tfm_format:
      INIT_MT ("MakeTeXTFM", NULL);
      INIT_FORMAT (".tfm", false, DEFAULT_TFMFONTS, KPSE_TFM_ENVS);
      FMT_INFO.suffix_search_only = true;
      break;
    case kpse_vf_format:
      INIT_FORMAT (".vf", false, DEFAULT_VFFONTS, KPSE_VF_ENVS);
      FMT_INFO.suffix_search_only = true;
      break;
    case kpse_dvips_config_format:
      INIT_FORMAT (NULL, false, DEFAULT_TEXCONFIG, KPSE_DVIPS_CONFIG_ENVS);
      FMT_INFO.type = "dvips config";
      break;
    case kpse_dvips_header_format:
      INIT_FORMAT (NULL, false, DEFAULT_DVIPSHEADERS, KPSE_DVIPS_HEADER_ENVS);
      FMT_INFO.type = "dvips header/type1 font";
      break;
    case kpse_troff_font_format:
      INIT_FORMAT (NULL, false, DEFAULT_TRFONTS, KPSE_TROFF_FONT_ENVS);
      FMT_INFO.type = "troff font";
      break;
    default:
      FATAL1 ("(kpathsea) init_path: Unknown format %d", format);
    }

#ifdef DEBUG
#define MAYBE_NULL(member) (FMT_INFO.member ? FMT_INFO.member : "(none)")

  /* Describe the monster we've created.  */
  if (KPSE_DEBUG_P (KPSE_DEBUG_PATHS))
    {
      DEBUGF2 ("Search path for %s files (from %s)\n",
              FMT_INFO.type, FMT_INFO.path_source);
      DEBUGF1 ("  = %s\n", FMT_INFO.path);
      DEBUGF1 ("  before expansion = %s\n", FMT_INFO.raw_path);
      DEBUGF1 ("  font override var applies = %d\n", FMT_INFO.font_override_p);
      DEBUGF1 ("  application config file path = %s\n",
              MAYBE_NULL (client_path));
      DEBUGF1 ("  texmf.cnf path = %s\n", MAYBE_NULL (cnf_path));
      DEBUGF1 ("  compile-time path = %s\n", MAYBE_NULL (default_path));
      DEBUGF1 ("  suffix = %s\n", MAYBE_NULL (suffix));
      DEBUGF1 ("  search only with suffix = %d\n",
               FMT_INFO.suffix_search_only);
      DEBUGF1 ("  runtime generation program = %s\n", MAYBE_NULL (program));
      DEBUGF1 ("  extra program args = %s\n", MAYBE_NULL (program_args));
      /* Don't print the `program_enabled_p' member, since it's likely
         always false (made true on a per-font basis), and hence it
         would just confuse matters.  */
    }
#endif /* DEBUG */

  return FMT_INFO.path;
}

/* Look up a file NAME of type FORMAT, and the given MUST_EXIST.  This
   initializes the path spec for FORMAT if it's the first lookup of that
   type.  Return the filename found, or NULL.  */
   
string
kpse_find_file P3C(const_string, name,  kpse_file_format_type, format,
                   boolean, must_exist)
{
  boolean name_has_suffix;
  const_string suffix;
  string ret = NULL;

  if (FMT_INFO.path == NULL)
    kpse_init_format (format);

  suffix = FMT_INFO.suffix;
  
  /* Does NAME already end in `.suffix'?  */
  if (suffix)
    { /* Don't do the strlen's at all if no SUFFIX.  */
      unsigned suffix_len = strlen (suffix);
      unsigned name_len = strlen (name);
      name_has_suffix = (name_len > suffix_len
                         && STREQ (suffix, name + name_len - suffix_len));
    }
  else
    name_has_suffix = false;
  
  if (suffix && !name_has_suffix)
    {
      /* Append `.suffix' and search for it.  If we're going to search
         for the original name if this fails, then set must_exist=false;
         otherwise, we'd be looking on the disk for foo.eps.tex.  */
      string full_name = concat (name, suffix);
      ret = kpse_path_search (FMT_INFO.path, full_name,
                             FMT_INFO.suffix_search_only ? must_exist : false);
      free (full_name);
    }

  /* For example, looking for foo.eps, first we look for foo.eps.tex,
     which presumably fails, then we want to look for foo.eps.  (But we
     want to find foo.tex before foo, so have to try with `suffix'
     first.)  Another example: tftopl passes in cmr10.tfm, but the
     drivers just pass in cmr10. To avoid pounding the disk for `cmr10',
     `suffix_search_only' for PK format is true. But then
     `name_has_suffix' must override, or tftopl would wind up doing
     no searches at all.  */
  if (!ret && (name_has_suffix || !FMT_INFO.suffix_search_only))
    ret = kpse_path_search (FMT_INFO.path, name, must_exist);

  return ret;
}
