/* proginit.c: useful initializations for DVI drivers.

Copyright (C) 1994, 95, 96 Karl Berry.

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
#include <kpathsea/proginit.h>
#include <kpathsea/tex-file.h>


/* These initializations were common to all the drivers modified for
   kpathsea, so a single routine seemed in order.  Kind of a bollixed-up
   mess, but still better than repeating the code.  */

void
kpse_init_prog P4C(const_string, prefix,  unsigned, dpi,  const_string, mode,
                   const_string, fallback)
{
  string font_var = concat (prefix, "FONTS");
  string header_var = concat (prefix, "HEADERS");
  string makepk_var = concat (prefix, "MAKEPK");
  string size_var = concat (prefix, "SIZES");
  
  /* Do both `pk_format' and `any_glyph_format' for the sake of xdvi; in
     general, mktexpk might apply to either, and the program will ask
     for the one it wants.  */
     
  /* Might have a program-specific name for mktexpk itself.  */
  if (getenv (makepk_var)) {
  /* If we did, we want to enable the program, I think.  */
    kpse_set_program_enabled (kpse_pk_format, 1, kpse_src_env);
    kpse_set_program_enabled (kpse_any_glyph_format, 1, kpse_src_env);

    kpse_format_info[kpse_pk_format].program
      = kpse_format_info[kpse_any_glyph_format].program
      = getenv (makepk_var);
  }

  /* A couple font paths have traditionally had application-specific
     environment variables to override all else; namely, XDVIFONTS and
     DVIPSHEADERS.  So set those if we have them.  */
  kpse_format_info[kpse_pk_format].override_path
    = kpse_format_info[kpse_gf_format].override_path
    = kpse_format_info[kpse_any_glyph_format].override_path
    = kpse_format_info[kpse_tfm_format].override_path
    = getenv (font_var);

  kpse_format_info[kpse_tex_ps_header_format].override_path
    = getenv (header_var);

  kpse_init_fallback_resolutions (size_var);
  xputenv_int ("MAKETEX_BASE_DPI", dpi);
  kpse_fallback_font = fallback;
  
  /* Ugliness.  See comments in kpse_make_tex in kpathsea/tex-make.c.  */
  xputenv ("MAKETEX_MODE", mode ? mode : DIR_SEP_STRING);
  
  free (font_var);
  free (header_var);
  free (makepk_var);
  free (size_var);
}
