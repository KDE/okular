
#ifndef _TEXFONT_PK_H
#define _TEXFONT_PK_H

#include <qstring.h>
#include <stdio.h>

#include "TeXFont.h"

class glyph;

class TeXFont_PK : public TeXFont {
 public:
  TeXFont_PK(TeXFontDefinition *parent);
  ~TeXFont_PK();
  
  glyph *getGlyph(unsigned int character, bool generateCharacterPixmap=false);

 private:
  FILE         *file;		// open font file or NULL
  class bitmap *characterBitmaps[TeXFontDefinition::max_num_of_chars_in_font];

  // For use by PK-decryption routines. I don't understand what these
  // are good for -- Stefan Kebekus
  int	         PK_flag_byte;
  unsigned       PK_input_byte;
  int	         PK_bitpos;
  int	         PK_dyn_f;
  int	         PK_repeat_count;

  // PK-internal routines which were taken from xdvi. Again, I do not
  // really know what they are good for -- Stefan Kebekus
  inline void read_PK_char(unsigned int ch);
  inline int  PK_get_nyb(FILE *fp);
  inline int  PK_packed_num(FILE *fp);
  inline void read_PK_index(void);
  inline void PK_skip_specials(void);
};

#endif
