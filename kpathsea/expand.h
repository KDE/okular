/* expand.h: general expansion.

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

#ifndef KPATHSEA_EXPAND_H
#define KPATHSEA_EXPAND_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Call kpse_var_expand and kpse_tilde_expand (in that order).  Result
   is always in fresh memory, even if no expansions were done.  */
extern string kpse_expand P1H(const_string s);

/* Call `kpse_expand' on each element of the result; return the final
   expansion (always in fresh memory, even if no expansions were
   done).  We don't call `kpse_expand_default' because there is a whole
   sequence of defaults to run through; see `kpse_init_format'.  */
extern string kpse_path_expand P1H(const_string path);

#endif /* not KPATHSEA_EXPAND_H */
