/* xopendir.h: Checked directory operations.

Copyright (C) 1994 Karl Berry.

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

#ifndef KPATHSEA_XDIR_H
#define KPATHSEA_XDIR_H

#include <kpathsea/c-dir.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Like opendir and closedir, but abort on error.  */
extern DIR *xopendir P1H(string dirname);
extern void xclosedir P1H(DIR *);

#endif /* not KPATHSEA_XDIR_H */
