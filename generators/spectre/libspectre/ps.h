/*
 * ps.h -- Include file for PostScript routines.
 * Copyright (C) 1992  Timothy O. Theisen
 * Copyright (C) 2004 Jose E. Marchesi
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
 * along with GNU gv; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *   Author: Tim Theisen           Systems Programmer
 * Internet: tim@cs.wisc.edu       Department of Computer Sciences
 *     UUCP: uwvax!tim             University of Wisconsin-Madison
 *    Phone: (608)262-0438         1210 West Dayton Street
 *      FAX: (608)262-9777         Madison, WI   53706
*/

#ifndef PS_H
#define PS_H

#include <libspectre/spectre-macros.h>
#include <stdio.h>

SPECTRE_BEGIN_DECLS

#ifndef NeedFunctionPrototypes
#if defined(FUNCPROTO) || defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define NeedFunctionPrototypes 1
#else
#define NeedFunctionPrototypes 0
#endif /* __STDC__ */
#endif /* NeedFunctionPrototypes */
 
/* Constants used to index into the bounding box array. */

#define LLX 0
#define LLY 1
#define URX 2
#define URY 3

	/* Constants used to store keywords that are scanned. */
	/* NONE is not a keyword, it tells when a field was not set */

/*
enum {ATEND = -1, NONE = 0, PORTRAIT, LANDSCAPE, ASCEND, DESCEND, SPECIAL};
*/

#define ATEND        (-1)
#define NONE            0
#define PORTRAIT        1
#define LANDSCAPE       2
#define SEASCAPE        3
#define UPSIDEDOWN      4
#define ASCEND          5
#define DESCEND         6
#define SPECIAL         7
#define AUTOMATIC       8

#define PSLINELENGTH 257	/* 255 characters + 1 newline + 1 NULL */

/*##############################################
      media
  ##############################################*/
typedef struct documentmedia {
	char *name;
	int  width;
	int  height;
} MediaStruct, *Media;


typedef struct document {
    unsigned int ref_count;
	
#ifdef GV_CODE
    int  structured;                    /* toc will be useful */ 
    int  labels_useful;                 /* page labels are distinguishable, hence useful */ 
#endif
    char *format;                       /* Postscript format */
    char *filename;                     /* Document filename */
    int  epsf;				/* Encapsulated PostScript flag. */
    char *title;			/* Title of document. */
    char *date;				/* Creation date. */
    char *creator;                      /* Program that created the file */
    char *fortext;
    char *languagelevel;
    int  pageorder;			/* ASCEND, DESCEND, SPECIAL */
    long beginheader, endheader;	/* offsets into file */
    unsigned int lenheader;
    long beginpreview, endpreview;
    unsigned int lenpreview;
    long begindefaults, enddefaults;
    unsigned int lendefaults;
    long beginprolog, endprolog;
    unsigned int lenprolog;
    long beginsetup, endsetup;
    unsigned int lensetup;
    long begintrailer, endtrailer;
    unsigned int lentrailer;
    int  boundingbox[4];
    int  default_page_boundingbox[4];
    int  orientation;			/* PORTRAIT, LANDSCAPE */
    int  default_page_orientation;	/* PORTRAIT, LANDSCAPE */
    unsigned int nummedia;
    struct documentmedia *media;
    Media default_page_media;
    unsigned int numpages;
    struct page *pages;
} *Document;

struct page {
    char *label;
    int  boundingbox[4];
    struct documentmedia *media;
    int  orientation;			/* PORTRAIT, LANDSCAPE */
    long begin, end;			/* offsets into file */
    unsigned int len;
};

	/* scans a PostScript file and return a pointer to the document
	   structure.  Returns NULL if file does not Conform to commenting
	   conventions . */

#define SCANSTYLE_NORMAL     0
#define SCANSTYLE_IGNORE_EOF (1<<0)
#define SCANSTYLE_IGNORE_DSC (1<<1)

Document				psscan (
#if NeedFunctionPrototypes
    const char *,
    int     /* scanstyle */
#endif
);

void					psdocdestroy (
#if NeedFunctionPrototypes
    struct document *
#endif
);

Document                                psdocreference (
#if NeedFunctionPrototypes
    struct document *
#endif
);

extern void				pscopydoc (
#if NeedFunctionPrototypes
    FILE *,
    char *,
    Document,
    char *
#endif
);

void                                    psgetpagebox (
#if NeedFunctionPrototypes
    const struct document *,
    int,
    int *,
    int *,
    int *,
    int *
#endif
);

void                                    pscopyheaders (
#if NeedFunctionPrototypes
    FILE *,
    FILE *,
    Document
#endif
);

void                                    pscopypage (
#if NeedFunctionPrototypes
    FILE *,
    FILE *,
    Document,
    unsigned int,
    unsigned int
#endif
);

void                                    pscopytrailer (
#if NeedFunctionPrototypes
    FILE *,
    FILE *,
    Document,
    unsigned int
#endif
);

SPECTRE_END_DECLS

#endif /* PS_H */
