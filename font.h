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

#include "dviwin.h"
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





typedef	void (dviWindow::*set_char_proc)(unsigned int, unsigned int);

struct font {
  // Currently, kdvi supports fonts with at most 256 characters to
  // comply with "The DVI Driver Standard, Level 0". If you change
  // this value here, make sure to go through all the source and
  // ensure that character numbers are stored in ints rather than
  // unsigned chars.
  static const int      max_num_of_chars_in_font = 256;
  enum                  font_flags {
                             FONT_IN_USE  = 1,	// used for housekeeping
                             FONT_LOADED  = 2,	// if font file has been read
                             FONT_VIRTUAL = 4	// if font is virtual 
                         };


  public:
                 font(char *nfontname, float nfsize, long chk, int mag, double dconv);
                ~font();
  glyph         *glyphptr(unsigned int ch);
  void           mark_as_used(void);
  unsigned char  load_font(void);

  struct font   *next;		// link to next font info block
  char          *fontname;	// name of font 
  unsigned char  flags;		// flags byte (see values below)
  double         dimconv;	// size conversion factor
  set_char_proc  set_char_p;	// proc used to set char
  float          fsize;		// size information (dots per inch)
  unsigned short timestamp;	// for LRU management of fonts
  FILE          *file;		// open font file or NULL

  glyph         *glyphtable;    // used by (loaded) raster fonts
  macro         *macrotable;    // used by (loaded) virtual fonts
  QIntDict<font> vf_table;      // used by (loaded) virtual fonts, list of fonts used by this vf, 
                                // acessible by number
  font          *first_font;	// used by (loaded) virtual fonts, list of fonts used by this vf

  private:
  int            magstepval;	/* magstep number * two, or NOMAGSTP */
  char          *filename;	/* name of font file */
  long           checksum;	/* checksum */


  FILE         *font_open (char *font, char **font_ret, double dpi, int *dpi_ret, char **filename_ret);

  // Functions related to virtual fonts
  void          read_VF_index(void );

  // Functions for pk fonts
  int           PK_get_nyb(FILE *fp);
  int           PK_packed_num(FILE *fp);
  void          PK_skip_specials(void);
  void          read_PK_char(unsigned int ch);
  void          read_PK_index(void);
};



#endif
