/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde/system includes
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qmutex.h>
#include <qvaluevector.h>
#include <qmap.h>
#include <kdebug.h>
#include <klocale.h>
#include <kfinddialog.h>
#include <kmessagebox.h>
#include <kpassdlg.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <krun.h>
#include <kurl.h>
#include <kuserprofile.h>
#include <string.h>

// local includes
#include "ErrorCodes.h"
#include "PDFDoc.h"
#include "QOutputDev.h"

#include "kpdf_error.h"
#include "document.h"
#include "page.h"

/* Notes:
- FIXME event queuing to avoid flow loops (!!??) maybe avoided by the
  warning to not call something 'active' inside an observer method.
*/

// structure used internally by KPDFDocument for local variables storage
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
    QString searchText;
    bool searchCase;
    int searchPage;
    // filtering related
    QString filterText;
    bool filterCase;

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
    d->searchPage = -1;
    SplashColor paperColor;
    paperColor.rgb8 = splashMakeRGB8( 0xff, 0xff, 0xff );
    d->kpdfOutputDev = new KPDFOutputDev( paperColor );
}

KPDFDocument::~KPDFDocument()
{
    closeDocument();
    delete d->kpdfOutputDev;
    delete d;
}

bool KPDFDocument::openDocument( const QString & docFile )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( IO_ReadOnly ) )
        return false;
    long fileSize = fileReadTest.size();
    fileReadTest.close();

    // free internal data
    closeDocument();

    GString *filename = new GString( QFile::encodeName( docFile ) );
    delete d->pdfdoc;
    d->pdfdoc = new PDFDoc( filename, 0, 0 );

    if ( !d->pdfdoc->isOk() )
    {
        if (d->pdfdoc->getErrorCode() == errEncrypted)
        {
            bool first, correct;
            QString prompt;
            QCString pwd;

            first = true;
            correct = false;
            while(!correct)
            {
                if (first)
                {
                    prompt = i18n("Please insert the password to read the document:");
                    first = false;
                }
                else prompt = i18n("Incorrect password. Try again:");
                if (KPasswordDialog::getPassword(pwd, prompt) == KPasswordDialog::Accepted)
                {
                    GString *pwd2;
                    pwd2 = new GString(pwd.data());
                    d->pdfdoc = new PDFDoc(filename, pwd2, pwd2);
                    delete pwd2;
                    correct = d->pdfdoc->isOk();
                    if (!correct && d->pdfdoc->getErrorCode() != errEncrypted)
                    {
                        delete d->pdfdoc;
                        d->pdfdoc = 0;
                        return false;
                    }
                }
                else
                {
                    delete d->pdfdoc;
                    d->pdfdoc = 0;
                    return false;
                }
            }
        }
        else
        {
            delete d->pdfdoc;
            d->pdfdoc = 0;
            return false;
        }
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
        processPageList( true );
        slotSetCurrentPage( 0 );
    }

    // check local directory for an overlay xml
    QString fileName = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
    fileName = "kpdf/" + QString::number(fileSize) + "." + fileName + ".xml";
    QString localFN = locateLocal( "data", fileName );
    kdWarning() << localFN << endl;

    return true;
}

void KPDFDocument::closeDocument()
{
    // delete pages and clear container
    for ( uint i = 0; i < d->pages.count() ; i++ )
        delete d->pages[i];
    d->pages.clear();

    // broadcast zero pages
    processPageList( true );

    // reset internal variables
    d->currentPage = -1;
    d->searchPage = -1;

    // delete xpds's PDFDoc contents generator
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

Outline * KPDFDocument::outline() const
{
    return d->pdfdoc ? d->pdfdoc->getOutline() : 0;
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
            // generate links if rendering pages on pageview
            bool genLinks = id == PAGEVIEW_ID;
            d->kpdfOutputDev->setParams( width, height, genTextPage, genLinks );

            d->docLock.lock();
            d->pdfdoc->displayPage( d->kpdfOutputDev, page + 1, fakeDpiX, fakeDpiY, 0, true, genLinks );
            d->docLock.unlock();

            kp->setPixmap( id, d->kpdfOutputDev->takePixmap() );
            if ( genTextPage )
                kp->setSearchPage( d->kpdfOutputDev->takeTextPage() );
            if ( genLinks )
                kp->setLinks( d->kpdfOutputDev->takeLinks() );

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

void KPDFDocument::slotSetFilter( const QString & pattern, bool keepCase )
{
    d->filterText = pattern;
    d->filterCase = keepCase;
    processPageList( false );
}

void KPDFDocument::slotBookmarkPage( int n, bool on )
{
    KPDFPage * page = ( n < (int)d->pages.count() ) ? d->pages[ n ] : 0;
    if ( page )
    {
        page->bookmark( on );
        foreachObserver( notifyPixmapChanged( n ) );
    }
}

void KPDFDocument::slotFind( const QString & string, bool keepCase )
{
    // turn selection drawing off on filtered pages
    if ( !d->filterText.isEmpty() )
        unHilightPages();

    // save params for the 'find next' case
    if ( !string.isEmpty() )
    {
        d->searchText = string;
        d->searchCase = keepCase;
    }

    // continue checking last SearchPage first (if it is the current page)
    int currentPage = d->currentPage;
    int pageCount = d->pages.count();
    KPDFPage * foundPage = 0,
             * lastPage = (d->searchPage > -1) ? d->pages[ d->searchPage ] : 0;
    if ( lastPage && d->searchPage == currentPage )
        if ( lastPage->hasText( d->searchText, d->searchCase, false ) )
            foundPage = lastPage;
        else
        {
            lastPage->hilightLastSearch( false );
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
            if ( page->hasText( d->searchText, d->searchCase, true ) )
            {
                foundPage = page;
                break;
            }
            currentPage++;
        }

    if ( foundPage )
    {
        int pageNumber = foundPage->number();
        d->searchPage = pageNumber;
        foundPage->hilightLastSearch( true );
        slotSetCurrentPage( pageNumber );
        foreachObserver( notifyPixmapChanged( pageNumber ) );
    }
    else
        KMessageBox::information( 0, i18n("No matches found for '%1'.").arg(d->searchText) );
}

void KPDFDocument::slotProcessLink( int n, int x, int y )
{
    KPDFPage * page = ( n < (int)d->pages.count() ) ? d->pages[ n ] : 0;
    const KPDFLink * link = page ? page->getLink( x, y ) : 0;
    if ( !link )
        return;

    switch( link->type() )
    {
    case KPDFLink::Goto: {
        // make copies of the 'nameDest' (string) and the 'dest' (class)
        char * namedDest = 0;
        link->copyString( namedDest, link->getNamedDest() );
        LinkDest * dest = link->getDest() ? ((LinkDest*)link->getDest())->copy() : 0;

        // first open filename if link is pointing outside this document
        QString fileName( link->getFileName() );
        if ( !fileName.isNull() )
            if ( !openRelativeFile( fileName ) )
            {
                kdWarning() << "Link: Error opening '" << fileName << "'." << endl;
                delete dest;
                delete namedDest;
                return;
            }

        // now previous KPDFLink and KPDFPage don't exist anymore!
        if ( namedDest && !dest )
        {
            d->docLock.lock();
            GString temp( namedDest );
            dest = d->pdfdoc->findDest( &temp  );
            d->docLock.unlock();
        }
        if ( dest )
        {
            // TODO implement page traversal
            //pri NOWARN ntf("HERE I AM\n");
            //displayDest(dest, zoom, rotate, gTrue);
        }
        delete namedDest;
        delete dest;
        } break;

    case KPDFLink::Execute: {
        QString fileName( link->getFileName() );
        if ( fileName.endsWith( ".pdf" ) || fileName.endsWith( ".PDF" ) )
            openRelativeFile( fileName );
        else
        {
            KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
            /* core developers say this is too dangerous
            fileName = fileName->copy();
            if (((LinkLaunch *)action)->getParams())
            {
                fileName->append(' ');
                fileName->append(((LinkLaunch *)action)->getParams());
            }
            fileName->append(" &");
            if (KMessageBox::questionYesNo(widget(), i18n("Do you want to execute the command:\n%1").arg(fileName->getCString()), i18n("Launching external application")) == KMessageBox::Yes)
            {
                system(fileName->getCString());
            }*/
        }
        } break;

    case KPDFLink::Named: {
        const char * name = link->getName();
        if ( !strcmp( name, "NextPage" ) && (d->currentPage < (int)d->pages.count() - 1) )
            slotSetCurrentPage( d->currentPage + 1 );
        else if ( !strcmp( name, "PrevPage" ) && d->currentPage > 0 )
            slotSetCurrentPage( d->currentPage - 1 );
        else if ( !strcmp( name, "FirstPage" ) )
            slotSetCurrentPage( 0 );
        else if ( !strcmp( name, "LastPage" ) )
            slotSetCurrentPage( d->pages.count() - 1 );
        else if ( !strcmp( name, "GoBack" ) )
            {} //TODO
        else if ( !strcmp( name, "GoForward" ) )
            {} //TODO
        else if ( !strcmp( name, "Quit" ) )
            kapp->quit();
        else
            {}//FIXME error(-1, "Unknown named action: '%s'", name);
        } break;

    case KPDFLink::URI: {
        KService::Ptr ptr = KServiceTypeProfile::preferredService("text/html", "Application");
        KURL::List lst;
        lst.append( link->getURI() );
        KRun::run( *ptr, lst );
        } break;

    case KPDFLink::Movie:
    case KPDFLink::Unknown:
        // unimplemented cases
        break;
    }
}
//END slots

bool KPDFDocument::openRelativeFile( const QString & fileName )
{
    const char * currentName = d->pdfdoc->getFileName()->getCString();
    if ( !currentName || currentName[0] == 0 )
        return false;

    // convert the pdf fileName to absolute using current pdf path
    QFileInfo currentInfo( currentName );
    QString absFileName( currentInfo.dir().absFilePath( fileName ) );
    kdDebug() << "openDocument: '" << absFileName << "'" << endl;

    // open the absolute filename
    return openDocument( absFileName );
}

void KPDFDocument::processPageList( bool documentChanged )
{
    if ( d->filterText.length() < 3 )
        unHilightPages();
    else
    {
        uint pageCount = d->pages.count();
        for ( uint i = 0; i < pageCount ; i++ )
        {
            KPDFPage * page = d->pages[ i ];
            if ( d->filterText.length() < 3 )
                page->hilightLastSearch( false );
            else
            {
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
                bool found = page->hasText( d->filterText, d->filterCase, true );
                page->hilightLastSearch( found );
            }
        }
    }

    // send the list to observers
    foreachObserver( pageSetup( d->pages, documentChanged ) );
}

void KPDFDocument::unHilightPages()
{
    if ( d->filterText == "" )
        return;

    d->filterText = "";
    QValueVector<KPDFPage*>::iterator it = d->pages.begin(), end = d->pages.end();
    for ( ; it != end; ++it )
    {
        KPDFPage * page = *it;
        if ( page->isHilighted() )
        {
            page->hilightLastSearch( false );
            foreachObserver( notifyPixmapChanged( page->number() ) );
        }
    }
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
