// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// fontEncodingPool.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef _FONTENCODINGPOOL_H
#define _FONTENCODINGPOOL_H

#include "fontEncoding.h"

#include <Q3Dict>

class QString;


class fontEncodingPool {
 public:
  fontEncodingPool();

  fontEncoding *findByName(const QString &name);

 private:
  Q3Dict<fontEncoding> dictionary;
};

#endif
