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

#ifndef QOUTPUTDEVKPRINTER_H
#define OUTPUTDEVKPRINTER_H

#include "QOutputDev.h"

class KPrinter;

class QOutputDevKPrinter : public QOutputDev
{
public:
        QOutputDevKPrinter(QPainter& painter, SplashColor paperColor, KPrinter& printer);
        virtual ~QOutputDevKPrinter();

	//----- initialization and control

	// Start a page.
	virtual void startPage(int pageNum, GfxState *state);

	// End a page
	virtual void endPage();

private:
        KPrinter& m_printer;            // the printer that we're drawing to
};

#endif
