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
#include "Error.h"

#include "kpdf_error.h"

#include <qstring.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "xpdf_errors.h"

QStringList errors::p_errors;

void errors::add(const QString &s)
{
	p_errors.append(s);
}

bool errors::exists(const QString &s)
{
	return p_errors.findIndex(s) != -1;
}

void errors::clear()
{
	p_errors.clear();
}

void CDECL error(int pos, const char *msg, ...) {
  va_list args;
  QString emsg, tmsg;
  char buffer[1024]; // should be big enough

  // NB: this can be called before the globalParams object is created
  if (globalParams && globalParams->getErrQuiet()) {
    return;
  }
  if (pos >= 0) {
    emsg = i18n("Error (%1): ").arg(pos);
  } else {
    emsg = i18n("Error: ");
  }
  va_start(args, msg);
  tmsg = XPDFErrorTranslator::translateError(msg);
  vsprintf(buffer, tmsg.latin1(), args);
  va_end(args);
  emsg += buffer;
  if (!errors::exists(emsg))
  {
  // TODO think a way to avoid threads can popup that
  //  KMessageBox::error(0, emsg);
    kdDebug() << emsg << endl;
    errors::add(emsg);
  }
}
