/* tilde.h: Declare tilde expander.

Copyright (C) 1993 Karl Berry.

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

#ifndef KPATHSEA_TILDE_H
#define KPATHSEA_TILDE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* Replace a leading ~ or ~name in FILENAME with getenv ("HOME") or
   name's home directory, respectively.  FILENAME may not be null.  */

extern string kpse_tilde_expand P1H(const_string filename);

#endif /* not KPATHSEA_TILDE_H */
