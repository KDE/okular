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

#include "documentWidget.h"
#include "faxrenderer.h"
#include "faxmultipage.h"

//#define KF_DEBUG

FaxRenderer::FaxRenderer(QWidget* par)
  : DocumentRenderer(par)
{
#ifdef KF_DEBUG
  kdError() << "FaxRenderer( parent=" << par << " )" << endl;
#endif
}



FaxRenderer::~FaxRenderer()
{
#ifdef KF_DEBUG
  kdDebug() << "~FaxRenderer" << endl;
#endif

  // Wait for all access to this documentRenderer to finish
  mutex.lock();
  mutex.unlock();
}


void FaxRenderer::drawPage(double resolution, RenderedDocumentPage* page)
{
#ifdef KF_DEBUG
  kdDebug() << "FaxRenderer::drawPage(documentPage*) called, page number " << page->getPageNumber() << endl;
#endif

  // Paranoid safety checks
  if (page == 0) {
    kdError() << "FaxRenderer::drawPage(documentPage*) called with argument == 0" << endl;
    return;
  }
  if (page->getPageNumber() == 0) {
    kdError() << "FaxRenderer::drawPage(documentPage*) called for a documentPage with page number 0" << endl;
    return;
  }

  // Wait for all access to this documentRenderer to finish
  mutex.lock();

  // more paranoid safety checks
  if (page->getPageNumber() > numPages) {
    kdError() << "FaxRenderer::drawPage(documentPage*) called for a documentPage with page number " << page->getPageNumber()
	      << " but the current fax file has only " << numPages << " pages." << endl;
    mutex.unlock();
    return;
  }

  QImage img = fax.page(page->getPageNumber() - 1);

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

      img = img.smoothScale(width_in_pixel, height_in_pixel);
      foreGroundPaint->drawImage(0, 0, img);
      page->returnPainter(foreGroundPaint);
    }
  } else
    kdError() << "FaxRenderer::drawPage() called, but page size for page " << page->getPageNumber() << " is invalid." << endl;
  
  // To indicate that the page was drawn, we set the appropriate flas in the page structure
  page->isEmpty = false;
  
  mutex.unlock();
}


bool FaxRenderer::setFile(const QString &fname)
{
#ifdef KF_DEBUG
  kdDebug() << "FaxRenderer::setFile(" << fname << ") called" << endl;
#endif
  
  // Wait for all access to this documentRenderer to finish
  mutex.lock();
  
  // If fname is the empty string, then this means: "close".
  if (fname.isEmpty()) {
    kdDebug() << "FaxRenderer::setFile( ... ) called with empty filename. Closing the file." << endl;
    mutex.unlock();
    return true;
  }
  
  // Paranoid saftey checks: make sure the file actually exists, and
  // that it is a file, not a directory. Otherwise, show an error
  // message and exit..
  QFileInfo fi(fname);
  QString   filename = fi.absFilePath();
  if (!fi.exists() || fi.isDir()) {
    KMessageBox::error( parentWidget,
			i18n("<qt><strong>File error.</strong> The specified file '%1' does not exist.</qt>").arg(filename),
			i18n("File Error"));
    // the return value 'false' indicates that this operation was not successful.
    mutex.unlock();
    return false;
  }

  // Now we assume that the file is fine and load the file into the
  // fax member. We abort on error and give an error message.
  bool ok = fax.loadImage(filename);

  // It can happen that fax.loadImage() returns with 'ok == true', but
  // still the file could NOT be loaded. This happens, e.g. for TIFF
  // file that do NOT contain FAX, but other image formats. We handle
  // that case here also.
  if ( (!ok) || (fax.numPages() == 0)) {
    // Unfortunately, it can happen that fax.loadImage() fails WITHOUT
    // leaving an error message in fax.errorString(). We try to handle
    // this case gracefully.
    if (fax.errorString().isEmpty())
      KMessageBox::error( parentWidget,
			  i18n("<qt><strong>File error.</strong> The specified file '%1' could not be loaded.</qt>").arg(filename),
			  i18n("File Error"));
    else
      KMessageBox::detailedError( parentWidget,
				  i18n("<qt><strong>File error.</strong> The specified file '%1' could not be loaded.</qt>").arg(filename),
				  fax.errorString(),
				  i18n("File Error"));
    clear();
    mutex.unlock();
    return false;
  }

  // Set the number of pages page sizes
  numPages = fax.numPages();

  // Set the page size for the first page in the pageSizes array.
  // The rest of the page sizes will be calculated on demand by the drawPage function.
  pageSizes.resize(numPages);
  Length w,h;

  if (numPages != 0) {
    for(Q_UINT16 pg=0; pg < numPages; pg++) {
      QSize pageSize = fax.page_size(pg);
      QPoint dpi = fax.page_dpi(pg);
      double dpix = dpi.x();
      double dpiy = dpi.y();

      if (dpix*dpiy < 1.0) {
        kdError() << "File invalid resolutions, dpi x = " << dpix << ", dpi y = "  << dpiy << ". This information will be ignored and 75 DPI assumed." << endl;
        dpix = dpiy = 75.0;
      }

      w.setLength_in_inch(pageSize.width() / dpix);
      h.setLength_in_inch(pageSize.height() / dpiy);
      pageSizes[pg].setPageSize(w, h);
    }
  }

  // the return value 'true' indicates that this operation was not successful.
  mutex.unlock();
  return true;
}


#include "faxrenderer.moc"
