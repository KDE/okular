/* c-limits.h: include the system parameter file.

Copyright (C) 1992, 93, 96 Free Software Foundation, Inc.

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

#ifndef C_LIMITS_H
#define C_LIMITS_H

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#include <kpathsea/systypes.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#endif /* not HAVE_LIMITS_H */

/* Some systems may have the floating-point limits in the above.  */
#if defined (HAVE_FLOAT_H) && !defined (FLT_MAX)
#include <float.h>
#endif

#endif /* not C_LIMITS_H */
