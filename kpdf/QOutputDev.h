/***************************************************************************
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Helio Chissini de Castro                        *
 *                           <helio@conectiva.com.br>                      *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef QOUTPUTDEV_H
#define QOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include "XRef.h"
#include "SplashOutputDev.h"

class TextPage;

class QPainter;

//------------------------------------------------------------------------
// QOutputDev
//------------------------------------------------------------------------

class QOutputDev : public SplashOutputDev
{
	public:
		// Constructor
		QOutputDev(SplashColor paperColor);
		
		// Destructor.
		virtual ~QOutputDev();
		
		// Start a page.
		virtual void startPage(int pageNum, GfxState *state);
		
		// End a page.
		virtual void endPage();
		
		//----- update text state
		virtual void updateFont(GfxState *state);
		
		//----- text drawing
		virtual void drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen);
		virtual GBool beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen);
		
		// Clear out the document (used when displaying an empty window).
		void clear();
		
		bool find(Unicode *s, int len, GBool startAtTop, GBool stopAtBottom, GBool startAtLast, GBool stopAtLast, double *xMin, double *yMin, double *xMax, double *yMax);
	
	private:
		
		TextPage *m_text;		// text from the current page
};

#endif
