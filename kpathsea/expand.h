/* expand.h: general expansion.

Copyright (C) 1993, 94, 96 Karl Berry.

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

#ifndef KPATHSEA_EXPAND_H
#define KPATHSEA_EXPAND_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Call kpse_var_expand and kpse_tilde_expand (in that order).  Result
   is always in fresh memory, even if no expansions were done.  */
extern KPSEDLL string kpse_expand P1H(const_string s);

/* Do brace expansion and call `kpse_expand' on each element of the
   result; return the final expansion (always in fresh memory, even if
   no expansions were done).  We don't call `kpse_expand_default'
   because there is a whole sequence of defaults to run through; see
   `kpse_init_format'.  */
extern KPSEDLL string kpse_brace_expand P1H(const_string path);

/* Do brace expansion and call `kpse_expand' on each argument of the
   result, then expand any `//' constructs.  The final expansion (always
   in fresh memory) is a path of all the existing directories that match
   the pattern. */
extern KPSEDLL string kpse_path_expand P1H(const_string path);

#endif /* not KPATHSEA_EXPAND_H */
