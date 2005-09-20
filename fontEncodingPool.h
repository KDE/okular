// -*- C++ -*-
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

#include "fontEncoding.h"

#include <qdict.h>

class QString;


class fontEncodingPool {
 public:
  fontEncodingPool();

  fontEncoding *findByName(const QString &name);

 private:
  QDict<fontEncoding> dictionary;
};

#endif
#endif // HAVE_FREETYPE
