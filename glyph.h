
#include <qbitmap.h>
#include <qimage.h>
#include <qpainter.h>


/*
 * Bitmap structure for raster ops.
 */
struct bitmap {
  unsigned short	w, h;		/* width and height in pixels */
  short		bytes_wide;	/* scan-line width in bytes */
  char		*bits;		/* pointer to the bits */
};

/*
 * Per-character information.
 * There is one of these for each character in a font (raster fonts only).
 * All fields are filled in at font definition time,
 * except for the bitmap, which is "faulted in"
 * when the character is first referenced.
 */

class glyph {
 public:
  ~glyph();

  void clearShrunkCharacter();
  QPixmap shrunkCharacter();

  long addr;		/* address of bitmap in font file */
  long dvi_adv;		/* DVI units to move reference point */
  short x, y;		/* x and y offset in pixels */
  // TODO: replace the bitmap by a Qbitmap
  struct bitmap bitmap;	/* bitmap for character */
  short x2, y2;		/* x and y offset in pixels (shrunken bitmap) */
  struct QPixmap *SmallChar; // shrunken bitmap for character 
};
