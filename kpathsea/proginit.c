/* proginit.c: useful initializations for DVI drivers.

Copyright (C) 1994 Karl Berry.

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
#include <kpathsea/proginit.h>
#include <kpathsea/progname.h>
#include <kpathsea/tex-file.h>


/* These initializations were common to all the drivers modified for
   kpathsea, so a single routine seemed in order.  Kind of a bollixed-up
   mess, but still better than repeating the code.  */

void
kpse_init_prog P5C(const_string, prefix,  unsigned, dpi,  const_string, mode,
                   boolean, make_tex_pk,  const_string, fallback)
{
  string makepk_var = concat (prefix, "MAKEPK");
  string font_var = concat (prefix, "FONTS");
  string size_var = concat (prefix, "SIZES");
  
  /* Do both `pk_format' and `any_glyph_format' for the sake of xdvi; in
     general, MakeTeXPK might apply to either, and the program will ask
     for the one it wants.  */
     
  /* Might have a program-specific name for MakeTeXPK itself.  */
  kpse_format_info[kpse_pk_format].program
    = kpse_format_info[kpse_any_glyph_format].program
    = getenv (makepk_var);
  
  /* If we did, we want to enable the program, I think.  */
  kpse_format_info[kpse_pk_format].program_enabled_p
    = kpse_format_info[kpse_any_glyph_format].program_enabled_p
    = getenv (makepk_var) || make_tex_pk;

  kpse_font_override_path = getenv (font_var);
  kpse_init_fallback_resolutions (size_var);
  xputenv_int ("MAKETEX_BASE_DPI", dpi);
  kpse_fallback_font = fallback;
  
  /* See comments in kpse_make_tex in kpathsea/tex-make.c.  */
  xputenv ("MAKETEX_MODE", mode ? mode : DIR_SEP_STRING);
  
  free (makepk_var);
  free (font_var);
  free (size_var);
}
