/***************************************************************************
 *   Copyright (C) 1996-2003 Glyph & Cog, LLC                              *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include "xpdf/GlobalParams.h"
#include "xpdf/Error.h"

#include <qstring.h>

#include <kdebug.h>

void CDECL error(int pos, char *msg, ...) {
  va_list args;
  QString emsg, tmsg;
  char buffer[1024]; // should be big enough

  // NB: this can be called before the globalParams object is created
  if (globalParams && globalParams->getErrQuiet()) {
    return;
  }
  if (pos >= 0) {
    emsg = QString("Error (%1): ").arg(pos);
  } else {
    emsg = "Error: ";
  }
  va_start(args, msg);
  vsprintf(buffer, msg, args);
  va_end(args);
  emsg += buffer;
  kdDebug() << emsg << endl;
}
