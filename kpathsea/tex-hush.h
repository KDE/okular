/* tex-hush.h: suppressing warnings?

Copyright (C) 1996 Karl Berry.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef KPATHSEA_TEX_HUSH_H
#define KPATHSEA_TEX_HUSH_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Return true if WHAT is included in the TEX_HUSH environment
   variable/config value.  */
extern KPSEDLL boolean kpse_tex_hush P1H(const_string what);

#endif /* not KPATHSEA_TEX_HUSH_H */
