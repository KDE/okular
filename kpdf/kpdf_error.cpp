//========================================================================
//
// Error.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
// Copyright 2004 Albert Astals Cid
//
//========================================================================

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

#include <klocale.h>
#include <kmessagebox.h>

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
    emsg = i18n("Error (%d): ").arg(pos);
  } else {
    emsg = i18n("Error: ");
  }
  va_start(args, msg);
  tmsg = i18n(msg);
  vsprintf(buffer, tmsg.latin1(), args);
  va_end(args);
  emsg += buffer;
  if (!errors::exists(emsg))
  {
    KMessageBox::error(0, emsg);
    errors::add(emsg);
  }
}
