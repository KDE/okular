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
#include "GlobalParams.h"

#include <qstring.h>

#include <kdebug.h>

static void CDECL kpdf_error(int pos, char *msg, va_list args) {
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
  vsprintf(buffer, msg, args);
  emsg += buffer;
  kDebug() << emsg << endl;
}
