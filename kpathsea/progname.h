/* progname.h: Declarations for argv[0] equivalents.

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

#ifndef KPATHSEA_PROGNAME_H
#define KPATHSEA_PROGNAME_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

extern string program_invocation_name;
extern string program_invocation_short_name;


/* Set the two variables above (if they're not predefined) to a copy of
   ARGV0 and everything in ARGV0 after the last directory separator,
   respectively.  */

extern void kpse_set_progname P1H(const_string argv0);

#endif /* not KPATHSEA_PROGNAME_H */
