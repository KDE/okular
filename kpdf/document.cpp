/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qfile.h>
#include <qmutex.h>
#include <qvaluevector.h>
#include <qmap.h>
#include <kdebug.h>

// local includes
#include "PDFDoc.h"
#include "QOutputDev.h"
//#include "TextOutputDev.h"

#include "kpdf_error.h"
#include "document.h"
#include "page.h"

/* Notes:
- FIXME event queuing to avoid flow interruption (!!??) maybe avoided by the
  warning to not call something 'active' inside an observer method.
- TODO implement filtering (on: "incremental search", "annotated pages", 
*/

// structure used internally by KPDFDocument for data storage
class KPDFDocumentPrivate
{
public:
    // document related
    QMutex docLock;
    PDFDoc * pdfdoc;
    KPDFOutputDev * kpdfOutputDev;
    int currentPage;
    float currentPosition;
    QValueVector< KPDFPage* > pages;

    // observers related (note: won't delete oservers)
    QMap< int, KPDFDocumentObserver* > observers;
};

#define foreachObserver( cmd ) {\
    QMap<int,KPDFDocumentObserver*>::iterator it = d->observers.begin();\
    QMap<int,KPDFDocumentObserver*>::iterator end = d->observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

/*
 * KPDFDocument class
 */
KPDFDocument::KPDFDocument()
{
    d = new KPDFDocumentPrivate;
    d->pdfdoc = 0;
    d->currentPage = -1;
    d->currentPosition = 0;
    SplashColor paperColor;
    paperColor.rgb8 = splashMakeRGB8( 0xff, 0xff, 0xff );
    d->kpdfOutputDev = new KPDFOutputDev( paperColor );
}

KPDFDocument::~KPDFDocument()
{
    close();
    delete d->kpdfOutputDev;
    delete d;
}

bool KPDFDocument::openFile( const QString & docFile )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( IO_ReadOnly ) )
        return false;
    fileReadTest.close();

    GString *filename = new GString( QFile::encodeName( docFile ) );
    delete d->pdfdoc;
    d->pdfdoc = new PDFDoc( filename, 0, 0 );
    deletePages();

    if ( !d->pdfdoc->isOk() )
    {
        delete d->pdfdoc;
        d->pdfdoc = 0;
        return false;
    }

    // clear xpdf errors
    errors::clear();

    // initialize output device for rendering current pdf
    d->kpdfOutputDev->startDoc( d->pdfdoc->getXRef() );

    // build Pages (currentPage was set -1 by deletePages)
    uint pageCount = d->pdfdoc->getNumPages();
    d->pages.resize( pageCount );
    if ( pageCount > 0 )
    {
        for ( uint i = 0; i < pageCount ; i++ )
            d->pages[i] = new KPDFPage( i, d->pdfdoc->getPageWidth(i+1), d->pdfdoc->getPageHeight(i+1), d->pdfdoc->getPageRotate(i+1) );
        // filter pages, setup observers and set the first page as current
        sendFilteredPageList();
        slotSetCurrentPage( 0 );
    }

    return true;
}

void KPDFDocument::close()
{
    //stopRunningJobs()...
    deletePages();
    delete d->pdfdoc;
    d->pdfdoc = 0;
}


uint KPDFDocument::currentPage() const
{
    return d->currentPage;
}

uint KPDFDocument::pages() const
{
    return d->pdfdoc ? d->pdfdoc->getNumPages() : 0;
}

bool KPDFDocument::atBegin() const
{
    return d->currentPage < 1;
}

bool KPDFDocument::atEnd() const
{
    return d->currentPage >= ((int)d->pages.count() - 1);
}

const KPDFPage * KPDFDocument::page( uint n ) const
{
    return ( n < d->pages.count() ) ? d->pages[n] : 0;
}


void KPDFDocument::addObserver( KPDFDocumentObserver * pObserver )
{
	d->observers[ pObserver->observerId() ] = pObserver;
}

void KPDFDocument::requestPixmap( int id, uint page, int width, int height, bool syn )
{
    KPDFPage * kp = d->pages[page];
    if ( !d->pdfdoc || !kp || kp->width() < 1 || kp->height() < 1 )
        return;

    if ( syn )
    {
        // in-place Pixmap generation for syncronous requests
        if ( !kp->hasPixmap( id, width, height ) )
        {
            // set KPDFPage pointer to outputdevice for links/text harvesting
            d->kpdfOutputDev->setParams( width, height, false );

            // compute dpi used to get an image with desired width and height
            double fakeDpiX = width * 72.0 / kp->width(),
                   fakeDpiY = height * 72.0 / kp->height();
            d->docLock.lock();
            d->pdfdoc->displayPage( d->kpdfOutputDev, page + 1, fakeDpiX, fakeDpiY, 0, true, true );
            d->docLock.unlock();

            kp->setPixmap( id, d->kpdfOutputDev->takePixmap() );

            d->observers[id]->notifyPixmapChanged( page );
        }
    }
    else
    {
        //TODO asyncronous events queuing
    }
}

// BEGIN slots 
void KPDFDocument::slotSetCurrentPage( int page )
{
    slotSetCurrentPagePosition( page, 0.0 );
}

void KPDFDocument::slotSetCurrentPagePosition( int page, float position )
{
    if ( page == d->currentPage && position == d->currentPosition )
        return;
    d->currentPage = page;
    d->currentPosition = position;
    foreachObserver( pageSetCurrent( page, position ) );
    pageChanged();
}

void KPDFDocument::slotFind( bool /*nextMatch*/, const QString & /*text*/ )
{
/*
  TextOutputDev *textOut;
  Unicode *u;
  bool found;
  double xMin1, yMin1, xMax1, yMax1;
  int len, pg;

  // This is more or less copied from what xpdf does, surely can be optimized
  len = s.length();
  u = (Unicode *)gmalloc(len * sizeof(Unicode));
  for (int i = 0; i < len; ++i) u[i] = (Unicode)(s.latin1()[i] & 0xff);

  // search current
  found = m_outputDev->find(u, len, next);

  if (!found)
  {
    // search following pages
    textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
    if (!textOut->isOk())
    {
      gfree(u);
      delete textOut;
      return;
    }

    pg = m_currentPage + 1;
    while(!found && pg <= d->pdfdoc->getNumPages())
    {
      m_docMutex.lock();
      d->pdfdoc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
      m_docMutex.unlock();
      found = textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse, &xMin1, &yMin1, &xMax1, &yMax1);
      if (!found) pg++;
    }

    if (!found && m_currentPage != 1)
    {
      if (KMessageBox::questionYesNo(widget(), i18n("End of document reached.\nContinue from the beginning?")) == KMessageBox::Yes)
      {
        // search previous pages
        pg = 1;
        while(!found && pg < m_currentPage)
        {
          m_docMutex.lock();
          d->pdfdoc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
          m_docMutex.unlock();
          found = textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse, &xMin1, &yMin1, &xMax1, &yMax1);
          if (!found) pg++;
        }
      }
    }

    delete textOut;
    if (found)
    {
       kdDebug() << "found at " << pg << endl;
       goToPage(pg);
       // xpdf says: can happen that we do not find the text if coalescing is bad OUCH
       //FIXME Enrico: expanded "m_outputDev(the widget)->find(u, len, false);" above:
    bool PageWidget::find( Unicode * u, int len, bool next )
    {return false;  TODO !!restore!! Enrico
        bool b;
        if (!next)
        {
            // ensure we are searching the whole page
            m_xMin = 0;
            m_yMin = 0;
        }

        b = m_outputdev(a QOut..) -> find(u, len, !next, true, next, false, &m_xMin, &m_yMin, &m_xMax, &m_yMax);
        m_xMin = m_xMin / m_zoomFactor;
        m_yMin = m_yMin / m_zoomFactor;
        m_xMax = m_xMax / m_zoomFactor;
        m_yMax = m_yMax / m_zoomFactor;
        m_selection = b;
        updateContents();
        return b;
    }
        // expanded ends here
    }
    else
    {
        if (next) KMessageBox::information(widget(), i18n("No more matches found for '%1'.").arg(s));
        else KMessageBox::information(widget(), i18n("No matches found for '%1'.").arg(s));
    }
  }

  if (found) m_findText = s;
  else m_findText = QString::null;

  gfree(u);
*/
}

void KPDFDocument::slotGoToLink( /* QString anchor */ )
{
}
//END slots 

void KPDFDocument::sendFilteredPageList( bool forceEmpty )
{
    // make up a value list of the pages [1,2,3..]
    uint pageCount = d->pages.count();
    QValueList<int> pagesList;
    if ( !forceEmpty )
        for ( uint i = 0; i < pageCount ; i++ )
            pagesList.push_back( i );

    // send the list to observers
    foreachObserver( pageSetup( pagesList ) );
}

void KPDFDocument::deletePages()
{
    if ( d->pages.isEmpty() )
        return;

    // broadcast an empty page list to observers
    sendFilteredPageList( true );

    // delete pages and clear container
    for ( uint i = 0; i < d->pages.count() ; i++ )
        delete d->pages[i];
    d->pages.clear();
    d->currentPage = -1;
}

/** TO BE IMPORTED:
	void generateThumbnails(PDFDoc *doc);
	void stopThumbnailGeneration();
protected slots:
	void customEvent(QCustomEvent *e);
private slots:
	void changeSelected(int i);
	void emitClicked(int i);
signals:
	void clicked(int);
private:
	void generateNextThumbnail();
	ThumbnailGenerator *m_tg;

	void resizeThumbnails();
	int m_nextThumbnail;
	bool m_ignoreNext;

DELETE:
if (m_tg)
{
	m_tg->wait();
	delete m_tg;
}

void ThumbnailList::generateThumbnails(PDFDoc *doc)
{
	m_nextThumbnail = 1;
	m_doc = doc;
	generateNextThumbnail();
}

void ThumbnailList::generateNextThumbnail()
{
	if (m_tg)
	{
		m_tg->wait();
		delete m_tg;
	}
	m_tg = new ThumbnailGenerator(m_doc, m_docMutex, m_nextThumbnail, QPaintDevice::x11AppDpiX(), this);
	m_tg->start();
}


void ThumbnailList::stopThumbnailGeneration()
{
	if (m_tg)
	{
		m_ignoreNext = true;
		m_tg->wait();
		delete m_tg;
		m_tg = 0;
	}
}


void ThumbnailList::customEvent(QCustomEvent *e)
{
	if (e->type() == 65432 && !m_ignoreNext)
	{
		QImage *i =  (QImage*)(e -> data());
		
		setThumbnail(m_nextThumbnail, i);
		m_nextThumbnail++;
		if (m_nextThumbnail <= m_doc->getNumPages()) generateNextThumbnail();
		else
		{
			m_tg->wait();
			delete m_tg;
			m_tg = 0;
		}
	}
	m_ignoreNext = false;
}
*/


#include "document.moc"
