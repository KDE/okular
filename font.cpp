// $Id$

#include <stdio.h>
#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>
#include <qapplication.h>
#include <qfile.h>

#include "dviwin.h"
#include "font.h"
#include "kdvi.h"
#include "xdvi.h"

extern void oops(QString message);


#define	PK_PRE		247
#define	PK_ID		89
#define	PK_MAGIC	(PK_PRE << 8) + PK_ID
#define	GF_PRE		247
#define	GF_ID		131
#define	GF_MAGIC	(GF_PRE << 8) + GF_ID
#define	VF_PRE		247
#define	VF_ID_BYTE	202
#define	VF_MAGIC	(VF_PRE << 8) + VF_ID_BYTE


macro::macro()
{
  pos     = 0L;		/* address of first byte of macro */
  end     = 0L;		/* address of last+1 byte */
  dvi_adv = 0;	/* DVI units to move reference point */
  free_me =  false;
}

macro::~macro()
{
  if ((pos != 0L) && (free_me == true))
    delete [] pos;
}


void font::fontNameReceiver(QString fname)
{
  flags |= font::FONT_LOADED;

  filename = fname;

#ifdef DEBUG_FONT
  kdDebug() << "FONT NAME RECEIVED:" << filename << endl;
#endif

  file = fopen(QFile::encodeName(filename), "r");
  if (file == NULL) {
    kdError() << i18n("Can't find font ") << fontname << "." << endl;
    return;
  }

  set_char_p = &dviWindow::set_char;
  int magic      = two(file);

  if (magic == PK_MAGIC) {
    // Achtung! Read_PK_index kann auch schiefgehen!
    read_PK_index();
    set_char_p = &dviWindow::set_char;
  } else
    if (magic == GF_MAGIC)
      oops(i18n("The GF format for font file %1 is no longer supported").arg(filename) );
    else
      if (magic == VF_MAGIC) {
	read_VF_index();
	set_char_p = &dviWindow::set_vf_char;
      } else
	oops(i18n("Cannot recognize format for font file %1").arg(filename) );
}


font::font(const char *nfontname, double resolution_in_dpi, long chk, Q_INT32 scale, double pixelPerDVIunit, class fontPool *pool, double shrinkFact, 
	   double _enlargement, double _cmPerDVIunit)
{
#ifdef DEBUG_FONT
  kdDebug() << "constructing font " << nfontname << " at " << (int) (resolution_in_dpi + 0.5) << " dpi" << endl;
#endif

  shrinkFactor = shrinkFact;
  enlargement  = _enlargement;
  cmPerDVIunit = _cmPerDVIunit;

  font_pool    = pool;
  fontname     = nfontname;
  naturalResolution_in_dpi        = resolution_in_dpi;
  checksum     = chk;
  flags        = font::FONT_IN_USE;
  file         = NULL; 
  filename     = "";
  x_dimconv    = scale*pixelPerDVIunit;
  scaled_size  = scale;
  
  glyphtable   = 0;
  macrotable   = 0;

  for(unsigned int i=0; i<max_num_of_chars_in_font; i++)
    characterPixmaps[i] = 0;

  // By default, this font contains only empty characters. After the
  // font has been loaded, this function pointer will be replaced by
  // another one.
  set_char_p  = &dviWindow::set_empty_char;
}


font::~font()
{
#ifdef DEBUG_FONT
  kdDebug() << "discarding font " << fontname << " at " << (int)(naturalResolution_in_dpi + 0.5) << " dpi" << endl;
#endif

  if (fontname != 0) {
    delete [] fontname;
    fontname = 0;
  }
  if (glyphtable != 0) {
    delete [] glyphtable;
    glyphtable = 0;
  }
  if (macrotable != 0) {
    delete [] macrotable;
    macrotable = 0;
  }

  for(unsigned int i=0; i<max_num_of_chars_in_font; i++)
    if (characterPixmaps[i]) {
      delete characterPixmaps[i];
      characterPixmaps[i] = 0;
    }

  if (flags & FONT_LOADED) {
    if (file != 0) {
      fclose(file);
      file = 0;
    }
    if (flags & FONT_VIRTUAL)
      vf_table.clear();
  }
}

void font::reset(double resolution_in_dpi, double pixelPerDVIunit)
{
  if (glyphtable != 0) {
    delete [] glyphtable;
    glyphtable = 0;
  }
  if (macrotable != 0) {
    delete [] macrotable;
    macrotable = 0;
  }
  
  for(unsigned int i=0; i<max_num_of_chars_in_font; i++)
    if (characterPixmaps[i]) {
      delete characterPixmaps[i];
      characterPixmaps[i] = 0;
    }
  
  if (flags & FONT_LOADED) {
    if (file != 0) {
      fclose(file);
      file = 0;
    }
    if (flags & FONT_VIRTUAL)
      vf_table.clear();
  }
 
  filename                 = QString::null;
  flags                    = font::FONT_IN_USE;
  x_dimconv                = scaled_size*pixelPerDVIunit;
  naturalResolution_in_dpi = resolution_in_dpi;
  set_char_p               = &dviWindow::set_empty_char;
}

void font::setShrinkFactor(float sf)
{
  shrinkFactor = sf;

  for(unsigned int i=0; i<max_num_of_chars_in_font; i++) {
    if (characterPixmaps[i] != 0) {
      delete characterPixmaps[i];
      characterPixmaps[i] = 0;
    }
  }
}


/** mark_as_used marks the font, and all the fonts it referrs to, as
    used, i.e. their FONT_IN_USE-flag is set. */

void font::mark_as_used(void)
{
#ifdef DEBUG_FONT
  kdDebug() << "marking font " << fontname << " at " << (int) (naturalResolution_in_dpi + 0.5) << " dpi" << endl;
#endif

  if (flags & font::FONT_IN_USE)
    return;

  flags |= font::FONT_IN_USE;

  // For virtual fonts, also go through the list of referred fonts
  if (flags & font::FONT_VIRTUAL) {
    QIntDictIterator<font> it(vf_table);
    while( it.current() ) {
      it.current()->flags |= font::FONT_IN_USE;
      ++it;
    }
  }
}


QPixmap font::characterPixmap(unsigned int ch)
{
  // Paranoia checks
  if (ch > max_num_of_chars_in_font) {
    kdError(4300) << "Tried to access character with number " << ch << endl;
    return nullPixmap;
  }
  if (characterPixmaps[ch])
    return *(characterPixmaps[ch]);
  
  glyph *g = glyphptr(ch);
  if (g == 0) {
    return nullPixmap;
  }
  
  // All is fine? Then we rescale the bitmap in order to produce the
  // required pixmap.  Rescaling a character, however, is an art that
  // requires some explanation...
  //
  // If we would just divide the size of the character and the
  // coordinates by the shrink factor, then the result would look
  // quite ugly: due to the ineviatable rounding errors in the integer
  // arithmetic, the characters would be displaced by up to a
  // pixel. That doesn't sound much, but on low-resolution devices,
  // such as a notebook screen, the effect would be a "dancing line"
  // of characters, which looks really bad.
  //
  // The cure is the following procedure:
  //
  // (a) scale the hot point 
  //
  // (b) fit the unshrunken bitmap into a bitmap which is even
  // bigger. Use this to produce extra empty rows and columns at the
  // borders. The proper choice of the border size will ensure that
  // the hot point will fall exactly onto the coordinates which we
  // calculated previously.
  
  // Here the cheating starts ... on the screen many fonts look very
  // light. We improve on the looks by lowering the shrink factor just
  // when shrinking the characters. The position of the chars on the
  // screen will not be affected, the chars are just slightly larger.

  
  // Calculate the coordinates of the hot point in the shrunken
  // bitmap
  g->x2 = (int)(g->x/shrinkFactor);
  g->y2 = (int)(g->y/shrinkFactor);
  
  // Calculate the size of the target bitmap for the
  int shrunk_width  = g->x2 + (int)((g->bitmap.w-g->x) / shrinkFactor + 0.5) + 1;
  int shrunk_height = g->y2 + (int)((g->bitmap.h-g->y) / shrinkFactor + 0.5) + 1;
  
  // Now calculate the size of the white border. This is some sort
  // of black magic. Don't modify unless you know what you are doing.
  int pre_rows = (int)((1.0 + g->y2)*shrinkFactor + 0.5) - g->y - 1;
  if (pre_rows < 0)
    pre_rows = 0;
  int post_rows = (int)(shrunk_height*shrinkFactor + 0.5) - g->bitmap.h;
  if (post_rows < 0)
    post_rows = 0;
  
  int pre_cols = (int)((1.0 + g->x2)*shrinkFactor + 0.5) - g->x - 1;
  if (pre_cols < 0)
    pre_cols = 0;
  int post_cols = (int)(shrunk_width*shrinkFactor + 0.5) - g->bitmap.w;
  if (post_cols < 0)
    post_cols = 0;
  
  // Now shrinking may begin. Produce a QBitmap with the unshrunk
  // character.
  QBitmap bm(g->bitmap.bytes_wide*8, (int)g->bitmap.h, (const uchar *)(g->bitmap.bits), TRUE);
  // ... turn it into a Pixmap (highly inefficient, please improve)
  characterPixmaps[ch] = new QPixmap(g->bitmap.w+pre_cols+post_cols, g->bitmap.h+pre_rows+post_rows);
  if ((characterPixmaps[ch] == 0) || (characterPixmaps[ch]->isNull())) {
    kdError(4300) << "Could not properly allocate SmallChar in glyph::shrunkCharacter!" << endl;
    if (characterPixmaps[ch] != 0)
      delete characterPixmaps[ch];
    characterPixmaps[ch] = 0;
    return 0;
  }
  
  if (!bm.isNull()) {
    QPainter paint(characterPixmaps[ch]);
    paint.setBackgroundColor(Qt::white);
    paint.setPen( Qt::black );
    paint.fillRect(0,0,g->bitmap.w+pre_cols+post_cols, g->bitmap.h+pre_rows+post_rows, Qt::white);
    paint.drawPixmap(pre_cols, pre_rows, bm);
    paint.end();
  } else 
    kdError(4300) << "Null Bitmap in glyph::shrunkCharacter encountered!" << endl;
  
  // Generate an Image and shrink it to the proper size. By the
  // documentation of smoothScale, the resulting Image will be
  // 8-bit.
  QImage im = characterPixmaps[ch]->convertToImage().smoothScale(shrunk_width, shrunk_height);
  // Generate the alpha-channel. This again is highly inefficient.
  // Would anybody please produce a faster routine?
  QImage im32 = im.convertDepth(32);
  im32.setAlphaBuffer(TRUE);
  for(int y=0; y<im.height(); y++) {
    QRgb *imag_scanline = (QRgb *)im32.scanLine(y);
    for(int x=0; x<im.width(); x++) {
      // Make White => Transparent
      if ((0x00ffffff & *imag_scanline) == 0x00ffffff)
	*imag_scanline &= 0x00ffffff;
      else
	*imag_scanline |= 0xff000000;
      imag_scanline++; // Disgusting pointer arithmetic. Should be forbidden.
    }
  }
  characterPixmaps[ch]->convertFromImage(im32,0);
  characterPixmaps[ch]->setOptimization(QPixmap::BestOptim);

  return *(characterPixmaps[ch]);
}


/** Returns a pointer to glyph number ch in the font, or NULL, if this
    number does not exist. This function first reads the bitmap of the
    character from the PK-file, if necessary */

glyph *font::glyphptr(unsigned int ch) {

  // Paranoid checks
  if (ch > max_num_of_chars_in_font) {
    kdError(4300) << "Tried to access character with number " << ch << endl;
    return 0;
  }
  if (glyphtable == 0) {
    kdError(4300) << "Tried to access glyphptr() for non-character font!" << endl;
    return 0;
  }
  
  struct glyph *g = glyphtable+ch;
  if (g->bitmap.bits == NULL) {
    if (g->addr == 0) {
      kdError() << i18n("Character %1 not defined in font %2").arg(ch).arg(fontname) << endl;
      g->addr = -1;
      return NULL;
    }
    if (g->addr == -1)
      return NULL;	/* previously flagged missing char */

    if (file == NULL) {
      file = fopen(QFile::encodeName(filename), "r");
      if (file == NULL) {
	oops(i18n("Font file disappeared: %1").arg(filename) );
	return NULL;
      }
    }
    fseek(file, g->addr, 0);
    read_PK_char(ch);

    if (g->bitmap.bits == NULL) {
      g->addr = -1;
      return NULL;
    }
  }

  return g;
}
#include "font.moc"
