// fontEncoding.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <../config.h>
#ifdef HAVE_FREETYPE


#ifndef _FONTENCODING_H
#define _FONTENCODING_H

class QString;

#include <qglobal.h>


class fontEncoding {
 public:
  fontEncoding(const QString &encName);

  QString encodingFullName;
  QString glyphNameVector[256];
};

#endif
#endif // HAVE_FREETYPE
