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
#include <klocale.h>
#include <kfinddialog.h>
#include <kmessagebox.h>

// local includes
#include "PDFDoc.h"
#include "QOutputDev.h"

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

    // find related
    QString lastSearchText;
    long lastSearchOptions;
    KPDFPage * lastSearchPage;

    // filtering related
    QString filterString;

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
    d->lastSearchPage = 0;
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
            // compute dpi used to get an image with desired width and height
            double fakeDpiX = width * 72.0 / kp->width(),
                   fakeDpiY = height * 72.0 / kp->height();

            // setup kpdf output device: text page is generated only if we are at 72dpi.
            // since we can pre-generate the TextPage at the right res.. why not?
            bool genTextPage = !kp->hasSearchPage() && (width == kp->width()) && (height == kp->height());
            d->kpdfOutputDev->setParams( width, height, genTextPage );

            d->docLock.lock();
            d->pdfdoc->displayPage( d->kpdfOutputDev, page + 1, fakeDpiX, fakeDpiY, 0, true, false/*dolinks*/ );
            d->docLock.unlock();

            kp->setPixmap( id, d->kpdfOutputDev->takePixmap() );
            if ( genTextPage )
                kp->setSearchPage( d->kpdfOutputDev->takeTextPage() );

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

void KPDFDocument::slotSetFilter( const QString & pattern )
{
    d->filterString = pattern;
    if ( pattern.length() > 3 )
        sendFilteredPageList();
}

void KPDFDocument::slotFind( const QString & t, long opt )
{
    // reload last options if in 'find next' case
    long options = t.isEmpty() ? d->lastSearchOptions : opt;
    QString text = t.isEmpty() ? d->lastSearchText : t;
    if ( !t.isEmpty() )
    {
        d->lastSearchText = t;
        d->lastSearchOptions = opt;
    }

    // check enabled options (only caseSensitive support until now)
    bool caseSensitive = options & KFindDialog::CaseSensitive;

    // continue checking last SearchPage first (if it is the current page)
    KPDFPage * foundPage = 0;
    int currentPage = d->currentPage;
    int pageCount = d->pages.count();
    if ( d->lastSearchPage && (int)d->lastSearchPage->number() == currentPage )
        if ( d->lastSearchPage->hasText( text, caseSensitive, false ) )
            foundPage = d->lastSearchPage;
        else
        {
            d->lastSearchPage->hilightLastSearch( false );
            currentPage++;
            pageCount--;
        }

    if ( !foundPage )
        // loop through the whole document
        for ( int i = 0; i < pageCount; i++ )
        {
            if ( currentPage >= pageCount )
            {
                if ( KMessageBox::questionYesNo(0, i18n("End of document reached.\nContinue from the beginning?")) == KMessageBox::Yes )
                    currentPage = 0;
                else
                    break;
            }
            KPDFPage * page = d->pages[ currentPage ];
            if ( !page->hasSearchPage() )
            {
                // build a TextPage using the lightweight KPDFTextDev generator..
                KPDFTextDev td;
                d->docLock.lock();
                d->pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, true, false );
                d->docLock.unlock();
                // ..and attach it to the page
                page->setSearchPage( td.takeTextPage() );
            }
            if ( page->hasText( text, caseSensitive, true ) )
            {
                foundPage = page;
                break;
            }
            currentPage++;
        }

    if ( foundPage )
    {
        d->lastSearchPage = foundPage;
        foundPage->hilightLastSearch( true );
        slotSetCurrentPage( foundPage->number() );
        foreachObserver( notifyPixmapChanged( foundPage->number() ) );
    }
    else
        KMessageBox::information( 0, i18n("No matches found for '%1'.").arg(text) );
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
    d->lastSearchPage = 0;
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
