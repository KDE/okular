/* c-memstr.h: memcpy, strchr, etc.

Copyright (C) 1992, 93, 94 Free Software Foundation, Inc.

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

#ifndef KPATHSEA_C_MEMSTR_H
#define KPATHSEA_C_MEMSTR_H

/* <X11/Xfuncs.h> tries to declare bcopy etc., which can only conflict.  */
#define _XFUNCS_H_

/* Just to be complete, we make both the system V/ANSI and the BSD
   versions of the string functions available.  */

#if (defined (STDC_HEADERS) || defined (HAVE_STRING_H))
#if 0
#define SYSV /* so <X11/Xos.h> knows not to include <strings.h> */
#endif

#include <string.h>

/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
#if !defined (STDC_HEADERS) && defined (HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */

/* Do not define these if we are not STDC_HEADERS, because in that
   case X11/Xos.h defines `strchr' to be `index'. */
#ifdef STDC_HEADERS
/* Let's hope that if index/rindex are defined, they're defined to the
   right thing.  */
#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif
#endif /* STDC_HEADERS */

#ifndef HAVE_BCOPY
#ifndef bcmp
#define bcmp(s1, s2, len) memcmp ((s1), (s2), (len))
#endif
#ifndef bcopy
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif
#ifndef bzero
#define bzero(s, len) memset ((s), 0, (len))
#endif
#endif /* not HAVE_BCOPY */

#else /* not (STDC_HEADERS or HAVE_STRING_H) */

#include <strings.h>

#ifndef strchr
#define strchr index
#endif
#ifndef strrchr
#define strrchr rindex
#endif

#ifndef memcmp
#define memcmp(s1, s2, n) bcmp ((s1), (s2), (n))
#endif
#ifndef memcpy
#define memcpy(to, from, len) bcopy ((from), (to), (len))
#endif

extern char *strtok ();
extern char *strstr ();

#endif /* not (STDC_HEADERS or HAVE_STRING_H) */

#endif /* not KPATHSEA_C_MEMSTR_H */
