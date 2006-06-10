/***************************************************************************
 *   Copyright (C) 2005 by Stefan Kebekus                                  *
 *   kebekus@kde.org                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>
#include <qfileinfo.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <QX11Info>

#include "faxrenderer.h"
#include "core/page.h"

//#define KF_DEBUG
KPDF_EXPORT_PLUGIN(FaxRenderer)

FaxRenderer::FaxRenderer(KPDFDocument * doc) 
    :   Generator( doc )
{
    kWarning() << "fax generator has landed" <<endl;
}

void FaxRenderer::generatePixmap( PixmapRequest * request )
{
    mutex.lock();
    QString a="S";
    if (request->async) a="As";

    kWarning() << a << "ync Pixmaprequestuest of " << request->width << "x" 
    << request->height << " size, pageNo " << request->pageNumber 
    << ", priority: " << request->priority << " pageaddress " << (unsigned long long int) request->page
    <<  endl;

    // Wait for all access to this documentRenderer to finish

    QPixmap* pix = new QPixmap(request->width,request->height);
    pix->fill();
    QPainter p(pix);
    QImage img = fax.page(request->pageNumber).scaled(request->width,request->height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    p.drawImage( 0,0, img, 0,0,img.width(),img.height());
/*
  SimplePageSize psize = pageSizes[page->getPageNumber() - 1];
  if (psize.isValid()) {
    QPainter *foreGroundPaint = page->getPainter();
    if (foreGroundPaint != 0) {
      // Compute an image for the page.

      // WARNING: It may be tempting to compute the image size in
      // pixel, using page->height() and page->width(). DON'T DO
      // THAT. KViewShell uses transformations e.g. to rotate the
      // page, and sets the argument 'resolution' accordingly. Similar
      // problems occur if KViewShell required a shrunken version of
      // the page, e.g. to print multiple pages on one sheet of paper.

      int width_in_pixel = qRound(resolution * psize.width().getLength_in_inch());
      int height_in_pixel = qRound(resolution * psize.height().getLength_in_inch());

      img = img.scaled(width_in_pixel, height_in_pixel, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      foreGroundPaint->drawImage(0, 0, img);
      page->returnPainter(foreGroundPaint);
    }
  } else
    kError() << "FaxRenderer::drawPage() called, but page size for page " << page->getPageNumber() << " is invalid." << endl;
  
  // To indicate that the page was drawn, we set the appropriate flas in the page structure
  page->isEmpty = false;
  */

    mutex.unlock();
    request->page->setPixmap( request->id, pix );
    signalRequestDone( request );
}


bool FaxRenderer::loadDocument( const QString & fileName, QVector< KPDFPage * > & pagesVector )
{
#ifdef KF_DEBUG
  kDebug() << "FaxRenderer::setFile(" << fname << ") called" << endl;
#endif

  // Wait for all access to this documentRenderer to finish
//   mutex.lock();

  // Now we assume that the file is fine and load the file into the
  // fax member. We abort on error and give an error message.
  bool ok = fax.loadImage(fileName);
  kWarning(1000) << "fax " << fileName << " loaded ok: " << ok <<endl;
  // It can happen that fax.loadImage() returns with 'ok == true', but
  // still the file could NOT be loaded. This happens, e.g. for TIFF
  // file that do NOT contain FAX, but other image formats. We handle
  // that case here also.
  if ( (!ok) || (fax.numPages() == 0)) {
    // Unfortunately, it can happen that fax.loadImage() fails WITHOUT
    // leaving an error message in fax.errorString(). We try to handle
    // this case gracefully.
    QString temp;
    if (fax.errorString().isEmpty())
    {
        temp=i18n("The specified file '%1' could not be opened.", fileName);
        emit error (temp,-1);
    }
    else
    {
        temp=i18n("Error while opening file: %1.", fax.errorString());
        emit error (temp,-1);
    }
//     mutex.unlock();
    return false;
  }

  // Set the number of pages page sizes
  quint16 pages = fax.numPages();
  pagesVector.resize(pages);

    for(quint16 pg=0; pg < pages; pg++) 
    {
      QSize pageSize = fax.page_size(pg);
      // how about rotation?

      QPoint dpi = fax.page_dpi(pg);
      double dpix = dpi.x();
      double dpiy = dpi.y();

      if (dpix*dpiy < 1.0) {
        kError() << "File invalid resolutions, dpi x = " << dpix << ", dpi y = "  << dpiy << ". This information will be ignored and 75 DPI assumed." << endl;
        dpix = dpiy = 75.0;
      }
      pagesVector[pg] = new KPDFPage(pg, QX11Info::appDpiX () * pageSize.width() / dpix, 
        QX11Info::appDpiY () * pageSize.height()/dpiy,0);
  }

  // the return value 'true' indicates that this operation was not successful.
//   mutex.unlock();
  return true;
}


#include "faxrenderer.moc"
