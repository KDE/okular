// fontEncodingPool.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <../config.h>
#ifdef HAVE_FREETYPE


#include "fontEncodingPool.h"

fontEncodingPool::fontEncodingPool(void)
{
}

fontEncoding *fontEncodingPool::findByName(const QString &name)
{
  fontEncoding *ptr = dictionary.find( name );
  
  if (ptr == 0) {
    ptr = new fontEncoding(name);
    dictionary.insert(name, ptr );
  } 
  
  return ptr;
}

#endif // HAVE_FREETYPE
