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
#define QOUTPUTDEVKPRINTER_H

#include "XRef.h"
#include "SplashOutputDev.h"

class KPrinter;
class QPainter;

class QOutputDevKPrinter : public SplashOutputDev
{
	public:
		QOutputDevKPrinter(QPainter& painter, SplashColor paperColor, KPrinter& printer);
		virtual ~QOutputDevKPrinter();
		
		// Start a page.
		virtual void startPage(int pageNum, GfxState *state);
		
		// End a page
		virtual void endPage();
		
	private:
		KPrinter& m_printer;            // the printer that we're drawing to
		QPainter& m_painter;            // the painter that we're drawing to
};

#endif
