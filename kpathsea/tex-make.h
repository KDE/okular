/* tex-make.h: declarations for executing external scripts.

Copyright (C) 1993, 94 Karl Berry.

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

#ifndef KPATHSEA_TEX_MAKE_H
#define KPATHSEA_TEX_MAKE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/types.h>


/* If true, throw away standard error from the mktex... scripts.
   (Standard output is the filename, so we never throw that away.)  */
extern KPSEDLL boolean kpse_make_tex_discard_errors;


/* Run a program to create a file named by BASE_FILE in format FORMAT.
   Return the full filename to it, or NULL.  Any other information about
   the file is passed through environment variables.  See the mktexpk
   stuff in `tex-make.c' for an example. */
extern string kpse_make_tex P2H(kpse_file_format_type format,
                                const_string base_file);

#endif /* not KPATHSEA_TEX_MAKE_H */
