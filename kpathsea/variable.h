/* variable.h: Declare variable expander.

Copyright (C) 1993, 95 Karl Berry.

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

#ifndef KPATHSEA_VARIABLE_H
#define KPATHSEA_VARIABLE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>


/* Return the (variable-expanded) environment variable value or config
   file value, or NULL.  */
extern KPSEDLL string kpse_var_value P1H(const_string var);

/* Expand $VAR and ${VAR} references in SRC, returning the (always newly
   dynamically-allocated) result.  An unterminated ${ or any other
   character following $ produce error messages, and that part of SRC is
   ignored.  In the $VAR form, the variable name consists of consecutive
   letters, digits, and underscores.  In the ${VAR} form, the variable
   name consists of whatever is between the braces.
   
   In any case, ``expansion'' means calling `getenv'; if the variable is not
   set, look in texmf.cnf files for a definition.  If not set there, either,
   the expansion is the empty string (no error).  */
extern KPSEDLL string kpse_var_expand P1H(const_string src);

#endif /* not KPATHSEA_VARIABLE_H */
