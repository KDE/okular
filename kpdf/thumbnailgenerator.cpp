/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qapplication.h>
#include <qevent.h>
#include <qimage.h>
#include <qmutex.h>
 
#include "PDFDoc.h"

#include "thumbnailgenerator.h"

ThumbnailGenerator::ThumbnailGenerator(PDFDoc *doc, QMutex *docMutex, int page, double ppp, QObject *o) : m_doc(doc), m_docMutex(docMutex), m_page(page), m_o(o), m_ppp(ppp)
{
}

int ThumbnailGenerator::getPage() const
{
	return m_page;
}

void ThumbnailGenerator::run()
{
	QCustomEvent *ce;
	QImage *i;
	
	SplashColor paperColor;
	paperColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
	QOutputDevPixmap odev(paperColor);
	odev.startDoc(m_doc->getXRef());
	m_docMutex->lock();
	m_doc -> displayPage(&odev, m_page, m_ppp, m_ppp, 0, true, false);
	m_docMutex->unlock();
	ce = new QCustomEvent(65432);
	i = new QImage(odev.getImage());
	i -> detach();
	ce -> setData(i);
	QApplication::postEvent(m_o, ce);
}
