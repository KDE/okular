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
#include <kmimetype.h>
#include <kpassdlg.h>
#include <kprinter.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kapplication.h>
#include <krun.h>
#include <kurl.h>
#include <kuserprofile.h>
#include <string.h>

// local includes
#include "ErrorCodes.h"
#include "PDFDoc.h"
#include "PSOutputDev.h"
#include "QOutputDev.h"

#include "kpdf_error.h"
#include "document.h"
#include "page.h"
#include "settings.h"

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
    QRect currentViewport;
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
    d->searchPage = -1;
    // load paper color from Settings
    QColor col = Settings::paperColor();
    SplashColor paperColor;
    paperColor.rgb8 = splashMakeRGB8( col.red(), col.green(), col.blue() );
    // create the output device
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
    // TODO import overlay layers from XML
    QString fileName = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
    fileName = "kpdf/" + QString::number(fileSize) + "." + fileName + ".xml";
    QString localFN = locateLocal( "data", fileName );
    //kdWarning() << localFN << endl;

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

    //kdDebug() << "id: " << id << " is requesting pixmap for page " << page << " [" << width << " x " << height << "]." << endl;
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
            // generate links and activeRects if rendering pages on pageview
            bool genLinks = id == PAGEVIEW_ID;
            d->kpdfOutputDev->setParams( width, height, genTextPage, genLinks, genLinks );

            d->docLock.lock();
            d->pdfdoc->displayPage( d->kpdfOutputDev, page + 1, fakeDpiX, fakeDpiY, 0, true, genLinks );
            d->docLock.unlock();

            kp->setPixmap( id, d->kpdfOutputDev->takePixmap() );
            if ( genTextPage )
                kp->setSearchPage( d->kpdfOutputDev->takeTextPage() );
            if ( genLinks )
            {
                kp->setLinks( d->kpdfOutputDev->takeLinks() );
                kp->setActiveRects( d->kpdfOutputDev->takeActiveRects() );
            }

            d->observers[id]->notifyPixmapChanged( page );
        }
    }
    else
    {
        //TODO asyncronous events queuing
    }
}

void KPDFDocument::requestTextPage( uint n )
{
    KPDFPage * page = d->pages[ n ];
    // build a TextPage using the lightweight KPDFTextDev generator..
    KPDFTextDev td;
    d->docLock.lock();
    d->pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, true, false );
    d->docLock.unlock();
    // ..and attach it to the page
    page->setSearchPage( td.takeTextPage() );
}

bool KPDFDocument::okToPrint() const
{
    return d->pdfdoc->okToPrint();
}

bool KPDFDocument::print(KPrinter &printer)
{
    KTempFile tf( QString::null, ".ps" );
    PSOutputDev *psOut = new PSOutputDev(tf.name().latin1(), d->pdfdoc->getXRef(), d->pdfdoc->getCatalog(), 1, d->pdfdoc->getNumPages(), psModePS);

    if (psOut->isOk())
    {
        std::list<int> pages;

        if (!printer.previewOnly())
        {
            QValueList<int> pageList = printer.pageList();
            QValueList<int>::const_iterator it;

            for(it = pageList.begin(); it != pageList.end(); ++it) pages.push_back(*it);
        }
        else
        {
            for(int i = 1; i <= d->pdfdoc->getNumPages(); i++) pages.push_back(i);
        }

        d->docLock.lock();
        d->pdfdoc->displayPages(psOut, pages, 72, 72, 0, globalParams->getPSCrop(), gFalse);
        d->docLock.unlock();

        // needs to be here so that the file is flushed, do not merge with the one
        // in the else
        delete psOut;
        printer.printFiles(tf.name(), true);
        return true;
    }
    else
    {
        delete psOut;
        return false;
    }
}

// BEGIN slots
void KPDFDocument::slotSetCurrentPage( int page, const QRect & viewport )
{
    if ( page < 0 )
        page = 0;
    else if ( page > (int)d->pages.count() )
        page = d->pages.count() - 1;
    if ( page == d->currentPage && viewport == d->currentViewport )
        return;
    d->currentPage = page;
    d->currentViewport = viewport;
    foreachObserver( pageSetCurrent( page, viewport ) );
    pageChanged();
}

void KPDFDocument::slotSetFilter( const QString & pattern, bool keepCase )
{
    d->filterText = pattern;
    d->filterCase = keepCase;
    processPageList( false );
}

void KPDFDocument::slotToggleBookmark( int n )
{
    KPDFPage * page = ( n < (int)d->pages.count() ) ? d->pages[ n ] : 0;
    if ( page )
    {
        page->toggleAttribute( KPDFPage::Bookmark );
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
            lastPage->clearAttribute( KPDFPage::Highlight );
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
                requestTextPage( page->number() );
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
        foundPage->setAttribute( KPDFPage::Highlight );
        slotSetCurrentPage( pageNumber );
        foreachObserver( notifyPixmapChanged( pageNumber ) );
    }
    else
        KMessageBox::information( 0, i18n("No matches found for '%1'.").arg(d->searchText) );
}

void KPDFDocument::slotProcessLink( const KPDFLink * link )
{
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
            GString temp( namedDest );
            d->docLock.lock();
            dest = d->pdfdoc->findDest( &temp  );
            d->docLock.unlock();
        }
        if ( dest && dest->isOk() )
        {
            // get destination page number
            int pageNum = dest->getPageNum() - 1;
            if ( dest->isPageRef() )
            {
                Ref ref = dest->getPageRef();
                d->docLock.lock();
                pageNum = d->pdfdoc->findPage( ref.num, ref.gen ) - 1;
                d->docLock.unlock();
            }
            // get destination position
            QRect r;
            //KPDFPage * page = (pageNum >= 0 && pageNum < (int)d->pages.count()) ? d->pages[ pageNum ] : 0;
            //if ( page )
            /* TODO
            switch ( dest->getKind() )
            {
            case destXYZ
                OD -> cvtUserToDev( dest->getLeft(), dest->getTop(), &X, &Y );ù
                if ( dest->getChangeLeft() )
                    make hor change
                if ( dest->getChangeTop() )
                    make ver change
                if ( dest->getChangeZoom() )
                    make zoom change
            case destFit
            case destFitB
                fit page
            case destFitH
            case destFitBH
                read top, fit Width
            case destFitV
            case destFitBV
                read left, fit Height
            destFitR
                read and fit left,bottom,right,top
            }*/
            slotSetCurrentPage( pageNum, r );
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
			KMimeType::Ptr mime;
			KService::Ptr ptr;

			// the only pdf i have that has that kind of link don't define an application
			// and use the fileName as the file to open

			fileName = giveAbsolutePath( fileName );
			mime = KMimeType::findByPath( fileName );
			// Check executables
			if ( KRun::isExecutableFile( fileName, mime->name() ) )
			{
				// Don't have any pdf that uses this code path, just a guess on how it should work
				if ( link->getParameters() )
				{
					fileName = giveAbsolutePath( link->getParameters() );
					mime = KMimeType::findByPath( fileName );
					if ( KRun::isExecutableFile( fileName, mime->name() ) )
					{
						// this case is a link pointing to an executable with a parameter
						// that also is an executable, possibly a hand-crafted pdf
						KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
						return;
					}
				}
				else
				{
					// this case is a link pointing to an executable with no parameters
					// core developers find unacceptable executing it even after asking the user
					KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
					return;
				}
			}

			ptr = KServiceTypeProfile::preferredService(mime->name(), "Application");
			if (ptr)
			{
				KURL::List lst;
				lst.append( fileName );
				KRun::run( *ptr, lst );
			}
			else KMessageBox::information( 0, i18n("No application found for opening file of mimetype %1.").arg(mime->name()) );
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

QString KPDFDocument::giveAbsolutePath( const QString & fileName )
{
	const char * currentName = d->pdfdoc->getFileName()->getCString();
	if ( !currentName || currentName[0] == 0 )
		return QString::null;

    // convert the pdf fileName to absolute using current pdf path
	QFileInfo currentInfo( currentName );
	return currentInfo.dir().absFilePath( fileName );
}

bool KPDFDocument::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isNull() )
        return false;

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
            page->clearAttribute( KPDFPage::Highlight );
            if ( d->filterText.length() > 2 )
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
                if ( page->hasText( d->filterText, d->filterCase, true ) )
                    page->setAttribute( KPDFPage::Highlight );
            }
        }
    }

    // send the list to observers
    foreachObserver( pageSetup( d->pages, documentChanged ) );
}

void KPDFDocument::unHilightPages()
{
    if ( d->filterText.isEmpty() )
        return;

    d->filterText = QString::null;
    QValueVector<KPDFPage*>::iterator it = d->pages.begin(), end = d->pages.end();
    for ( ; it != end; ++it )
    {
        KPDFPage * page = *it;
        if ( page->attributes() & KPDFPage::Highlight )
        {
            page->clearAttribute( KPDFPage::Highlight );
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
