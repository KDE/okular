/* c-fopen.h: how to open files with fopen.

Copyright (C) 1992, 94, 95, 96 Free Software Foundation, Inc.

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

#ifndef C_FOPEN_H
#define C_FOPEN_H

/* How to open a text file:  */
#ifndef FOPEN_A_MODE
#define FOPEN_A_MODE "a"
#endif

#ifndef FOPEN_R_MODE
#define FOPEN_R_MODE "r"
#endif

#ifndef FOPEN_W_MODE
#define FOPEN_W_MODE "w"
#endif

/* How to open a binary file for reading:  */
#ifndef FOPEN_RBIN_MODE
#if defined (VMS) || defined (VMCMS) || defined(DOS) || defined (OS2) || defined (WIN32) || defined (__DJGPP__) || defined (__CYGWIN32__)
#define	FOPEN_RBIN_MODE	"rb"
#else
#define	FOPEN_RBIN_MODE	"r"
#endif /* not (VM/CMS or VMS or DOS or OS2 or WIN32 or __DJGPP__ or __CYGWIN32__) */
#endif /* not FOPEN_RBIN_MODE */

/* How to open a binary file for writing:  */
#ifndef FOPEN_WBIN_MODE
#if defined (DOS) || defined (OS2) || defined (WIN32) || defined (__DJGPP__) || defined (__CYGWIN32__)
#define FOPEN_WBIN_MODE "wb"
#else
#ifdef VMCMS
#define FOPEN_WBIN_MODE "wb, lrecl=1024, recfm=f"
#else
#define	FOPEN_WBIN_MODE	"w"
#endif /* not VM/CMS */
#endif /* not (DOS or OS2 or WIN32 or DJGPP or CYGWIN32) */
#endif /* not FOPEN_WBIN_MODE */

/* How to open a binary file for appending:  */
#ifndef FOPEN_ABIN_MODE
#if defined (DOS) || defined (OS2) || defined (WIN32) || defined (__DJGPP__) || defined (__CYGWIN32__)
#define FOPEN_ABIN_MODE "ab"
#else
#define FOPEN_ABIN_MODE "a"
#endif /* not (DOS or OS2 or WIN32 or DJGPP or CYGWIN32) */
#endif /* not FOPEN_ABIN_MODE */

/* How to switch an already open file handle to binary mode.
   Used on DOSISH systems when we need to switch a standard
   stream, such as stdin or stdout, to binary mode.  */
#include <fcntl.h>
#ifdef DOSISH
#include <io.h>
#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#endif
#endif
#if defined (__DJGPP__) || defined (WIN32) || defined (__CYGWIN32__)
#define SET_BINARY(f) setmode((f), O_BINARY)
#endif
#else  /* not DOSISH */
#ifndef O_BINARY
#define O_BINARY 0
#endif
#define SET_BINARY(f) 0
#endif /* not DOSISH */

#endif /* not C_FOPEN_H */
