/* c-pathch.h: define the characters which separate components of
   filenames and environment variable paths.

Copyright (C) 1992, 93, 95, 97 Free Software Foundation, Inc.

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

#ifndef C_PATHCH_H
#define C_PATHCH_H

#include <kpathsea/c-ctype.h>

/* What separates filename components?  */
#ifndef DIR_SEP
#ifdef VMS
#define DIR_SEP ':'
#define DIR_SEP_STRING ":"
#else
#ifdef DOSISH
/* Either \'s or 's work.  Wayne Sullivan's web2pc prefers /, so we'll
   go with that.  */
#define DIR_SEP '/'
#define DIR_SEP_STRING "/"
#define IS_DEVICE_SEP(ch) ((ch) == ':')
#define NAME_BEGINS_WITH_DEVICE(name) (*(name) && IS_DEVICE_SEP((name)[1]))
/* On DOS, it's good to allow both \ and / between directories.  */
#define IS_DIR_SEP(ch) ((ch) == '/' || (ch) == '\\')
/* On win32, UNC names are authorized */
#ifdef WIN32
#define IS_UNC_NAME(name) (strlen(name)>=3 && IS_DIR_SEP(*name)  \
                            && IS_DIR_SEP(*(name+1)) && isalpha(*(name+2)))
#endif
#else
#ifdef AMIGA
#define DIR_SEP '/'
#define DIR_SEP_STRING "/"
#define IS_DIR_SEP(ch) ((ch) == '/' || (ch) == ':')
#define IS_DEVICE_SEP(ch) ((ch) == ':')
#else
#ifdef VMCMS
#define DIR_SEP ' '
#define DIR_SEP_STRING " "
#else
#define DIR_SEP '/'
#define DIR_SEP_STRING "/"
#endif /* not VM/CMS */
#endif /* not AMIGA */
#endif /* not DOSISH */
#endif /* not VMS */
#endif /* not DIR_SEP */

#ifndef IS_DIR_SEP
#define IS_DIR_SEP(ch) ((ch) == DIR_SEP)
#endif
#ifndef IS_DEVICE_SEP /* No `devices' on, e.g., Unix.  */
#define IS_DEVICE_SEP(ch) 0 
#endif
#ifndef NAME_BEGINS_WITH_DEVICE
#define NAME_BEGINS_WITH_DEVICE(name) 0 
#endif


/* What separates elements in environment variable path lists?  */
#ifndef ENV_SEP
#ifdef VMS
#define ENV_SEP ','
#define ENV_SEP_STRING ","
#else
#ifdef DOSISH
#define ENV_SEP ';'
#define ENV_SEP_STRING ";"
#else
#ifdef AMIGA
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
#endif /* not AMIGA */
#endif /* not DOS */
#endif /* not VMS */
#endif /* not ENV_SEP */

#ifndef IS_ENV_SEP
#define IS_ENV_SEP(ch) ((ch) == ENV_SEP)
#endif

#endif /* not C_PATHCH_H */
