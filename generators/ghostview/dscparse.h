/* Copyright (C) 2000-2001, Ghostgum Software Pty Ltd.  All rights reserved.
    
  This file is part of GSview.
   
  This file is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
  to anyone for the consequences of using it or for whether it serves any
  particular purpose or works at all, unless he says so in writing.  Refer
  to the GNU General Public Licence for full details.
   
  Everyone is granted permission to copy, modify and redistribute this
  file, but only under the conditions described in the GNU General
  Public Licence.  A copy of this license is supposed to have been given
  to you along with this file so you can know your rights and
  responsibilities.  It should be in a file named COPYING.  Among other
  things, the copyright notice and this notice must be preserved on all
  copies.
*/

/* $Id: dscparse.h 420737 2005-06-01 09:09:35Z wgreven $ */

/* dscparse.h */
/* Interface for the DSC parser. */

#ifndef _DSCPARSE_H_
#define _DSCPARSE_H_

/* Some local types that may need modification */
typedef bool GSBOOL;
typedef unsigned long GSDWORD;	/* must be at least 32 bits */
typedef unsigned int GSWORD;	/* must be at least 16 bits */

#ifndef FALSE
# define FALSE ((GSBOOL)0)
# define TRUE ((GSBOOL)(!FALSE))
#endif

#ifndef dsc_private
# ifdef private
#  define dsc_private private
# else
#  define dsc_private static
# endif
#endif

#ifndef min
# define min(a,b)  ((a) < (b) ? (a) : (b))
#endif

/* macros to allow conversion of function declarations to K&R */
#ifndef P0
#define P0() void
#define P1(t1) t1
#define P2(t1,t2) t1,t2
#define P3(t1,t2,t3) t1,t2,t3
#define P4(t1,t2,t3,t4) t1,t2,t3,t4
#define P5(t1,t2,t3,t4,t5) t1,t2,t3,t4,t5
#define P6(t1,t2,t3,t4,t5,t6) t1,t2,t3,t4,t5,t6
#endif

/* maximum legal length of lines in a DSC compliant file */
#define DSC_LINE_LENGTH 255

/* memory for strings is allocated in chunks of this length */
#define CDSC_STRING_CHUNK 4096

/* page array is allocated in chunks of this many pages */
#define CDSC_PAGE_CHUNK 128	

/* buffer length for storing lines passed to dsc_scan_data() */
/* must be at least 2 * DSC_LINE_LENGTH */
/* We choose 8192 as twice the length passed to us by GSview */
#define CDSC_DATA_LENGTH 8192

/* Return codes from dsc_scan_data()
 *  < 0     = error
 *  >=0     = OK
 *
 *  -1      = error, usually insufficient memory.
 *  0-9     = normal
 *  10-99   = internal codes, should not be seen.
 *  100-999 = identifier of last DSC comment processed.
 */

typedef enum {
  CDSC_ERROR		= -1,	/* Fatal error, usually insufficient memory */

  CDSC_OK		= 0,	/* OK, no DSC comment found */
  CDSC_NOTDSC	 	= 1,	/* Not DSC, or DSC is being ignored */

/* Any section */
  CDSC_UNKNOWNDSC	= 100,	/* DSC comment not recognised */

/* Header section */
  CDSC_PSADOBE		= 200,	/* %!PS-Adobe- */
  CDSC_BEGINCOMMENTS	= 201,	/* %%BeginComments */
  CDSC_ENDCOMMENTS	= 202,	/* %%EndComments */
  CDSC_PAGES		= 203,	/* %%Pages: */
  CDSC_CREATOR		= 204,	/* %%Creator: */
  CDSC_CREATIONDATE	= 205,	/* %%CreationDate: */
  CDSC_TITLE		= 206,	/* %%Title: */
  CDSC_FOR		= 207,	/* %%For: */
  CDSC_LANGUAGELEVEL	= 208,	/* %%LanguageLevel: */
  CDSC_BOUNDINGBOX	= 209,	/* %%BoundingBox: */
  CDSC_ORIENTATION	= 210,	/* %%Orientation: */
  CDSC_PAGEORDER	= 211,	/* %%PageOrder: */
  CDSC_DOCUMENTMEDIA	= 212,	/* %%DocumentMedia: */
  CDSC_DOCUMENTPAPERSIZES    = 213,	/* %%DocumentPaperSizes: */
  CDSC_DOCUMENTPAPERFORMS    = 214,	/* %%DocumentPaperForms: */
  CDSC_DOCUMENTPAPERCOLORS   = 215,	/* %%DocumentPaperColors: */
  CDSC_DOCUMENTPAPERWEIGHTS  = 216,	/* %%DocumentPaperWeights: */
  CDSC_DOCUMENTDATA	     = 217,	/* %%DocumentData: */
  CDSC_REQUIREMENTS	     = 218,	/* IGNORED %%Requirements: */
  CDSC_DOCUMENTNEEDEDFONTS   = 219,	/* IGNORED %%DocumentNeededFonts: */
  CDSC_DOCUMENTSUPPLIEDFONTS = 220,	/* IGNORED %%DocumentSuppliedFonts: */
  CDSC_HIRESBOUNDINGBOX	     = 221,	/* %%HiResBoundingBox: */
  CDSC_CROPBOX	     	     = 222,	/* %%CropBox: */
// added 02 August 2005
  CDSC_COPYRIGHT             = 223, /* %%Copyright: */

/* Preview section */
  CDSC_BEGINPREVIEW	= 301,	/* %%BeginPreview */
  CDSC_ENDPREVIEW	= 302,	/* %%EndPreview */

/* Defaults section */
  CDSC_BEGINDEFAULTS	= 401,	/* %%BeginDefaults */
  CDSC_ENDDEFAULTS	= 402,	/* %%EndDefaults */
/* also %%PageMedia, %%PageOrientation, %%PageBoundingBox */

/* Prolog section */
  CDSC_BEGINPROLOG	= 501,	/* %%BeginProlog */
  CDSC_ENDPROLOG	= 502,	/* %%EndProlog */
  CDSC_BEGINFONT	= 503,	/* IGNORED %%BeginFont */
  CDSC_ENDFONT		= 504,	/* IGNORED %%EndFont */
  CDSC_BEGINFEATURE	= 505,	/* IGNORED %%BeginFeature */
  CDSC_ENDFEATURE	= 506,	/* IGNORED %%EndFeature */
  CDSC_BEGINRESOURCE	= 507,	/* IGNORED %%BeginResource */
  CDSC_ENDRESOURCE	= 508,	/* IGNORED %%EndResource */
  CDSC_BEGINPROCSET	= 509,	/* IGNORED %%BeginProcSet */
  CDSC_ENDPROCSET	= 510,	/* IGNORED %%EndProcSet */

/* Setup section */
  CDSC_BEGINSETUP	= 601,	/* %%BeginSetup */
  CDSC_ENDSETUP		= 602,	/* %%EndSetup */
  CDSC_FEATURE		= 603,	/* IGNORED %%Feature: */
  CDSC_PAPERCOLOR	= 604,	/* IGNORED %%PaperColor: */
  CDSC_PAPERFORM	= 605,	/* IGNORED %%PaperForm: */
  CDSC_PAPERWEIGHT	= 606,	/* IGNORED %%PaperWeight: */
  CDSC_PAPERSIZE	= 607,	/* %%PaperSize: */
/* also %%Begin/EndFeature, %%Begin/EndResource */

/* Page section */
  CDSC_PAGE		= 700,	/* %%Page: */
  CDSC_PAGETRAILER	= 701,	/* IGNORED %%PageTrailer */
  CDSC_BEGINPAGESETUP	= 702,	/* IGNORED %%BeginPageSetup */
  CDSC_ENDPAGESETUP	= 703,	/* IGNORED %%EndPageSetup */
  CDSC_PAGEMEDIA	= 704,	/* %%PageMedia: */
/* also %%PaperColor, %%PaperForm, %%PaperWeight, %%PaperSize */
  CDSC_PAGEORIENTATION	= 705,	/* %%PageOrientation: */
  CDSC_PAGEBOUNDINGBOX	= 706,	/* %%PageBoundingBox: */
/* also %%Begin/EndFont, %%Begin/EndFeature */
/* also %%Begin/EndResource, %%Begin/EndProcSet */
  CDSC_INCLUDEFONT	= 707,	/* IGNORED %%IncludeFont: */
  CDSC_VIEWINGORIENTATION = 708, /* %%ViewingOrientation: */

/* Trailer section */
  CDSC_TRAILER		= 800,	/* %%Trailer */
/* also %%Pages, %%BoundingBox, %%Orientation, %%PageOrder, %%DocumentMedia */ 
/* %%Page is recognised as an error */
/* also %%DocumentNeededFonts, %%DocumentSuppliedFonts */

/* End of File */
  CDSC_EOF		= 900	/* %%EOF */
} CDSC_RETURN_CODE;


/* stored in dsc->preview */ 
typedef enum {
    CDSC_NOPREVIEW = 0,
    CDSC_EPSI = 1,
    CDSC_TIFF = 2,
    CDSC_WMF = 3,
    CDSC_PICT = 4
} CDSC_PREVIEW_TYPE;

/* stored in dsc->page_order */ 
typedef enum {
    CDSC_ORDER_UNKNOWN = 0,
    CDSC_ASCEND = 1,
    CDSC_DESCEND = 2,
    CDSC_SPECIAL = 3
} CDSC_PAGE_ORDER;

/* stored in dsc->page_orientation and dsc->page[pagenum-1].orientation */ 
typedef enum {
    CDSC_ORIENT_UNKNOWN = 0,
    CDSC_PORTRAIT = 1,
    CDSC_LANDSCAPE = 2,
    CDSC_UPSIDEDOWN = 3,
    CDSC_SEASCAPE = 4
} CDSC_ORIENTATION_ENUM;

/* stored in dsc->document_data */
typedef enum {
    CDSC_DATA_UNKNOWN = 0,
    CDSC_CLEAN7BIT = 1,
    CDSC_CLEAN8BIT = 2,
    CDSC_BINARY = 3
} CDSC_DOCUMENT_DATA ;

typedef struct CDSCBBOX_S {
    int llx;
    int lly;
    int urx;
    int ury;
} CDSCBBOX;

typedef struct CDSCFBBOX_S {
    float fllx;
    float flly;
    float furx;
    float fury;
} CDSCFBBOX;

typedef struct CDSCMEDIA_S {
    const char *name;
    float width;	/* PostScript points */
    float height;
    float weight;	/* GSM */
    const char *colour;
    const char *type;
    CDSCBBOX *mediabox;	/* Used by GSview for PDF MediaBox */
} CDSCMEDIA;

#define CDSC_KNOWN_MEDIA 46
extern const CDSCMEDIA dsc_known_media[CDSC_KNOWN_MEDIA];

typedef struct CDSCCTM_S { /* used for %%ViewingOrientation */
    float xx;
    float xy;
    float yx;
    float yy;
    /* float ty; */
    /* float ty; */
} CDSCCTM;

typedef struct CDSCPAGE_S {
    int ordinal;
    const char *label;
    unsigned long begin;
    unsigned long end;
    unsigned int orientation;
    const CDSCMEDIA *media;
    CDSCBBOX *bbox;  /* PageBoundingBox, also used by GSview for PDF CropBox */
    CDSCCTM *viewing_orientation;
} CDSCPAGE;

/* binary DOS EPS header */
typedef struct CDSCDOSEPS_S {
    GSDWORD ps_begin;
    GSDWORD ps_length;
    GSDWORD wmf_begin;
    GSDWORD wmf_length;
    GSDWORD tiff_begin;
    GSDWORD tiff_length;
    GSWORD checksum;
} CDSCDOSEPS;

/* rather than allocated every string with malloc, we allocate
 * chunks of 4k and place the (usually) short strings in these
 * chunks.
 */
typedef struct CDSCSTRING_S CDSCSTRING;
struct CDSCSTRING_S {
    unsigned int index;
    unsigned int length;
    char *data;
    CDSCSTRING *next;
};


/* DSC error reporting */

typedef enum {
  CDSC_MESSAGE_BBOX = 0,
  CDSC_MESSAGE_EARLY_TRAILER = 1,
  CDSC_MESSAGE_EARLY_EOF = 2,
  CDSC_MESSAGE_PAGE_IN_TRAILER = 3,
  CDSC_MESSAGE_PAGE_ORDINAL = 4,
  CDSC_MESSAGE_PAGES_WRONG = 5,
  CDSC_MESSAGE_EPS_NO_BBOX = 6,
  CDSC_MESSAGE_EPS_PAGES = 7,
  CDSC_MESSAGE_NO_MEDIA = 8,
  CDSC_MESSAGE_ATEND = 9,
  CDSC_MESSAGE_DUP_COMMENT = 10,
  CDSC_MESSAGE_DUP_TRAILER = 11,
  CDSC_MESSAGE_BEGIN_END = 12,
  CDSC_MESSAGE_BAD_SECTION = 13,
  CDSC_MESSAGE_LONG_LINE = 14,
  CDSC_MESSAGE_INCORRECT_USAGE = 15
} CDSC_MESSAGE_ERROR;

/* severity */
typedef enum {
  CDSC_ERROR_INFORM	= 0,	/* Not an error */
  CDSC_ERROR_WARN	= 1,	/* Not a DSC error itself,  */
  CDSC_ERROR_ERROR	= 2	/* DSC error */
} CDSC_MESSAGE_SEVERITY;

/* response */
typedef enum {
  CDSC_RESPONSE_OK	= 0,
  CDSC_RESPONSE_CANCEL	= 1,
  CDSC_RESPONSE_IGNORE_ALL = 2
} CDSC_RESPONSE;

extern const char * const dsc_message[];

typedef struct CDSC_S CDSC;
struct CDSC_S {
    /* public data */
    GSBOOL dsc;			/* TRUE if DSC comments found */
    GSBOOL ctrld;		/* TRUE if has CTRLD at start of stream */
    GSBOOL pjl;			/* TRUE if has HP PJL at start of stream */
    GSBOOL epsf;		/* TRUE if EPSF */
    GSBOOL pdf;			/* TRUE if Portable Document Format */
    unsigned int preview;	/* enum CDSC_PREVIEW_TYPE */
    char *dsc_version;	/* first line of file */
    unsigned int language_level;
    unsigned int document_data;	/* Clean7Bit, Clean8Bit, Binary */
				/* enum CDSC_DOCUMENT_DATA */
    /* DSC sections */
    unsigned long begincomments;
    unsigned long endcomments;
    unsigned long beginpreview;
    unsigned long endpreview;
    unsigned long begindefaults;
    unsigned long enddefaults;
    unsigned long beginprolog;
    unsigned long endprolog;
    unsigned long beginsetup;
    unsigned long endsetup;
    unsigned long begintrailer;
    unsigned long endtrailer;
    CDSCPAGE *page;
    unsigned int page_count;	/* number of %%Page: pages in document */
    unsigned int page_pages;	/* number of pages in document from %%Pages: */
    unsigned int page_order;	/* enum CDSC_PAGE_ORDER */
    unsigned int page_orientation;  /* the default page orientation */
				/* enum CDSC_ORIENTATION */
    CDSCCTM *viewing_orientation;
    unsigned int media_count;	/* number of media items */
    CDSCMEDIA **media;		/* the array of media */
    const CDSCMEDIA *page_media;/* the default page media */
    CDSCBBOX *bbox;		/* the document bounding box */
    CDSCBBOX *page_bbox;	/* the default page bounding box */
    CDSCDOSEPS *doseps;		/* DOS binary header */
    char *dsc_title;
    char *dsc_creator;
    char *dsc_date;
    char *dsc_for;
    // added 02 August 2005
    char *dsc_copyright;

    unsigned int max_error;	/* highest error number that will be reported */
    const int *severity;	/* array of severity values, one per error */


    /* private data */
    void *caller_data;		/* pointer to be provided when calling */
			        /* error and debug callbacks */
    int id;			/* last DSC comment found */
    int scan_section;		/* section currently being scanned */
				/* enum CDSC_SECTION */

    unsigned long doseps_end;	/* ps_begin+ps_length, otherwise 0 */
    unsigned int page_chunk_length; /* number of pages allocated */
    unsigned long file_length;	/* length of document */
		/* If provided we try to recognise %%Trailer and %%EOF */
		/* incorrectly embedded inside document. */
		/* Can be left set to default value of 0 */
    int skip_document;		/* recursion level of %%BeginDocument: */
    int skip_bytes;		/* #bytes to ignore from BeginData: */
				/* or DOSEPS preview section */
    int skip_lines;		/* #lines to ignore from BeginData: */
    GSBOOL skip_pjl;		/* TRUE if skip PJL until first PS comment */ 
    int begin_font_count;	/* recursion level of %%BeginFont */
    int begin_feature_count;	/* recursion level of %%BeginFeature */
    int begin_resource_count;	/* recursion level of %%BeginResource */
    int begin_procset_count;	/* recursion level of %%BeginProcSet */

    /* buffer for input */
    char data[CDSC_DATA_LENGTH];/* start of buffer */
    unsigned int data_length; 	/* length of data in buffer */
    unsigned int data_index;	/* offset to next char in buffer */
    unsigned long data_offset;	/* offset from start of document */
			       	/* to byte in data[0] */
    GSBOOL eof;			/* TRUE if there is no more data */

    /* information about DSC line */
    char *line;			/* pointer to last read DSC line */
				/* not null terminated */
    unsigned int line_length; 	/* number of characters in line */
    GSBOOL eol;			/* TRUE if dsc_line contains EOL */
    GSBOOL last_cr;		/* TRUE if last line ended in \r */
				/* check next time for \n */
    unsigned int line_count;	/* line number */
    GSBOOL long_line;		/* TRUE if found a line longer than 255 characters */
    char last_line[256];	/* previous DSC line, used for %%+ */

    /* more efficient string storage (for short strings) than malloc */
    CDSCSTRING *string_head;	/* linked list head */
    CDSCSTRING *string;		/* current list item */

    /* memory allocation routines */
    void *(*memalloc)(P2(size_t size, void *closure_data));
    void (*memfree)(P2(void *ptr, void *closure_data));
    void *mem_closure_data;

    /* function for printing debug messages */
    void (*debug_print_fn)(P2(void *caller_data, const char *str));

    /* function for reporting errors in DSC comments */
    int (*dsc_error_fn)(P5(void *caller_data, CDSC *dsc, 
	unsigned int explanation, const char *line, unsigned int line_len));

    /* public data */
    /* Added 2001-10-01 */
    CDSCFBBOX *hires_bbox;	/* the hires document bounding box */
    CDSCFBBOX *crop_box;	/* the size of the trimmed page */
};


/* Public functions */

/* Create and initialise DSC parser */
CDSC *dsc_init(P1(void *caller_data));

CDSC *dsc_init_with_alloc(P4(
    void *caller_data,
    void *(*memalloc)(size_t size, void *closure_data),
    void (*memfree)(void *ptr, void *closure_data),
    void *closure_data));

/* Free the DSC parser */
void dsc_free(P1(CDSC *dsc));

/* Tell DSC parser how long document will be, to allow ignoring
 * of early %%Trailer and %%EOF.  This is optional.
 */
void dsc_set_length(P2(CDSC *dsc, unsigned long len));

/* Process a buffer containing DSC comments and PostScript */
int dsc_scan_data(P3(CDSC *dsc, const char *data, int len));

/* All data has been processed, fixup any DSC errors */
int dsc_fixup(P1(CDSC *dsc));

/* Install error query function */
void dsc_set_error_function(P2(CDSC *dsc, 
    int (*dsc_error_fn)(P5(void *caller_data, CDSC *dsc, 
	unsigned int explanation, const char *line, unsigned int line_len))));

/* Install print function for debug messages */
void dsc_set_debug_function(P2(CDSC *dsc, 
	void (*debug_fn)(P2(void *caller_data, const char *str))));

/* Print a message to debug output, if provided */
void dsc_debug_print(P2(CDSC *dsc, const char *str));

/* should be internal only functions, but made available to 
 * GSview for handling PDF
 */
int dsc_add_page(P3(CDSC *dsc, int ordinal, char *label));
int dsc_add_media(P2(CDSC *dsc, CDSCMEDIA *media));
int dsc_set_page_bbox(P6(CDSC *dsc, unsigned int page_number, 
    int llx, int lly, int urx, int ury));

#endif

// vim:sw=4:sts=4:ts=8:noet
