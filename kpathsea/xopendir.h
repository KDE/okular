/* xopendir.h: Checked directory operations.

Copyright (C) 1994, 96 Karl Berry.

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

#if !defined (KPATHSEA_XOPENDIR_H) && !defined (WIN32)
#define KPATHSEA_XOPENDIR_H

#include <kpathsea/c-dir.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Like opendir and closedir, but abort on error.  */
extern DIR *xopendir P1H(string dirname);
extern void xclosedir P1H(DIR *);

#endif /* not (KPATHSEA_XOPENDIR_H or WIN32) */
