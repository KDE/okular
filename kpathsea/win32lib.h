/* System description file for Windows NT.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.

This file is part of Web2C.

Web2C is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

Web2C is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Web2C; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef KPATHSEA_WIN32LIB_H
#define KPATHSEA_WIN32LIB_H

#pragma warning( disable : 4007 4096 4018 4244 )  

#include <kpathsea/c-auto.h>
#include <kpathsea/c-proto.h>

/*
 *      Define symbols to identify the version of Unix this is.
 *      Define all the symbols that apply correctly.
 */

#ifndef DOSISH
#define DOSISH
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN      _MAX_PATH
#endif

#define HAVE_DUP2       	1
#define HAVE_RENAME     	1
#define HAVE_RMDIR      	1
#define HAVE_MKDIR      	1
#define HAVE_GETHOSTNAME	1
#define HAVE_RANDOM		1
#define USE_UTIME		1
#define HAVE_MOUSE		1
#define HAVE_TZNAME		1

/* These have to be defined because our compilers treat __STDC__ as being
   defined (most of them anyway). */

#define access     _access
#define alloca     _alloca
#define chdir      _chdir
#define chmod      _chmod
#define close      _close
#define creat      _creat
#define daylight   _daylight
#define dup        _dup
#define dup2       _dup2
#define execlp     _execlp
#define execvp     _execvp
#define fcloseall  _fcloseall
#define fdopen     _fdopen
#define fileno     _fileno
#define flushall   _flushall
#define fstat      _fstat
#define ftime      _ftime
#define getpid     _getpid
#define getcwd     _getcwd
#define getwd(dir)  GetCurrentDirectory(MAXPATHLEN, dir)
#define index      strchr
#define isascii    __isascii
#define isatty     _isatty
#define itoa       _itoa
#define link       _link
#define lseek      _lseek
#define memicmp    _memicmp
#define mkdir      _mkdir
#define mktemp     _mktemp
#define open       _open
#define pipe       _pipe
#if 0
#define popen	   _popen
#define pclose	   _pclose
#endif
#define putenv     _putenv
#define read       _read
#define rmdir      _rmdir
#define setmode    _setmode
#define spawnlp    _spawnlp
#define stat       _stat
#define stricmp    _stricmp
#define strcasecmp _stricmp
#define strdup     _strdup
#define strncasecmp _strnicmp
#define strnicmp   _strnicmp
#define tempnam    _tempnam
#define timeb      _timeb
#define timezone   _timezone
#define unlink     _unlink
#define umask	   _umask
#define utime	   _utime
#define write      _write

#ifndef S_IFMT
#define S_IFMT _S_IFMT
#endif
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif
#ifndef S_IFCHR
#define S_IFCHR _S_IFCHR
#endif
#ifndef S_IFIFO
#define S_IFIFO _S_IFIFO
#endif
#ifndef S_IFREG
#define S_IFREG _S_IFREG
#endif
#ifndef S_IREAD
#define S_IREAD _S_IREAD
#endif
#ifndef S_IWRITE
#define S_IWRITE _S_IWRITE
#endif
#define S_IEXEC  _S_IEXEC 
#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif
#ifndef S_IXGRP
#define S_IXGRP _S_IEXEC
#endif
#ifndef S_IXOTH
#define S_IXOTH _S_IEXEC
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IROTH
#define S_IROTH _S_IREAD
#endif
#ifndef S_IWOTH
#define S_IWOTH _S_IWRITE
#endif
#ifndef S_IRGRP
#define S_IRGRP _S_IREAD
#endif
#ifndef S_IWGRP
#define S_IWGRP _S_IWRITE
#endif
#ifndef O_RDWR
#define O_RDWR _O_RDWR
#endif
#ifndef O_CREAT
#define O_CREAT _O_CREAT
#endif
#ifndef O_TRUNC
#define O_TRUNC _O_TRUNC
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#ifndef O_WRONLY
#define O_WRONLY _O_WRONLY
#endif
#ifndef O_APPEND
#define O_APPEND _O_APPEND
#endif
#ifndef O_TEXT
#define O_TEXT _O_TEXT
#endif
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif
#ifndef O_EXCL
#define O_EXCL _O_EXCL
#endif

/* Test for each symbol individually and define the ones necessary (some
   systems claiming Posix compatibility define some but not all). */

#if defined (S_IFBLK) && !defined (S_ISBLK)
#define        S_ISBLK(m)      (((m)&S_IFMT) == S_IFBLK)       /* block device */
#endif

#if defined (S_IFCHR) && !defined (S_ISCHR)
#define        S_ISCHR(m)      (((m)&S_IFMT) == S_IFCHR)       /* character device */
#endif

#if defined (S_IFDIR) && !defined (S_ISDIR)
#define        S_ISDIR(m)      (((m)&S_IFMT) == S_IFDIR)       /* directory */
#endif

#if defined (S_IFREG) && !defined (S_ISREG)
#define        S_ISREG(m)      (((m)&S_IFMT) == S_IFREG)       /* file */
#endif

#if defined (S_IFIFO) && !defined (S_ISFIFO)
#define        S_ISFIFO(m)     (((m)&S_IFMT) == S_IFIFO)       /* fifo - named pipe */
#endif

#if defined (S_IFLNK) && !defined (S_ISLNK)
#define        S_ISLNK(m)      (((m)&S_IFMT) == S_IFLNK)       /* symbolic link */
#endif

#if defined (S_IFSOCK) && !defined (S_ISSOCK)
#define        S_ISSOCK(m)     (((m)&S_IFMT) == S_IFSOCK)      /* socket */
#endif

/* Define this so that winsock.h definitions don't get included when
   windows.h is...  For this to have proper effect, config.h must
   always be included before windows.h.  */ 
#if !defined(_WINSOCKAPI_) && !defined(WWW_WIN_DLL)
#define _WINSOCKAPI_    1
#endif

#include <windows.h>

/* Defines size_t and alloca ().  */
#include <malloc.h>

/* For proper declaration of environ.  */
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <process.h>

/* Web2C takes care of ensuring that these are defined.  */
#ifdef max
#undef max
#undef min
#endif

/* Functions from win32lib.c */
extern KPSEDLL FILE *popen(const char *, const char *);
extern KPSEDLL int pclose(FILE *);

/* ============================================================ */

#endif /* not KPATHSEA_WIN32LIB_H */
