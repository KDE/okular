/*
 * The layout of a font information block.
 * There is one of these for every loaded font or magnification thereof.
 * Duplicates are eliminated:  this is necessary because of possible recursion
 * in virtual fonts.
 *
 * Also note the strange units.  The design size is in 1/2^20 point
 * units (also called micro-points), and the individual character widths
 * are in the TFM file in 1/2^20 ems units, i.e., relative to the design size.
 *
 * We then change the sizes to SPELL units (unshrunk pixel / 2^16).
 */


#ifndef _FONT_H
#define _FONT_H


#include <stdio.h>
#include <qintdict.h>

#include "glyph.h"

#define	NOMAGSTP (-29999)

/** Per character information for virtual fonts */
struct macro {
  unsigned char	*pos;		/* address of first byte of macro */
  unsigned char	*end;		/* address of last+1 byte */
  long	        dvi_adv;	/* DVI units to move reference point */
  unsigned char	free_me;	/* if free(pos) should be called when */
				/* freeing space */
};



#define	FONT_IN_USE	1	/* used for housekeeping */
#define	FONT_LOADED	2	/* if font file has been read */
#define	FONT_VIRTUAL	4	/* if font is virtual */


typedef	void (*read_char_proc)(register struct font *, unsigned int);
typedef	void (*set_char_proc)(unsigned int, unsigned int);

struct font {
  struct font   *next;		/* link to next font info block */
  char          *fontname;	/* name of font */
  float          fsize;		/* size information (dots per inch) */
  int            magstepval;	/* magstep number * two, or NOMAGSTP */
  FILE          *file;		/* open font file or NULL */
  char          *filename;	/* name of font file */
  long           checksum;	/* checksum */
  unsigned short timestamp;	/* for LRU management of fonts */
  unsigned char  flags;		/* flags byte (see values below) */
  unsigned char  fmaxchar;	/* largest character code */
  double         dimconv;	/* size conversion factor */
  set_char_proc  set_char_p;	/* proc used to set char */

  /* these fields are used by (loaded) raster fonts */
  read_char_proc read_char;	/* function to read bitmap */
  struct glyph  *glyph;

  /* these fields are used by (loaded) virtual fonts */
  QIntDict<struct font> vf_table;  /* list of fonts used by this vf */
  struct font   *first_font;	/* first font defined */
  struct macro  *macro;

  /* I suppose the above could be put into a union, but we */
  /* wouldn't save all that much space. */
  

  font(char *nfontname, float nfsize, long chk, int mag, double dconv);
  ~font();

  void realloc_font(unsigned int newsize);
  void mark_as_used(void);
  unsigned char load_font(void);
};



#endif
