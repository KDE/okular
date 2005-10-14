// -*- C++ -*-
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

#include <qintdict.h>
#include <qstring.h>

#include <stdio.h>

class dviRenderer;
class TeXFont;

typedef void (dviRenderer::*set_char_proc)(unsigned int, unsigned int);


// Per character information for virtual fonts

class macro {
 public:
  macro();
  ~macro();

  unsigned char	*pos;		/* address of first byte of macro */
  unsigned char	*end;		/* address of last+1 byte */
  Q_INT32        dvi_advance_in_units_of_design_size_by_2e20;	/* DVI units to move reference point */
  bool          free_me;        // if memory at pos should be returned on destruction
};


class TeXFontDefinition {
 public:
  // Currently, kdvi supports fonts with at most 256 characters to
  // comply with "The DVI Driver Standard, Level 0". If you change
  // this value here, make sure to go through all the source and
  // ensure that character numbers are stored in ints rather than
  // unsigned chars.
  static const unsigned int max_num_of_chars_in_font = 256;
  enum font_flags {
    FONT_IN_USE  = 1,	// used for housekeeping
    FONT_LOADED  = 2,	// if font file has been read
    FONT_VIRTUAL = 4,	// if font is virtual 
    FONT_KPSE_NAME = 8 // if kpathsea has already tried to find the font name
  };
  

  TeXFontDefinition(QString nfontname, double _displayResolution_in_dpi, Q_UINT32 chk, Q_INT32 _scaled_size_in_DVI_units,
       class fontPool *pool, double _enlargement);
  ~TeXFontDefinition();

  void reset();
  void fontNameReceiver(const QString&);

  // Members for character fonts
  void           setDisplayResolution(double _displayResolution_in_dpi);

  bool           isLocated() const {return ((flags & FONT_KPSE_NAME) != 0);}
  void           markAsLocated() {flags |= FONT_KPSE_NAME;}

  void           mark_as_used();
  class fontPool *font_pool;    // Pointer to the pool that contains this font.
  QString        fontname;	// name of font, such as "cmr10"
  unsigned char  flags;		// flags byte (see values below)
  double         enlargement;
  Q_INT32        scaled_size_in_DVI_units;   // Scaled size from the font definition command; in DVI units
  set_char_proc  set_char_p;	// proc used to set char
  
  // Resolution of the display device (resolution will usually be
  // scaled, according to the zoom)
  double         displayResolution_in_dpi;
  
  FILE          *file;		// open font file or NULL
  QString        filename;	// name of font file
  
  TeXFont       *font;
  macro         *macrotable;    // used by (loaded) virtual fonts
  QIntDict<TeXFontDefinition> vf_table;      // used by (loaded) virtual fonts, list of fonts used by this vf, 
  // acessible by number
  TeXFontDefinition  *first_font;	// used by (loaded) virtual fonts, list of fonts used by this vf

#ifdef HAVE_FREETYPE
  const QString &getFullFontName() const {return fullFontName;}
  const QString &getFullEncodingName() const {return fullEncodingName;}
#endif
  const QString &getFontTypeName() const {return fontTypeName;}

#ifdef HAVE_FREETYPE
  /** For FREETYPE fonts, which use a map file, this field will
      contain the full name of the font (e.g. 'Computer Modern'). If
      the name does not exist, or cannot be found, this field will be
      QString::null. Only subclasses of TeXFont should write into this
      field. */
  QString        fullFontName;

  /** For FREETYPE fonts, which use a map file, this field will
      contain the full name of the font encoding (e.g. 'TexBase1'). If
      the encoding name does not exist, or cannot be found, this field
      will be QString::null. Only subclasses of TeXFont should write
      into this field. */
  QString        fullEncodingName;
#endif

 private:
  Q_UINT32       checksum;

  /** This will be set to a human-readable description of the font,
      e.g. "virtual" or "TeX PK", or "Type 1" */
  QString        fontTypeName;
  
  // Functions related to virtual fonts
  void          read_VF_index(void );
};

#endif
