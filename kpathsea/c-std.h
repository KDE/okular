/* c-std.h: the first header files.

Copyright (C) 1992, 93, 94, 95, 96, 97 Free Software Foundation, Inc.

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

#ifndef KPATHSEA_C_STD_H
#define KPATHSEA_C_STD_H

/* Header files that essentially all of our sources need, and
   that all implementations have.  We include these first, to help with
   NULL being defined multiple times.  */
/* Workaround against a <math.h> MSVC bug : it can't be declared 
   extern "C" in a c++ source file. */
#ifndef __cplusplus
#include <math.h>
#endif
#include <stdio.h>

/* Be sure we have constants from <unistd.h>.  */
#include <kpathsea/c-unistd.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
/* Include <stdlib.h> before <stddef.h>, to help avoid NULL
   redefinitions on some systems.  (We don't include <stddef.h>
   ourselves any more, but FYI.)  */
#else
/* It's impossible to say for sure what the system will deign to put in
   <stdlib.h>, but let's hope it's at least this.  */
extern char *getenv ();
#endif /* not HAVE_STDLIB_H */

#ifdef WIN32
#include <malloc.h>
#else
#ifndef STDC_HEADERS
#ifndef ALLOC_RETURN_TYPE
#ifdef DOSISH
#define ALLOC_RETURN_TYPE void
#else
#define ALLOC_RETURN_TYPE char
#endif /* not DOSISH */
#endif /* not ALLOC_RETURN_TYPE */
extern ALLOC_RETURN_TYPE *calloc (), *malloc (), *realloc ();
#endif /* not STDC_HEADERS */
#endif /* not WIN32 */

/* SunOS 4.1.1 gets STDC_HEADERS defined, but it doesn't provide
   EXIT_FAILURE.  So far no system has defined one of EXIT_FAILURE and
   EXIT_SUCCESS without the other.  */
#ifndef EXIT_SUCCESS
#ifdef VMS
#define EXIT_SUCCESS 1
#define EXIT_FAILURE 0
#else
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif
#endif /* not EXIT_SUCCESS */

/* strchr vs. index, memcpy vs. bcopy, etc.  */
#include <kpathsea/c-memstr.h>

/* Error numbers and errno declaration.  */
#include <kpathsea/c-errno.h>

/* Numeric minima and maxima.  */
#include <kpathsea/c-minmax.h>

/* Assertions are too useful to not make generally available.  */
#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(expr) /* as nothing */
#endif

#ifdef VMS
#include <unixlib.h>
#include <unixio.h>
#else
#ifndef WIN32
/* `popen' and `pclose' are part of POSIX.2, not POSIX.1.  So
   STDC_HEADERS isn't enough.  */
extern FILE *popen ();
extern int pclose ();
#endif /* not WIN32 */
#endif /* not VMS */

#endif /* not KPATHSEA_C_STD_H */
