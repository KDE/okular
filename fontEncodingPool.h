// fontEncodingPool.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <../config.h>
#ifdef HAVE_FREETYPE


#ifndef _FONTENCODINGPOOL_H
#define _FONTENCODINGPOOL_H

#include <kprocio.h>
#include <qdict.h>
#include <qstring.h>

#include "fontEncoding.h"


class fontEncodingPool {
 public:
  fontEncodingPool(void);

  fontEncoding *findByName(const QString &name);

 private:
  QDict<fontEncoding> dictionary;
};

#endif
#endif // HAVE_FREETYPE
