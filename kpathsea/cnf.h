/* cnf.h: runtime config files.

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

#ifndef KPATHSEA_CNF_H
#define KPATHSEA_CNF_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Return the value in the last-read cnf file for VAR, or NULL if none.
   On the first call, also read all the cnf files in the path (and
   initialize the path) named the `program' member of the
   `kpse_cnf_format' element of `kpse_format_info'.  */
extern string kpse_cnf_get P1H(const_string var);

#endif /* not KPATHSEA_CNF_H */
