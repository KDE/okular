/* c-pathch.h: define the characters which separate components of
   filenames and environment variable paths.

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

#ifndef C_PATHCH_H
#define C_PATHCH_H

/* What separates filename components?  */
#ifndef DIR_SEP
#ifdef VMS
#define DIR_SEP ':'
#define DIR_SEP_STRING ":"
#else
#ifdef DOS
#define DIR_SEP '\\'
#define DIR_SEP_STRING "\\"
/* On DOS, it's good to allow both \ and / between directories.  */
#define IS_DIR_SEP(ch) ((ch) == DIR_SEP || (ch) == '/')
#else
#ifdef VMCMS
#define DIR_SEP ' '
#define DIR_SEP_STRING " "
#else
#define DIR_SEP '/'
#define DIR_SEP_STRING "/"
#endif /* not VM/CMS */
#endif /* not DOS */
#endif /* not VMS */
#endif /* not PATH_SEP */

#ifndef IS_DIR_SEP
#define IS_DIR_SEP(ch) ((ch) == DIR_SEP)
#endif


/* What separates elements in environment variable path lists?  */
#ifndef ENV_SEP
#ifdef VMS
#define ENV_SEP ','
#define ENV_SEP_STRING ","
#else
#ifdef DOS
#define ENV_SEP ';'
#define ENV_SEP_STRING ";"
#else
#ifdef VMCMS
#define ENV_SEP ' '
#define ENV_SEP_STRING " "
#else
#define ENV_SEP ':'
#define ENV_SEP_STRING ":"
#endif /* not VM/CMS */
#endif /* not DOS */
#endif /* not VMS */
#endif /* not ENV_SEP */

#ifndef IS_ENV_SEP
#define IS_ENV_SEP(ch) ((ch) == ENV_SEP)
#endif

#endif /* not C_PATHCH_H */
