/*
 * ps.h -- Include file for PostScript routines.
 * Copyright (C) 1992  Timothy O. Theisen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *   Author: Tim Theisen           Systems Programmer
 * Internet: tim@cs.wisc.edu       Department of Computer Sciences
 *     UUCP: uwvax!tim             University of Wisconsin-Madison
 *    Phone: (608)262-0438         1210 West Dayton Street
 *      FAX: (608)262-9777         Madison, WI   53706
 */

#ifndef _PS_H
#define _PS_H

#define PSLINELENGTH 257	/* 255 characters + 1 newline + 1 NULL */

	/* Copy a portion of the PostScript file */

void pscopy(FILE *from, FILE *to, long begin, long end);

	/* Copy a portion of the PostScript file upto a comment */

char *pscopyuntil(FILE *from, FILE *to, long begin, long end,
		  const char *comment);

#endif 

/* 
 * vim:sw=4:sts=4:ts=8:noet 
 */
