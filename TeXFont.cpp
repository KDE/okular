// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
#include <config.h>

#include "TeXFont.h"


TeXFont::~TeXFont()
{
  for(unsigned int i=0; i<TeXFontDefinition::max_num_of_chars_in_font; i++)
  {
    if (glyphtable[i].shrunkenCharacter)
    {
      cairo_surface_destroy(glyphtable[i].shrunkenCharacter);
      glyphtable[i].shrunkenCharacter = 0;
    }
  }
}
