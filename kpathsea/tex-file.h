/* tex-file.h: find files in a particular format.

Copyright (C) 1993, 94, 95, 96 Karl Berry.
Copyright 1998, 1999 Olaf Weber.

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

#ifndef KPATHSEA_TEX_FILE_H
#define KPATHSEA_TEX_FILE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* If non-NULL, try looking for this if can't find the real font.  */
extern const_string kpse_fallback_font;


/* If non-NULL, default list of fallback resolutions comes from this
   instead of the compile-time value.  Set by dvipsk for the R config
   cmd.  *SIZES environment variables override/use as default.  */
extern KPSEDLL const_string kpse_fallback_resolutions_string;

/* If non-NULL, check these if can't find (within a few percent of) the
   given resolution.  List must end with a zero element.  */
extern unsigned *kpse_fallback_resolutions;

/* This initializes the fallback resolution list.  If ENVVAR
   is set, it is used; otherwise, the envvar `TEXSIZES' is looked at; if
   that's not set either, a compile-time default is used.  */
extern void kpse_init_fallback_resolutions P1H(string envvar);

/* We put the glyphs first so we don't waste space in an array in
   tex-glyph.c.  Accompany a new format here with appropriate changes in
   tex-file.c and kpsewhich.c (the suffix variable).  */
typedef enum
{
  kpse_gf_format,
  kpse_pk_format,
  kpse_any_glyph_format,	/* ``any'' meaning anything above */
  kpse_tfm_format, 
  kpse_afm_format, 
  kpse_base_format, 
  kpse_bib_format, 
  kpse_bst_format, 
  kpse_cnf_format,
  kpse_db_format,
  kpse_fmt_format,
  kpse_fontmap_format,
  kpse_mem_format,
  kpse_mf_format, 
  kpse_mfpool_format, 
  kpse_mft_format, 
  kpse_mp_format, 
  kpse_mppool_format, 
  kpse_mpsupport_format,
  kpse_ocp_format,
  kpse_ofm_format, 
  kpse_opl_format,
  kpse_otp_format,
  kpse_ovf_format,
  kpse_ovp_format,
  kpse_pict_format,
  kpse_tex_format,
  kpse_texdoc_format,
  kpse_texpool_format,
  kpse_texsource_format,
  kpse_tex_ps_header_format,
  kpse_troff_font_format,
  kpse_type1_format, 
  kpse_vf_format,
  kpse_dvips_config_format,
  kpse_ist_format,
  kpse_truetype_format,
  kpse_type42_format,
  kpse_web2c_format,
  kpse_program_text_format,
  kpse_program_binary_format,
  kpse_miscfonts_format,
  kpse_last_format /* one past last index */
} kpse_file_format_type;


/* Perhaps we could use this for path values themselves; for now, we use
   it only for the program_enabled_p value.  */
typedef enum
{
  kpse_src_implicit,   /* C initialization to zero */
  kpse_src_compile,    /* configure/compile-time default */
  kpse_src_texmf_cnf,  /* texmf.cnf, the kpathsea config file */
  kpse_src_client_cnf, /* application config file, e.g., config.ps */
  kpse_src_env,        /* environment variable */
  kpse_src_x,          /* X Window System resource */
  kpse_src_cmdline     /* command-line option */
} kpse_src_type;


/* For each file format, we record the following information.  The main
   thing that is not part of this structure is the environment variable
   lists. They are used directly in tex-file.c. We could incorporate
   them here, but it would complicate the code a bit. We could also do
   it via variable expansion, but not now, maybe not ever:
   ${PKFONTS-${TEXFONTS-/usr/local/lib/texmf/fonts//}}.  */

typedef struct
{
  const_string type;		/* Human-readable description.  */
  const_string path;		/* The search path to use.  */
  const_string raw_path;	/* Pre-$~ (but post-default) expansion.  */
  const_string path_source;	/* Where the path started from.  */
  const_string override_path;	/* From client environment variable.  */
  const_string client_path;	/* E.g., from dvips's config.ps.  */
  const_string cnf_path;	/* From texmf.cnf.  */
  const_string default_path;	/* If all else fails.  */
  const_string *suffix;		/* For kpse_find_file to check for/append.  */
  const_string *alt_suffix;	/* More suffixes to check for.  */
  boolean suffix_search_only;	/* Only search with a suffix?  */
  const_string program;		/* ``mktexpk'', etc.  */
  const_string program_args;	/* Args to `program'.  */
  boolean program_enabled_p;	/* Invoke `program'?  */
  kpse_src_type program_enable_level; /* Who said to invoke `program'.  */
  boolean binmode;              /* The files must be opened in binary mode. */
} kpse_format_info_type;

/* The sole variable of that type, indexed by `kpse_file_format_type'.
   Initialized by calls to `kpse_find_file' for `kpse_init_format'.  */
extern KPSEDLL kpse_format_info_type kpse_format_info[kpse_last_format];


/* If LEVEL is higher than `program_enabled_level' for FMT, set
   `program_enabled_p' to VALUE.  */
extern KPSEDLL void kpse_set_program_enabled P3H(kpse_file_format_type fmt,
                                         boolean value, kpse_src_type level);
/* Call kpse_set_program_enabled with VALUE and the format corresponding
   to FMTNAME.  */
extern KPSEDLL void kpse_maketex_option P2H(const_string fmtname,  boolean value);

/* Initialize the info for the given format.  This is called
   automatically by `kpse_find_file', but the glyph searching (for
   example) can't use that function, so make it available.  */
extern KPSEDLL const_string kpse_init_format P1H(kpse_file_format_type);

/* If FORMAT has a non-null `suffix' member, append it to NAME "."
   and call `kpse_path_search' with the result and the other arguments.
   If that fails, try just NAME.  */
extern KPSEDLL string kpse_find_file P3H(const_string name,  
                            kpse_file_format_type format,  boolean must_exist);

/* Here are some abbreviations.  */
#define kpse_find_mf(name)   kpse_find_file (name, kpse_mf_format, true)
#define kpse_find_mft(name)  kpse_find_file (name, kpse_mft_format, true)
#define kpse_find_pict(name) kpse_find_file (name, kpse_pict_format, true)
#define kpse_find_tex(name)  kpse_find_file (name, kpse_tex_format, true)
#define kpse_find_tfm(name)  kpse_find_file (name, kpse_tfm_format, true)
#define kpse_find_ofm(name)  kpse_find_file (name, kpse_ofm_format, true)

/* The `false' is correct for DVI translators, which should clearly not
   require vf files for every font (e.g., cmr10.vf).  But it's wrong for
   VF translators, such as vftovp.  */
#define kpse_find_vf(name) kpse_find_file (name, kpse_vf_format, false)
#define kpse_find_ovf(name) kpse_find_file (name, kpse_ovf_format, false)

/* Don't just look up the name, actually open the file.  */
extern KPSEDLL FILE *kpse_open_file P2H(const_string, kpse_file_format_type);

/* This function is used to set kpse_program_name (from progname.c) to
   a different value.  It will clear the path searching information, to
   ensure that the search paths are appropriate to the new name. */

extern KPSEDLL void kpse_reset_program_name P1H(const_string progname);

#endif /* not KPATHSEA_TEX_FILE_H */
