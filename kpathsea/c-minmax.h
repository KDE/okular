/* c-minmax.h: define INT_MIN, etc.  Assume a 32-bit machine if the
   values aren't defined.

Copyright (C) 1992, 93 Free Software Foundation, Inc.

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

#ifndef KPATHSEA_C_MINMAX_H
#define KPATHSEA_C_MINMAX_H

#include <kpathsea/c-limits.h>

/* Declared in <limits.h> on ANSI C systems.  If the system doesn't
   define it, we use the minimum ANSI values -- except for `int'; we
   assume 32-bit integers.  */

#ifndef SCHAR_MIN
#define SCHAR_MIN (-127)
#endif
#ifndef SCHAR_MAX
#define SCHAR_MAX 128
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif

#ifndef SHRT_MIN
#define SHRT_MIN (-32767)
#endif
#ifndef SHRT_MAX
#define SHRT_MAX 32767
#endif
#ifndef USHRT_MAX
#define USHRT_MAX 65535
#endif

#ifndef INT_MIN
#define INT_MIN (-2147483647)
#endif
#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#ifndef UINT_MAX
#define UINT_MAX 4294967295
#endif

#ifndef LONG_MIN
#define LONG_MIN INT_MIN
#endif
#ifndef LONG_MAX
#define LONG_MAX INT_MAX
#endif
#ifndef ULONG_MAX
#define ULONG_MAX UINT_MAX
#endif

/* Declared in <float.h> on ANSI C systems.  */
#ifndef DBL_MIN
#define DBL_MIN 1e-37
#endif
#ifndef DBL_MAX
#define DBL_MAX 1e+37
#endif

#ifndef FLT_MIN
#define FLT_MIN 1e-37
#endif
#ifndef FLT_MAX
#define FLT_MAX 1e+37
#endif

#endif /* not KPATHSEA_C_MINMAX_H */
