// QOutputDevPixmap.h

// Copyright (C)  2003  Christophe Devriese <oelewapperke@oelewapperke.org>

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307  USA

//========================================================================
//
// XOutputDev.h
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#ifndef QOUTPUTDEVPIXMAP_H
#define QOUTPUTDEVPIXMAP_H

#ifdef __GNUC__
#pragma interface
#endif

#include "QOutputDev.h"

class QPixmap;

class QOutputDevPixmap : public QOutputDev {
public:

	// Constructor.
	QOutputDevPixmap(SplashColor paperColor);

	// Destructor.
	virtual ~QOutputDevPixmap();

	//----- initialization and control

	// Start a page.
	virtual void startPage(int pageNum, GfxState *state);

	// End a page
	virtual void endPage();

public:
	QPixmap * getPixmap() const { return m_pixmap; };

private:
	QPixmap * m_pixmap;   		// pixmap to draw into
};

#endif // QOUTPUTDEVPIXMAP
