/* c-systypes.h: include <sys/types.h>.  It's too bad we need this file,
   but some systems don't protect <sys/types.h> from multiple
   inclusions, and I'm not willing to put up with that.

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

#ifndef C_SYSTYPES_H
#define C_SYSTYPES_H

#include <sys/types.h>

/* This is the symbol that X uses to determine if <sys/types.h> has been
   read, so we define it.  */
#define __TYPES__

#endif /* not C_SYSTYPES_H */
