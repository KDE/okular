/* c-ctype.h: ASCII-safe versions of the <ctype.h> macros.

Copyright (C) 1992 Free Software Foundation, Inc.

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

#ifndef KPATHSEA_C_CTYPE_H
#define KPATHSEA_C_CTYPE_H

#include <ctype.h>

/* Be sure we have `isascii'.  */
#ifndef isascii
#define isascii(c) 1
#endif

#define ISALNUM(c) (isascii (c) && isalnum(c))
#define ISALPHA(c) (isascii (c) && isalpha(c))
#define ISASCII isascii
#define ISCNTRL(c) (isascii (c) && iscntrl(c))
#define ISDIGIT(c) (isascii (c) && isdigit (c))
#define ISGRAPH(c) (isascii (c) && isgraph(c))
#define ISLOWER(c) (isascii (c) && islower(c))
#define ISPRINT(c) (isascii (c) && isprint(c))
#define ISPUNCT(c) (isascii (c) && ispunct(c))
#define ISSPACE(c) (isascii (c) && isspace(c))
#define ISUPPER(c) (isascii (c) && isupper(c))
#define ISXDIGIT(c) (isascii (c) && isxdigit(c))
#define TOASCII toascii
#define TOLOWER(c) (ISUPPER (c) ? tolower (c) : (c))
#define TOUPPER(c) (ISLOWER (c) ? toupper (c) : (c))

/* This isn't part of the usual <ctype.h>, but it's useful sometimes.  */
#ifndef isblank
#define isblank(c) ((c) == ' ' || (c) == '\t')
#endif


/* Here's why this mess is necessary:

From: meyering@cs.utexas.edu (Jim Meyering)
Date: Wed, 25 Nov 1992 09:52:33 -0600
Subject: ss-921123: using isascii with <ctype.h> macros

  Yesterday some cursory regression testing found that GNU od
  (in an upcoming release of textutils) generated incorrect output
  when run on an SGI indigo because isprint ('\377') returned true.
  Of course, '\377' is not a printing character;  the problem lay
  in using isprint without first making sure its integer argument
  corresponded to an ascii code.

  MORAL: always guard uses of ctype macros with isascii if it's available.
  An obvious alternative is to avoid <ctype.h> and define and use your
  own versions of the ctype macros.

  A pretty clean approach to using <ctype.h> and isascii was
  suggested by David MacKenzie:

  #ifndef isascii
  #define isascii(c) 1
  #endif

  #define ISDIGIT(c) (isascii (c) && isdigit (c))
  #define ISPRINT(c) (isascii (c) && isprint (c))
  ...

  then, use ISDIGIT, etc. instead of isdigit, etc.  */
  
#endif /* not KPATHSEA_C_CTYPE_H */
