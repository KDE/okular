/*****************************  **********************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qimage.h>
#include <qregexp.h>
#include <kauthorized.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kwallet.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kdebug.h>

// local includes
#include "generator_pdf.h"
#include "core/observer.h" //for PAGEVIEW_ID
#include "core/page.h"
#include "core/annotations.h"
#include "core/pagetransition.h"
#include "settings.h"

#include <config.h>

// id for DATA_READY PDFPixmapGeneratorThread Event
#define TGE_DATAREADY_ID 6969

static void fillViewportFromLinkDestination( DocumentViewport &viewport, const Poppler::LinkDestination &destination, const Poppler::Document *pdfdoc )
{
    viewport.pageNumber = destination.pageNumber();

    if (viewport.pageNumber == -1) return;

    // get destination position
    // TODO add other attributes to the viewport (taken from link)
//     switch ( destination->getKind() )
//     {
//         case destXYZ:
            if (destination.isChangeLeft() || destination.isChangeTop())
            {
                Poppler::Page *page = pdfdoc->page( viewport.pageNumber );
                // TODO remember to change this if we implement DPI and/or rotation
                double left, top;
                left = destination.left();
                top = destination.top();

                QSize pageSize = page->pageSize();
                delete page;
                viewport.rePos.normalizedX = (double)left / (double)pageSize.width();
                viewport.rePos.normalizedY = (double)top / (double)pageSize.height();
                viewport.rePos.enabled = true;
                viewport.rePos.pos = DocumentViewport::TopLeft;
            }
            /* TODO
            if ( dest->getChangeZoom() )
                make zoom change*/
/*        break;

        default:
            // implement the others cases
        break;*/
//     }
}

static KPDFLink* createKPDFLinkFromPopplerLink(const Poppler::Link *popplerLink, const Poppler::Document *pdfdoc)
{
	KPDFLink *kpdfLink = 0;
	const Poppler::LinkGoto *popplerLinkGoto;
	const Poppler::LinkExecute *popplerLinkExecute;
	const Poppler::LinkBrowse *popplerLinkBrowse;
	const Poppler::LinkAction *popplerLinkAction;
	DocumentViewport viewport;
	
	switch(popplerLink->linkType())
	{
		case Poppler::Link::None:
		break;
	
		case Poppler::Link::Goto:
			popplerLinkGoto = static_cast<const Poppler::LinkGoto *>(popplerLink);
			fillViewportFromLinkDestination( viewport, popplerLinkGoto->destination(), pdfdoc );
			kpdfLink = new KPDFLinkGoto(popplerLinkGoto->fileName(), viewport);
		break;
		
		case Poppler::Link::Execute:
			popplerLinkExecute = static_cast<const Poppler::LinkExecute *>(popplerLink);
			kpdfLink = new KPDFLinkExecute( popplerLinkExecute->fileName(), popplerLinkExecute->parameters() );
		break;
		
		case Poppler::Link::Browse:
			popplerLinkBrowse = static_cast<const Poppler::LinkBrowse *>(popplerLink);
			kpdfLink = new KPDFLinkBrowse( popplerLinkBrowse->url() );
		break;
		
		case Poppler::Link::Action:
			popplerLinkAction = static_cast<const Poppler::LinkAction *>(popplerLink);
			kpdfLink = new KPDFLinkAction( (KPDFLinkAction::ActionType)popplerLinkAction->actionType() );
		break;
		
		case Poppler::Link::Movie:
			// not implemented
		break;
	}
	
	return kpdfLink;
}

static QLinkedList<ObjectRect*> generateKPDFLinks( const QList<Poppler::Link*> &popplerLinks, int width, int height, const Poppler::Document *pdfdoc )
{
	QLinkedList<ObjectRect*> kpdfLinks;
	foreach(const Poppler::Link *popplerLink, popplerLinks)
	{
		QRectF linkArea = popplerLink->linkArea();
		double nl = linkArea.left() / (double)width,
		       nt = linkArea.top() / (double)height,
		       nr = linkArea.right() / (double)width,
		       nb = linkArea.bottom() / (double)height;
		// create the rect using normalized coords and attach the KPDFLink to it
		ObjectRect * rect = new ObjectRect( nl, nt, nr, nb, ObjectRect::Link, createKPDFLinkFromPopplerLink(popplerLink, pdfdoc) );
		// add the ObjectRect to the container
		kpdfLinks.push_front( rect );
	}
	qDeleteAll(popplerLinks);
	return kpdfLinks;
}

/** NOTES on threading:
 * internal: thread race prevention is done via the 'docLock' mutex. the
 *           mutex is needed only because we have the asynchronous thread; else
 *           the operations are all within the 'gui' thread, scheduled by the
 *           Qt scheduler and no mutex is needed.
 * external: dangerous operations are all locked via mutex internally, and the
 *           only needed external thing is the 'canGeneratePixmap' method
 *           that tells if the generator is free (since we don't want an
 *           internal queue to store PixmapRequests). A generatedPixmap call
 *           without the 'ready' flag set, results in undefined behavior.
 * So, as example, printing while generating a pixmap asynchronously is safe,
 * it might only block the gui thread by 1) waiting for the mutex to unlock
 * in async thread and 2) doing the 'heavy' print operation.
 */

KPDF_EXPORT_PLUGIN(PDFGenerator)

PDFGenerator::PDFGenerator( KPDFDocument * doc )
    : Generator( doc ), pdfdoc( 0 ), ready( true ),
    pixmapRequest( 0 ), docInfoDirty( true ), docSynopsisDirty( true ),
    docFontsDirty( true )
{
    // generate kpdfOutputDev and cache page color
    reparseConfig();
    // generate the pixmapGeneratorThread
    generatorThread = new PDFPixmapGeneratorThread( this );
    connect(generatorThread, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
}

PDFGenerator::~PDFGenerator()
{
    // first stop and delete the generator thread
    if ( generatorThread )
    {
        generatorThread->wait();
        delete generatorThread;
    }
    // remove other internal objects
    docLock.lock();
    delete pdfdoc;
    docLock.unlock();
}

void PDFGenerator::setOrientation(QVector<KPDFPage*> & pagesVector, int orientation)
{
    loadPages(pagesVector,orientation,true);
}

//BEGIN Generator inherited functions
bool PDFGenerator::loadDocument( const QString & filePath, QVector<KPDFPage*> & pagesVector )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kDebug() << "PDFGenerator: multiple calls to loadDocument. Check it." << endl;
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::load( filePath, 0, 0 );

    // if the file didn't open correctly it might be encrypted, so ask for a pass
    bool firstInput = true;
    bool triedWallet = false;
    KWallet::Wallet * wallet = 0;
    while ( !pdfdoc && pdfdoc->isLocked() )
    {
        QByteArray password;

        // 1.A. try to retrieve the first password from the kde wallet system
        if ( !triedWallet )
        {
            QString walletName = KWallet::Wallet::NetworkWallet();
            wallet = KWallet::Wallet::openWallet( walletName );
            if ( wallet )
            {
                // use the KPdf folder (and create if missing)
                if ( !wallet->hasFolder( "KPdf" ) )
                    wallet->createFolder( "KPdf" );
                wallet->setFolder( "KPdf" );

                // look for the pass in that folder
                QString retrievedPass;
                if ( !wallet->readPassword( filePath.section('/', -1, -1), retrievedPass ) )
                    password = retrievedPass.toLocal8Bit();
            }
            triedWallet = true;
        }

        // 1.B. if not retrieved, ask the password using the kde password dialog
        if ( password.isNull() )
        {
            QString prompt;
            if ( firstInput )
                prompt = i18n( "Please insert the password to read the document:" );
            else
                prompt = i18n( "Incorrect password. Try again:" );
            firstInput = false;

            // if the user presses cancel, abort opening
            if ( KPasswordDialog::getPassword( 0, password, prompt ) != KPasswordDialog::Accepted )
                break;
        }

        // 2. reopen the document using the password
        pdfdoc->unlock( password, password );

        // 3. if the password is correct, store it to the wallet
        if ( !pdfdoc->isLocked() && wallet && /*safety check*/ wallet->isOpen() )
        {
            QString goodPass = QString::fromLocal8Bit( password.data() );
            wallet->writePassword( filePath.section('/', -1, -1), goodPass );
        }
    }
    if ( !pdfdoc )
    {
        pdfdoc = 0;
        return false;
    }

    // build Pages (currentPage was set -1 by deletePages)
    uint pageCount = pdfdoc->numPages();
    pagesVector.resize(pageCount);

    loadPages(pagesVector, -1, false);

    // the file has been loaded correctly
    return true;
}

void PDFGenerator::loadPages(QVector<KPDFPage*> &pagesVector, int rotation, bool clear)
{
    // TODO XPDF 3.01 check
    int count=pagesVector.count(),w=0,h=0;
    for ( int i = 0; i < count ; i++ )
    {
        // get xpdf page
        Poppler::Page * p = pdfdoc->page( i );
        QSize pSize = p->pageSize();
        w = pSize.width();
        h = pSize.height();
        if (rotation==-1)
        {
          switch (p->orientation())
          {
            case Poppler::Page::Landscape: rotation = 1; break;
            case Poppler::Page::UpsideDown: rotation = 2; break;
            case Poppler::Page::Seascape: rotation = 3; break;
            case Poppler::Page::Portrait: rotation = 0; break;
          }
        }
        if (rotation % 2 == 1)
          qSwap(w,h);
        // init a kpdfpage, add transition and annotation information
        KPDFPage * page = new KPDFPage( i, w, h, rotation );
        addTransition( p, page );
        if ( true ) //TODO real check
          addAnnotations( p, page );
// 	    kWarning() << page->width() << "x" << page->height() << endl;

// need a way to find efficient (maybe background textpage generation)
	docLock.lock();
	QList<Poppler::TextBox*> textList = p->textList();
	docLock.unlock();
	page->setSearchPage(abstractTextPage(textList, page->height(), page->width(), rotation));
	
	delete p;

        if (clear && pagesVector[i])
            delete pagesVector[i];
        // set the kpdfpage at the right position in document's pages vector
        pagesVector[i] = page;
// 	kWarning() << page->width() << "x" << page->height() << endl;
    }
}

QString * PDFGenerator::getText( const RegularAreaRect * area, KPDFPage * page  )
{
    QRect rect = area->first()->geometry((int)page->width(),(int)page->height());
    Poppler::Page *pp = pdfdoc->page( page->number() );
    QString text = pp->text(rect);
    delete pp;
    return new QString(text);
}

RegularAreaRect * PDFGenerator::findText (const QString & text, SearchDir dir, 
    const bool strictCase, const RegularAreaRect * sRect, KPDFPage * page )
{
    dir = sRect ? NextRes : FromTop;
    QRectF rect;

    if ( dir == NextRes )
    {
        // when using thein ternal search we only play with normrects
        rect.setLeft( sRect->first()->left * page->width() );
        rect.setTop( sRect->first()->top * page->height() );
        rect.setRight( sRect->first()->right * page->width() );
        rect.setBottom( sRect->first()->bottom * page->height() );
    }

    // this loop is only for 'bad case' Reses
    bool found = false;
    Poppler::Page *pp = pdfdoc->page( page->number() );
    docLock.lock();
    Poppler::Page::SearchMode sm;
    if (strictCase) sm = Poppler::Page::CaseSensitive;
    else sm = Poppler::Page::CaseInsensitive;
    while ( !found )
    {
        if ( dir == FromTop ) found = pp->search(text, rect, Poppler::Page::FromTop, sm);
        else if ( dir == NextRes ) found = pp->search(text, rect, Poppler::Page::NextResult, sm);
        else if ( dir == PrevRes ) found = pp->search(text, rect, Poppler::Page::PreviousResult, sm);

        // if not found (even in case unsensitive search), terminate
        if ( !found )
            break;
    }
    docLock.unlock();
    delete pp;

    // if the page was found, return a new normalizedRect
    if ( found )
    {
        RegularAreaRect *ret=new RegularAreaRect;
        ret->append (new NormalizedRect( rect.left() / page->width(), rect.top() / page->height(), 
            rect.right() / page->width(), rect.bottom() / page->height() ) );
        return ret;
    }
    return 0;
}

const DocumentInfo * PDFGenerator::generateDocumentInfo()
{
    if ( docInfoDirty )
    {
        docLock.lock();
        
        docInfo.set( "mimeType", "application/pdf" );
        
        if ( pdfdoc )
        {
            // compile internal structure reading properties from PDFDoc
            docInfo.set( "title", pdfdoc->info("Title"), i18n("Title") );
            docInfo.set( "subject", pdfdoc->info("Subject"), i18n("Subject") );
            docInfo.set( "author", pdfdoc->info("Author"), i18n("Author") );
            docInfo.set( "keywords", pdfdoc->info("Keywords"), i18n("Keywords") );
            docInfo.set( "creator", pdfdoc->info("Creator"), i18n("Creator") );
            docInfo.set( "producer", pdfdoc->info("Producer"), i18n("Producer") );
            docInfo.set( "creationDate", KGlobal::locale()->formatDateTime( pdfdoc->date("CreationDate"), false, true ), i18n("Created") );
            docInfo.set( "modificationDate", KGlobal::locale()->formatDateTime( pdfdoc->date("ModDate"), false, true ), i18n("Modified") );

            docInfo.set( "format", i18nc( "PDF v. <version>", "PDF v. %1",
                         QString::number( pdfdoc->pdfVersion() ) ), i18n( "Format" ) );
            docInfo.set( "encryption", pdfdoc->isEncrypted() ? i18n( "Encrypted" ) : i18n( "Unencrypted" ),
                         i18n("Security") );
            docInfo.set( "optimization", pdfdoc->isLinearized() ? i18n( "Yes" ) : i18n( "No" ),
                         i18n("Optimized") );

            docInfo.set( "pages", QString::number( pdfdoc->numPages() ), i18n("Pages") );
        }
        else
        {
            // TODO not sure one can reach here, check and if it is not possible, remove the code
            docInfo.set( "title", i18n("Unknown"), i18n("Title") );
            docInfo.set( "subject", i18n("Unknown"), i18n("Subject") );
            docInfo.set( "author", i18n("Unknown"), i18n("Author") );
            docInfo.set( "keywords", i18n("Unknown"), i18n("Keywords") );
            docInfo.set( "creator", i18n("Unknown"), i18n("Creator") );
            docInfo.set( "producer", i18n("Unknown"), i18n("Producer") );
            docInfo.set( "creationDate", i18n("Unknown Date"), i18n("Created") );
            docInfo.set( "modificationDate", i18n("Unknown Date"), i18n("Modified") );

            docInfo.set( "format", "PDF", i18n( "Format" ) );
            docInfo.set( "encryption", i18n( "Unknown Encryption" ), i18n( "Security" ) );
            docInfo.set( "optimization", i18n( "Unknown Optimization" ), i18n( "Optimized" ) );

            docInfo.set( "pages", i18n("Unknown"), i18n("Pages") );
        }
        docLock.unlock();

        // if pdfdoc is valid then we cached good info -> don't cache them again
        if ( pdfdoc )
            docInfoDirty = false;
    }
    return &docInfo;
}

const DocumentSynopsis * PDFGenerator::generateDocumentSynopsis()
{
    if ( !docSynopsisDirty )
        return &docSyn;

    if ( !pdfdoc )
        return NULL;

    docLock.lock();
    QDomDocument *toc = pdfdoc->toc();
    docLock.unlock();
    if ( !toc )
        return NULL;

    docSyn = DocumentSynopsis();
    addSynopsisChildren(toc, &docSyn);
    delete toc;

    docSynopsisDirty = false;
    return &docSyn;
}

const DocumentFonts * PDFGenerator::generateDocumentFonts()
{
    if ( !docFontsDirty )
        return &docFonts;

    // initialize fonts dom
    docFonts = DocumentFonts();
    docFonts.appendChild( docFonts.createElement( "Fonts" ) );

    docLock.lock();
    QList<Poppler::FontInfo> fonts = pdfdoc->fonts();
    docLock.unlock();

    const QString fontTypeNames[8] = {
        i18n("unknown"),
        i18n("Type 1"),
        i18n("Type 1C"),
        i18n("Type 3"),
        i18n("TrueType"),
        i18n("CID Type 0"),
        i18n("CID Type 0C"),
        i18n("CID TrueType")
    };
    
    foreach (const Poppler::FontInfo &font, fonts)
    {
        // 0. add font element
        QDomElement fontElem = docFonts.createElement( "font" );
        docFonts.firstChild().appendChild( fontElem );

        // 1. set Name
        const QString &name = font.name();
        fontElem.setAttribute( "Name", name.isNull() ? i18n("[none]") : name );

        // 2. set Type
        fontElem.setAttribute( "Type", fontTypeNames[ font.type() ] );

        // 3. set Embedded
        fontElem.setAttribute( "Embedded", font.isEmbedded() ? i18n("Yes") : i18n("No") );

        // 4. set Path
        fontElem.setAttribute( "File", font.file() );
    }

    return &docFonts;
}

bool PDFGenerator::isAllowed( int permissions )
{
#if !KPDF_FORCE_DRM
    if (KAuthorized::authorize("skip_drm") && !KpdfSettings::obeyDRM()) return true;
#endif

    bool b = true;
    if (permissions & KPDFDocument::AllowModify) b = b && pdfdoc->okToChange();
    if (permissions & KPDFDocument::AllowCopy) b = b && pdfdoc->okToCopy();
    if (permissions & KPDFDocument::AllowPrint) b = b && pdfdoc->okToPrint();
    if (permissions & KPDFDocument::AllowNotes) b = b && pdfdoc->okToAddNotes();
    return b;
}

bool PDFGenerator::canGeneratePixmap( bool /* async */)
{
    return ready;
}

void PDFGenerator::generatePixmap( PixmapRequest * request )
{
#ifndef NDEBUG
    if ( !ready )
        kDebug() << "calling generatePixmap() when not in READY state!" << endl;
#endif
    // update busy state (not really needed here, because the flag needs to
    // be set only to prevent asking a pixmap while the thread is running)
    ready = false;

    // debug requests to this (xpdf) generator
    //kDebug() << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "]." << endl;

    /** asynchronous requests (generation in PDFPixmapGeneratorThread::run() **/
    if ( request->async )
    {
        // start the generation into the thread
        generatorThread->startGeneration( request );
        return;
    }

    /** synchronous request: in-place generation **/
    // compute dpi used to get an image with desired width and height
    KPDFPage * page = request->page;
    double fakeDpiX = request->width * 72.0 / page->width(),
           fakeDpiY = request->height * 72.0 / page->height();

    // setup kpdf output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasSearchPage() && (request->width == page->width()) &&
                       (request->height == page->height());
    // generate links and image rects if rendering pages on pageview
    bool genObjectRects = request->id & (PAGEVIEW_ID | PRESENTATION_ID);

    // 0. LOCK [waits for the thread end]
    docLock.lock();

    // 1. Set OutputDev parameters and Generate contents
    // note: thread safety is set on 'false' for the GUI (this) thread
    Poppler::Page *p = pdfdoc->page(page->number());

    // 2. Take data from outputdev and attach it to the Page
    page->setPixmap( request->id, p->splashRenderToPixmap(fakeDpiX, fakeDpiY, -1, -1, -1, -1, genObjectRects) );
    
    if ( genObjectRects )
    {
    	// TODO previously we extracted Image type rects too, but that needed porting to poppler
        // and as we are not doing anything with Image type rects i did not port it, have a look at
        // dead gp_outputdev.cpp on image extraction
        page->setObjectRects( generateKPDFLinks(p->links(), request->width, request->height, pdfdoc) );
    }

    // 3. UNLOCK [re-enables shared access]
    docLock.unlock();
    if ( genTextPage )
    {
        page->setSearchPage( abstractTextPage(p->textList(), page->height(), page->width(),page->rotation()) );
    }
    delete p;
    
    // update ready state
    ready = true;

    // notify the new generation
    signalRequestDone( request );
}

bool PDFGenerator::canGenerateTextPage()
{
    return true;
}

void PDFGenerator::generateSyncTextPage( KPDFPage * page )
{
    // build a TextList...
    Poppler::Page *pp = pdfdoc->page( page->number() );
    docLock.lock();
    QList<Poppler::TextBox*> textList = pp->textList();
    docLock.unlock();
    delete pp;
    // ..and attach it to the page
    page->setSearchPage( abstractTextPage(textList, page->height(), page->width(),page->rotation()) );
}

bool PDFGenerator::print( KPrinter& printer )
{
    int width, height;
    QString ps = printer.option("PageSize");
    if (ps.indexOf(QRegExp("w\\d+h\\d+")) == 0)
    {
        // size not supported by Qt, KPrinter gives us the size as wWIDTHhHEIGHT
        // remove the w
        ps = ps.mid(1);
        int hPos = ps.indexOf("h");
        width = ps.left(hPos).toInt();
        height = ps.mid(hPos+1).toInt();
    }
    else
    {
        // size is supported by Qt, we get either the pageSize name or nothing because the default pageSize is used
        QPrinter dummy(QPrinter::PrinterResolution);
        dummy.setFullPage(true);
        dummy.setPageSize((QPrinter::PageSize)(ps.isEmpty() ? KGlobal::locale()->pageSize() : pageNameToPageSize(ps)));

        width = dummy.width();
        height = dummy.height();
    }

    KTempFile tf( QString::null, ".ps" );

    QList<int> pageList;
    if (!printer.previewOnly()) pageList = printer.pageList();
    else for(int i = 1; i <= pdfdoc->numPages(); i++) pageList.push_back(i);
    
    docLock.lock();
    // TODO rotation
    if (pdfdoc->print(tf.name(), pageList, 72, 72, 0))
    {
        docLock.unlock();
        printer.printFiles(QStringList(tf.name()), true);
        return true;
    }
    else
    {
        docLock.unlock();
        return false;
    }
	return false;
}

QString PDFGenerator::getMetaData( const QString & key, const QString & option )
{
    if ( key == "StartFullScreen" )
    {
        // asking for the 'start in fullscreen mode' (pdf property)
        if ( pdfdoc->pageMode() == Poppler::Document::FullScreen )
            return "yes";
    }
    else if ( key == "NamedViewport" && !option.isEmpty() )
    {
        // asking for the page related to a 'named link destination'. the
        // option is the link name. @see addSynopsisChildren.
        DocumentViewport viewport;
        docLock.lock();
        Poppler::LinkDestination *ld = pdfdoc->linkDestination( option );
        docLock.unlock();
        if ( ld )
        {
            fillViewportFromLinkDestination( viewport, *ld, pdfdoc );
        }
        delete ld;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    return QString();
}

bool PDFGenerator::reparseConfig()
{
    // load paper color from Settings or use the white default color
    QColor color = ( (KpdfSettings::renderMode() == KpdfSettings::EnumRenderMode::Paper ) &&
                     KpdfSettings::changeColors() ) ? KpdfSettings::paperColor() : Qt::white;
    // if paper color is changed we have to rebuild every visible pixmap in addition
    // to the outputDevice. it's the 'heaviest' case, other effect are just recoloring
    // over the page rendered on 'standard' white background.
    if ( pdfdoc && color != pdfdoc->paperColor() )
    {
        docLock.lock();
        pdfdoc->setPaperColor(color);
        docLock.unlock();
        return true;
    }
    return false;
}
//END Generator inherited functions

inline void append (KPDFTextPage* ktp,
    QString s, double l, double b, double r, double t)
{
//       kWarning() << "text: " << s << " at (" << l << "," << t << ")x(" << r <<","<<b<<")" << endl;
                ktp->append( s ,
                    new NormalizedRect(
                    l,
                    b,
                    r,
                    t
                    ));
}

// TODO have a look at who should delete what's inside text
KPDFTextPage * PDFGenerator::abstractTextPage(const QList<Poppler::TextBox*> &text, double height, double width,int rot)
{    
    KPDFTextPage* ktp=new KPDFTextPage;
    Poppler::TextBox *next; 
    kWarning() << "getting text page in generator pdf - rotation: " << rot << endl;
    int charCount=0;
    int j;
    QString s;
    NormalizedRect * wordRect = new NormalizedRect;
    foreach (Poppler::TextBox *word, text)
    {
	wordRect->left = word->boundingBox().left();
	wordRect->bottom = word->boundingBox().bottom();
	wordRect->right = word->boundingBox().right();
	wordRect->top = word->boundingBox().top();
        charCount=word->text().length();
        next=word->nextWord();
        switch (rot)
        {
            case 0:
            // 0 degrees, normal word boundaries are top and bottom
            // only x boundaries change the order of letters is normal not reversed
            for (j = 0; j < charCount; j++)
            {
                s = word->text().at(j);
                append(ktp, (j==charCount-1 && !next ) ? (s + "\n") : s,
                    // this letters boundary
                    word->edge(j)/width,
                    wordRect->bottom/height,
                    // next letters boundary
                    word->edge(j+1)/width,
                    wordRect->top/height);
            }
            
            if ( word->hasSpaceAfter() && next )
              append(ktp, " ",
                    // this letters boundary
                     word->edge(charCount)/width,
                     wordRect->bottom/height,
                    // next letters boundary
                     next->edge(0)/width,
                     wordRect->top/height);
            break;

            case 1:
            // 90 degrees, x boundaries not changed
            // y ones change, the order of letters is normal not reversed
            for (j=0;j<charCount;j++)
            {
                s=word->text().at(j);
                append(ktp, (j==charCount-1 && !next ) ? (s + "\n") : s,
                    wordRect->left/width,
                    word->edge(j)/height,
                    wordRect->right/width,
                    word->edge(j+1)/height);
            }
            
            if ( word->hasSpaceAfter() && next )
              append(ktp, " ",
                    // this letters boundary
                     wordRect->left/width,
                     word->edge(charCount)/height,
                    // next letters boundary
                     wordRect->right/width,
                     next->edge(0)/height);
            break;

            case 2:
            // same as case 0 but reversed order of letters
            for (j=0;j<charCount;j++)
            {
                s=word->text().at(j);
                append(ktp, (j==charCount-1 && !next ) ? (s + "\n") : s,
                    word->edge(j+1)/width,
                    wordRect->bottom/height,
                    word->edge(j)/width,
                    wordRect->top/height);

            }
            
            if ( word->hasSpaceAfter() && next )
              append(ktp, " ",
                    // this letters boundary
                     next->edge(0)/width,
                     wordRect->bottom/height,
                    // next letters boundary
                     word->edge(charCount)/width,
                     wordRect->top/height);
           
            break;

            case 3:
            for (j=0;j<charCount;j++)
            {
                s=word->text().at(j);
                append(ktp, (j==charCount-1 && !next ) ? (s + "\n") : s,
                    wordRect->left/width,
                    word->edge(j+1)/height,
                    wordRect->right/width,
                    word->edge(j)/height);
            }
            
            if ( word->hasSpaceAfter() && next )
              append(ktp, " ",
                    // this letters boundary
                     wordRect->left/width,
                     next->edge(0)/height,
                    // next letters boundary
                     wordRect->right/width,
                     word->edge(charCount)/height);
            break;
        }
    }
    delete wordRect;
    return ktp;
}

void PDFGenerator::addSynopsisChildren( QDomNode * parent, QDomNode * parentDestination )
{
    // keep track of the current listViewItem
    QDomNode n = parent->firstChild();
    while( !n.isNull() )
    {
        // convert the node to an element (sure it is)
        QDomElement e = n.toElement();

        // The name is the same
        QDomElement item = docSyn.createElement( e.tagName() );
        parentDestination->appendChild(item);

        if (!e.attribute("ExternalFileName").isNull()) item.setAttribute("ExternalFileName", e.attribute("ExternalFileName"));
        if (!e.attribute("DestinationName").isNull()) item.setAttribute("ViewportName", e.attribute("DestinationName"));
        if (!e.attribute("Destination").isNull())
        {
            DocumentViewport vp;
            fillViewportFromLinkDestination( vp, Poppler::LinkDestination(e.attribute("Destination")), pdfdoc );
            item.setAttribute( "Viewport", vp.toString() );
        }

        // descend recursively and advance to the next node
        if ( e.hasChildNodes() ) addSynopsisChildren( &n, &	item );
        n = n.nextSibling();
    }
}

void PDFGenerator::addAnnotations( Poppler::Page * popplerPage, KPDFPage * page )
{
    QList<Poppler::Annotation*> popplerAnnotations = popplerPage->annotations();
    foreach(Poppler::Annotation *a, popplerAnnotations)
    {
        a->window.width = (int)(page->width() * a->window.width);
        a->window.height = (int)(page->height() * a->window.height);
        //a->window.width = a->window.width < 200 ? 200 : a->window.width;
        // a->window.height = a->window.height < 120 ? 120 : a->window.height;
        // resize annotation's geometry to an icon
        // TODO oKular geom.right = geom.left + 22.0 / page->width();
        // TODO oKular geom.bottom = geom.top + 22.0 / page->height();
        
        // this is uber ugly but i don't know a better way to do it without introducing a poppler::annotation dependency on core
        QDomDocument doc;
        QDomElement root = doc.createElement("root");
        doc.appendChild(root);
        a->store( root, doc );
        page->addAnnotation( new Annotation( doc ) );
    }
    qDeleteAll(popplerAnnotations);
}

void PDFGenerator::addTransition( Poppler::Page * pdfPage, KPDFPage * page )
// called on opening when MUTEX is not used
{
    Poppler::PageTransition *pdfTransition = pdfPage->transition();
    if ( !pdfTransition || pdfTransition->type() == Poppler::PageTransition::Replace )
        return;

    KPDFPageTransition *transition = new KPDFPageTransition();
    switch ( pdfTransition->type() ) {
        case Poppler::PageTransition::Replace:
            // won't get here, added to avoid warning
            break;
        case Poppler::PageTransition::Split:
            transition->setType( KPDFPageTransition::Split );
            break;
        case Poppler::PageTransition::Blinds:
            transition->setType( KPDFPageTransition::Blinds );
            break;
        case Poppler::PageTransition::Box:
            transition->setType( KPDFPageTransition::Box );
            break;
        case Poppler::PageTransition::Wipe:
            transition->setType( KPDFPageTransition::Wipe );
            break;
        case Poppler::PageTransition::Dissolve:
            transition->setType( KPDFPageTransition::Dissolve );
            break;
        case Poppler::PageTransition::Glitter:
            transition->setType( KPDFPageTransition::Glitter );
            break;
        case Poppler::PageTransition::Fly:
            transition->setType( KPDFPageTransition::Fly );
            break;
        case Poppler::PageTransition::Push:
            transition->setType( KPDFPageTransition::Push );
            break;
        case Poppler::PageTransition::Cover:
            transition->setType( KPDFPageTransition::Cover );
            break;
        case Poppler::PageTransition::Uncover:
            transition->setType( KPDFPageTransition::Uncover );
            break;
        case Poppler::PageTransition::Fade:
            transition->setType( KPDFPageTransition::Fade );
            break;
    }

    transition->setDuration( pdfTransition->duration() );

    switch ( pdfTransition->alignment() ) {
        case Poppler::PageTransition::Horizontal:
            transition->setAlignment( KPDFPageTransition::Horizontal );
            break;
        case Poppler::PageTransition::Vertical:
            transition->setAlignment( KPDFPageTransition::Vertical );
            break;
    }

    switch ( pdfTransition->direction() ) {
        case Poppler::PageTransition::Inward:
            transition->setDirection( KPDFPageTransition::Inward );
            break;
        case Poppler::PageTransition::Outward:
            transition->setDirection( KPDFPageTransition::Outward );
            break;
    }

    transition->setAngle( pdfTransition->angle() );
    transition->setScale( pdfTransition->scale() );
    transition->setIsRectangular( pdfTransition->isRectangular() );

    page->setTransition( transition );
}



void PDFGenerator::threadFinished()
{
#if 0
    // check if thread is running (has to be stopped now)
    if ( generatorThread->running() )
    {
        // if so, wait for effective thread termination
        if ( !generatorThread->wait( 9999 /*10s timeout*/ ) )
        {
            kWarning() << "PDFGenerator: thread sent 'data available' "
                        << "signal but had problems ending." << endl;
            return;
        }
}
#endif

    // 1. the mutex must be unlocked now
    bool isLocked = true;
    if (docLock.tryLock()) {
        docLock.unlock();
        isLocked = false;
    }
    if ( isLocked )
    {
        kWarning() << "PDFGenerator: 'data available' but mutex still "
                    << "held. Recovering." << endl;
        // synchronize GUI thread (must not happen)
        docLock.lock();
        docLock.unlock();
    }

    // 2. put thread's generated data into the KPDFPage
    PixmapRequest * request = generatorThread->request();
    QImage * outImage = generatorThread->takeImage();
    QList<Poppler::TextBox*> outText = generatorThread->takeText();
    QLinkedList< ObjectRect * > outRects = generatorThread->takeObjectRects();

    request->page->setPixmap( request->id, new QPixmap( *outImage ) );
    delete outImage;
    if ( !outText.isEmpty() )
        request->page->setSearchPage( abstractTextPage( outText , 
            request->page->height(), request->page->width(),request->page->rotation()));
    bool genObjectRects = request->id & (PAGEVIEW_ID | PRESENTATION_ID);
    if (genObjectRects) request->page->setObjectRects( outRects );

    // 3. tell generator that data has been taken
    generatorThread->endGeneration();

    // update ready state
    ready = true;
    // notify the new generation
    signalRequestDone( request );
}



/** The  PDF Pixmap Generator Thread  **/

struct PPGThreadPrivate
{
    // reference to main objects
    PDFGenerator * generator;
    PixmapRequest * currentRequest;

    // internal temp stored items. don't delete this.
    QImage * m_image;
    QList<Poppler::TextBox*> m_textList;
    QLinkedList< ObjectRect * > m_rects;
    bool m_rectsTaken;
};

PDFPixmapGeneratorThread::PDFPixmapGeneratorThread( PDFGenerator * gen )
    : QThread(), d( new PPGThreadPrivate() )
{
    d->generator = gen;
    d->currentRequest = 0;
    d->m_image = 0;
    d->m_rectsTaken = true;
}

PDFPixmapGeneratorThread::~PDFPixmapGeneratorThread()
{
    // delete internal objects if the class is deleted before the gui thread
    // takes the data
    delete d->m_image;
    qDeleteAll(d->m_textList);
    if ( !d->m_rectsTaken && d->m_rects.count() )
    {
        qDeleteAll(d->m_rects);
    }
    delete d->currentRequest;
    // delete internal storage structure
    delete d;
}

void PDFPixmapGeneratorThread::startGeneration( PixmapRequest * request )
{
#ifndef NDEBUG
    // check if a generation is already running
    if ( d->currentRequest )
    {
        kDebug() << "PDFPixmapGeneratorThread: requesting a pixmap "
                  << "when another is being generated." << endl;
        delete request;
        return;
    }

    // check if the mutex is already held
    bool isLocked = true;
    if (d->generator->docLock.tryLock()) {
        d->generator->docLock.unlock();
        isLocked = false;
    }
    if ( isLocked )
    {
        kDebug() << "PDFPixmapGeneratorThread: requesting a pixmap "
                  << "with the mutex already held." << endl;
        delete request;
        return;
    }
#endif
    // set generation parameters and run thread
    d->currentRequest = request;
    start( QThread::InheritPriority );
}

void PDFPixmapGeneratorThread::endGeneration()
{
#ifndef NDEBUG
    // check if a generation is already running
    if ( !d->currentRequest )
    {
        kDebug() << "PDFPixmapGeneratorThread: 'end generation' called "
                  << "but generation was not started." << endl;
        return;
    }
#endif
    // reset internal members preparing for a new generation
    d->currentRequest = 0;
}

PixmapRequest *PDFPixmapGeneratorThread::request() const
{
    return d->currentRequest;
}

QImage * PDFPixmapGeneratorThread::takeImage() const
{
    QImage * img = d->m_image;
    d->m_image = 0;
    return img;
}

QList<Poppler::TextBox*> PDFPixmapGeneratorThread::takeText()
{
    QList<Poppler::TextBox*> tl = d->m_textList;
    d->m_textList.clear();
    return tl;
}

QLinkedList< ObjectRect * > PDFPixmapGeneratorThread::takeObjectRects() const
{
    d->m_rectsTaken = true;
    return d->m_rects;
}

void PDFPixmapGeneratorThread::run()
// perform contents generation, when the MUTEX is already LOCKED
// @see PDFGenerator::generatePixmap( .. ) (and be aware to sync the code)
{
    // compute dpi used to get an image with desired width and height
    KPDFPage * page = d->currentRequest->page;
    int width = d->currentRequest->width,
        height = d->currentRequest->height;
    double fakeDpiX = width * 72.0 / page->width(),
           fakeDpiY = height * 72.0 / page->height();

    // setup kpdf output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasSearchPage() &&
                       ( width == page->width() ) &&
                       ( height == page->height() );

    // generate links and image rects if rendering pages on pageview
    bool genObjectRects = d->currentRequest->id & (PAGEVIEW_ID | PRESENTATION_ID);

    // 0. LOCK s[tart locking XPDF thread unsafe classes]
    d->generator->docLock.lock();

    // 1. set OutputDev parameters and Generate contents
    Poppler::Page *pp = d->generator->pdfdoc->page( page->number() );
    
    // 2. grab data from the OutputDev and store it locally (note takeIMAGE)
#ifndef NDEBUG
    if ( d->m_image )
        kDebug() << "PDFPixmapGeneratorThread: previous image not taken" << endl;
    if ( !d->m_textList.isEmpty() )
        kDebug() << "PDFPixmapGeneratorThread: previous text not taken" << endl;
#endif
    d->m_image = new QImage( pp->splashRenderToImage( fakeDpiX, fakeDpiY, -1, -1, -1, -1, genObjectRects ) );
    
    if ( genObjectRects )
    {
    	d->m_rects = generateKPDFLinks(pp->links(), width, height, d->generator->pdfdoc);
    }
    else 
    d->m_rectsTaken = false;

    if ( genTextPage )
    {
        d->m_textList = pp->textList();
    }
    delete pp;
    
    // 3. [UNLOCK] mutex
    d->generator->docLock.unlock();

    // by ending the thread notifies the GUI thread that data is pending and can be read
}
#include "generator_pdf.moc"

