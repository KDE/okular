/* variable.h: Declare variable expander.

Copyright (C) 1993 Karl Berry.

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

#ifndef KPATHSEA_VARIABLE_H
#define KPATHSEA_VARIABLE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* Expand $VAR and ${VAR} references in SRC, returning the (always
   dynamically-allocated) result.  An unterminated ${ or any other
   character following $ produce error messages, and that part of SRC is
   ignored.  In the $VAR form, the variable name consists of consecutive
   letters, digits, and underscores.  In the ${VAR} form, the variable
   name consists of whatever is between the braces.  In any case,
   ``expansion'' just means calling `getenv'; if the variable is not
   set, the expansion is the empty string.  */
extern string kpse_var_expand P1H(const_string src);

#endif /* not KPATHSEA_VARIABLE_H */
