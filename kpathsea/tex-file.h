/* tex-file.h: find files in a particular format.

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

#ifndef KPATHSEA_TEX_FILE_H
#define KPATHSEA_TEX_FILE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* If non-NULL, try looking for this if can't find the real font.  */
extern const_string kpse_fallback_font;


/* If non-NULL, check these if can't find (within a few percent of) the
   given resolution.  List must end with a zero element.  */
extern unsigned *kpse_fallback_resolutions;

/* This initializes the fallback resolution list.  If ENVVAR
   is set, it is used; otherwise, the envvar `TEXSIZES' is looked at; if
   that's not set either, a compile-time default is used.  */
extern void kpse_init_fallback_resolutions P1H(string envvar);


/* If non-null, used instead of the usual envvar/path defaults, e.g.,
   set to `getenv ("XDVIFONTS")'.  */
extern string kpse_font_override_path;

/* We put the glyphs first so we don't waste space in an array.  A new
   format here must be accompanied by a new initialization
   abbreviation below and a new entry in `tex-make.c'.  */
typedef enum
{
  kpse_gf_format,
  kpse_pk_format,
  kpse_any_glyph_format,	/* ``any'' meaning anything above */
  kpse_base_format, 
  kpse_bib_format, 
  kpse_bst_format, 
  kpse_cnf_format,
  kpse_fmt_format,
  kpse_mem_format,
  kpse_mf_format, 
  kpse_mfpool_format, 
  kpse_mp_format,
  kpse_mppool_format,
  kpse_mpsupport_format,
  kpse_pict_format,
  kpse_tex_format,
  kpse_texpool_format,
  kpse_tfm_format, 
  kpse_vf_format,
  kpse_dvips_config_format,
  kpse_dvips_header_format,
  kpse_troff_font_format,
  kpse_last_format /* one past last index */
} kpse_file_format_type;


/* For each file format, we record the following information.  The main
   thing that is not part of this structure is the environment variable
   lists above. They are used directly in tex-file.c. We could
   incorporate them here, but it would complicate the code a bit. We
   could also do it via variable expansion, but not now, maybe not ever:
   ${PKFONTS-${TEXFONTS-/usr/local/lib/texmf/fonts//}}.  */

typedef struct
{
  const_string type;		/* Human-readable description.  */
  const_string path;		/* The search path to use.  */
  const_string raw_path;	/* Pre-$~ (but post-default) expansion.  */
  const_string path_source;	/* Where the path started from.  */
  boolean font_override_p;	/* Use kpse_font_override_path?  */
  const_string client_path;	/* E.g., from dvips's config.ps.  */
  const_string cnf_path;	/* From our texmf.cnf.  */
  const_string default_path;	/* If all else fails.  */
  const_string suffix;		/* For kpse_find_file to append, or NULL.  */
  boolean suffix_search_only;	/* Only search if the suffix is present?  */
  const_string program;		/* ``MakeTeXPK'', etc.  */
  const_string program_args;	/* Args to the `program'.  */
  boolean program_enabled_p;	/* Invoke the `program'?  */
} kpse_format_info_type;

/* The sole variable of that type, indexed by `kpse_file_format_type'.
   Initialized by calls to `kpse_find_file' for `kpse_init_format'.  */
extern kpse_format_info_type kpse_format_info[];


/* Initialize the info for the given format.  This is called
   automatically by `kpse_find_file', but the glyph searching (for
   example) can't use that function, so make it available.  */
extern const_string kpse_init_format P1H(kpse_file_format_type);

/* If FORMAT has a non-null `suffix' member, concatenate NAME "." and it
   and call `kpse_path_search' with the result and the other arguments.
   If that fails, try just NAME.  */
extern string kpse_find_file P3H(const_string name,  
                            kpse_file_format_type format,  boolean must_exist);

/* Here are some abbreviations.  */
#define kpse_find_pict(name) kpse_find_file (name, kpse_pict_format, true)
#define kpse_find_tex(name)  kpse_find_file (name, kpse_tex_format, true)
#define kpse_find_tfm(name)  kpse_find_file (name, kpse_tfm_format, true)

/* The `false' is correct for DVI translators, which should clearly not
   require vf files for every font (e.g., cmr10.vf).  But it's wrong for
   VF translators, such as vftovp.  */
#define kpse_find_vf(name) kpse_find_file (name, kpse_vf_format, false)

#endif /* not KPATHSEA_TEX_FILE_H */
