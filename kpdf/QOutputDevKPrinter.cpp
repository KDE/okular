/***************************************************************************
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "QOutputDevKPrinter.h"

#include <kprinter.h>
#include <qpainter.h>

QOutputDevKPrinter::QOutputDevKPrinter(QPainter& painter, SplashColor paperColor, KPrinter& printer )
  : QOutputDev(&painter, paperColor), m_printer( printer )
{
}

QOutputDevKPrinter::~QOutputDevKPrinter()
{
}

void QOutputDevKPrinter::startPage(int page, GfxState *state)
{
  // TODO: page size ?
  QOutputDev::startPage( page, state);
}

void QOutputDevKPrinter::endPage()
{
  QOutputDev::endPage();
}

