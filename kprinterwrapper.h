// kprinterwrapper.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL


#ifndef _PRINTERWRAPPER_H
#define _PRINTERWRAPPER_H

#include "kprinter.h"

class KPrinterWrapper : public KPrinter
{
public:
  KPrinterWrapper(void) : KPrinter(true, QPrinter::ScreenResolution) {; };

  void doPreparePrinting() { preparePrinting(); };
};

#endif
