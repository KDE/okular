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

#ifndef KPDFOUTPUTDEV_H
#define KPDFOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include <qimage.h>

#include "SplashOutputDev.h"
#include "Link.h"

class TextPage;
class KPDFPage;

/**
 * @short A SplashOutputDev rendered that grab text and links.
 *
 * This output device: 
 * - renders the page using SplashOutputDev (its parent)
 * - harvests text into a textPage (for searching text)
 * - harvests links and set them to a KPDFPage
 */
class KPDFOutputDev : public SplashOutputDev
{
public:
	KPDFOutputDev( SplashColor paperColor );
	virtual ~KPDFOutputDev();

	// to be called before PDFDoc->displayPage( thisclass, .. )
	void setParams( int pixmapWidth, int pixmapHeight, bool generateText );

	// takes pointers out of the class (so deletion it's up to others)
	QPixmap * takePixmap();
	TextPage * takeTextPage();

	/** inherited from OutputDev */
	// Start a page.
	virtual void startPage(int pageNum, GfxState *state);
	// End a page.
	virtual void endPage();
	//----- link borders
	virtual void drawLink(Link *link, Catalog *catalog);
	//----- update text state
	virtual void updateFont(GfxState *state);
	//----- text drawing
	virtual void drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen);
	virtual GBool beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen);

private:
	// the pixmap where the page is drawn (generated on every execution)
	int m_pageNum;
	int m_pixmapWidth;
	int m_pixmapHeight;
	QPixmap * m_pixmap;

	// text page generated on demand
	TextPage * m_text;
};

#endif
