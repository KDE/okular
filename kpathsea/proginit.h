/* proginit.h: declarations for DVI driver initializations.

Copyright (C) 1994, 95 Karl Berry.

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

#ifndef KPATHSEA_PROGINIT_H
#define KPATHSEA_PROGINIT_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* Common initializations for DVI drivers -- check for `PREFIX'SIZES and
   `PREFIX'FONTS environment variables, setenv MAKETEX_MODE to MODE,
   etc., etc.  See the source.  */

extern void
kpse_init_prog P5H(const_string prefix,  unsigned dpi,  const_string mode,
                   boolean make_tex_pk,  const_string fallback);

#endif /* not KPATHSEA_PROGINIT_H */
