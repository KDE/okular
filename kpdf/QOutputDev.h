/* This file is part of the KDE libraries
   Copyright (C) 2004 Albert Astals Cid <tsdgeos@terra.es>
   Copyright (C) 2001, 2003 Christophe Devriese <oelewapperke@kde.org>
   Copyright 1996 Derek B. Noonburg

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

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
		QOutputDev(QPainter *p, SplashColor paperColor);
		
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
	
	protected:
		QPainter *m_painter;
	
	private:
		void draw();
		
		TextPage *m_text;		// text from the current page
};

#endif
