/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qfile.h>
#include <qevent.h>
#include <qimage.h>
#include <qapplication.h>
#include <qpaintdevicemetrics.h>
#include <qregexp.h>
#include <kapplication.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kwallet.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kdebug.h>

// xpdf includes
#include "xpdf/PSOutputDev.h"
#include "xpdf/TextOutputDev.h"
#include "xpdf/Link.h"
#include "xpdf/ErrorCodes.h"
#include "xpdf/UnicodeMap.h"
#include "xpdf/Outline.h"
#include "xpdf/GfxState.h"
#include "xpdf/Annot.h" // for retrieving fonts only
#include "xpdf/GlobalParams.h"
#include "goo/GList.h"

// local includes
#include "generator_pdf.h"
#include "gp_outputdev.h"
#include "core/observer.h" //for PAGEVIEW_ID
#include "core/page.h"
#include "core/annotations.h"
#include "core/pagetransition.h"
#include "conf/settings.h"

#include <config.h>

// id for DATA_READY PDFPixmapGeneratorThread Event
#define TGE_DATAREADY_ID 6969

/** NOTES on threading:
 * internal: thread race prevention is done via the 'docLock' mutex. the
 *           mutex is needed only because we have the asyncronous thread; else
 *           the operations are all within the 'gui' thread, scheduled by the
 *           Qt scheduler and no mutex is needed.
 * external: dangerous operations are all locked via mutex internally, and the
 *           only needed external thing is the 'canGeneratePixmap' method
 *           that tells if the generator is free (since we don't want an
 *           internal queue to store PixmapRequests). A generatedPixmap call
 *           without the 'ready' flag set, results in undefined behavior.
 * So, as example, printing while generating a pixmap asyncronously is safe,
 * it might only block the gui thread by 1) waiting for the mutex to unlock
 * in async thread and 2) doing the 'heavy' print operation.
 */

unsigned int PDFGenerator::m_count = 0;

KPDF_EXPORT_PLUGIN(PDFGenerator)

PDFGenerator::PDFGenerator( KPDFDocument * doc )
    : Generator( doc ), pdfdoc( 0 ), kpdfOutputDev( 0 ), ready( true ),
    pixmapRequest( 0 ), docInfoDirty( true ), docSynopsisDirty( true ),
    docFontsDirty( true )
{
    // xpdf 'extern' global class (m_count is a static instance counter)
    //if ( m_count ) TODO check if we need to insert these lines..
    //	delete globalParams;
    globalParams = new GlobalParams("");
    globalParams->setupBaseFonts(NULL);
    m_count++;
    
    // generate kpdfOutputDev and cache page color
    reparseConfig();
    // generate the pixmapGeneratorThread
    generatorThread = new PDFPixmapGeneratorThread( this );
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
    delete kpdfOutputDev;
    delete pdfdoc;
    docLock.unlock();
    
    if ( --m_count == 0 )
        delete globalParams;
}

void PDFGenerator::setOrientation(QValueVector<KPDFPage*> & pagesVector, int orientation)
{
    loadPages(pagesVector,orientation,true);
}

//BEGIN Generator inherited functions
bool PDFGenerator::loadDocument( const QString & filePath, QValueVector<KPDFPage*> & pagesVector )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kdDebug() << "PDFGenerator: multiple calls to loadDocument. Check it." << endl;
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = new PDFDoc( new GString( QFile::encodeName( filePath ) ), 0, 0 );

    // if the file didn't open correctly it might be encrypted, so ask for a pass
    bool firstInput = true;
    bool triedWallet = false;
    KWallet::Wallet * wallet = 0;
    while ( !pdfdoc->isOk() && pdfdoc->getErrorCode() == errEncrypted )
    {
        QCString password;

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
                    password = retrievedPass.local8Bit();
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
            if ( KPasswordDialog::getPassword( password, prompt ) != KPasswordDialog::Accepted )
                break;
        }

        // 2. reopen the document using the password
        GString * pwd2 = new GString( password.data() );
        delete pdfdoc;
        pdfdoc = new PDFDoc( new GString( QFile::encodeName( filePath ) ), pwd2, pwd2 );
        delete pwd2;

        // 3. if the password is correct, store it to the wallet
        if ( pdfdoc->isOk() && wallet && /*safety check*/ wallet->isOpen() )
        {
            QString goodPass = QString::fromLocal8Bit( password.data() );
            wallet->writePassword( filePath.section('/', -1, -1), goodPass );
        }
    }
    if ( !pdfdoc->isOk() )
    {
        delete pdfdoc;
        pdfdoc = 0;
        return false;
    }

    // initialize output device for rendering current pdf
    kpdfOutputDev->initDevice( pdfdoc );

    // build Pages (currentPage was set -1 by deletePages)
    uint pageCount = pdfdoc->getNumPages();
    pagesVector.resize( pageCount );

    loadPages(pagesVector,0);

    // the file has been loaded correctly
    return true;
}

void PDFGenerator::loadPages(QValueVector<KPDFPage*> & pagesVector, int rotation, bool clear)
{
    KPDFTextDev td;
    int count=pagesVector.count();
    for ( uint i = 0; i < count ; i++ )
    {
        // get xpdf page
        Page * p = pdfdoc->getCatalog()->getPage( i + 1 );

        // init a kpdfpage, add transition and annotations informations
        KPDFPage * page = new KPDFPage( i, p->getWidth(), p->getHeight(), rotation );
        addTransition( p, page );
        if ( true ) //TODO real check
            addAnnotations( p, page );

	docLock.lock();
	pdfdoc->displayPage( &td, page->number()+1, 72, 72, rotation, true, false );
	TextPage * textPage = td.takeTextPage();
	docLock.unlock();

	page->setSearchPage(abstractTextPage(textPage,page->height(),page->width()));

        if (clear && pagesVector[i])
            delete pagesVector[i];
        // set the kpdfpage at the right position in document's pages vector
        pagesVector[i] = page;
    }
}

TextPage * PDFGenerator::fastTextPage (KPDFPage * page)
{
    // fetch ourselves a textpage
    KPDFTextDev td;
    docLock.lock();
    pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, true, false );
    TextPage * textPage = td.takeTextPage();
    docLock.unlock();
    return textPage;
}

QString * PDFGenerator::getText( const RegularAreaRect * area, KPDFPage * page  )
{
    TextPage* textPage=fastTextPage (page);

    int left,right,bottom,top;
    QString * text = new QString;
    // when using internal stuff we just have 
    NormalizedRect * rect = area->first();
    left = (int)( rect->left * page->width() );
    top = (int)( rect->top * page->height() );
    right = (int)( rect->right * page->width() );
    bottom = (int)( rect->bottom * page->height() );
    *text= textPage->getText( left, top, right, bottom )->getCString();

    return text;
}

RegularAreaRect * PDFGenerator::findText (const QString & text, SearchDir dir, 
    const bool strictCase, const RegularAreaRect * sRect, KPDFPage * page )
{
// create a xpf's Unicode (unsigned int) array for the given text
    const QChar * str = text.unicode();
    int len = text.length();
    QMemArray<Unicode> u(len);
    for (int i = 0; i < len; ++i)
        u[i] = str[i].unicode();

    // find out the direction of search
    dir = sRect ? NextRes : FromTop;
    TextPage * textPage=fastTextPage (page);
    double sLeft, sTop, sRight, sBottom;

    if ( dir == NextRes )
    {
        // when using thein ternal search we only play with normrects
        sLeft = sRect->first()->left * page->width();
        sTop = sRect->first()->top * page->height();
        sRight = sRect->first()->right * page->width();
        sBottom = sRect->first()->bottom * page->height();
    }
    
    // this loop is only for 'bad case' Reses
    bool found = false;
    while ( !found )
    {
        if ( dir == FromTop )
            found = textPage->findText( const_cast<Unicode*>(static_cast<const Unicode*>(u)), len, gTrue, gTrue, gFalse, gFalse, &sLeft, &sTop, &sRight, &sBottom );
        else if ( dir == NextRes )
            found = textPage->findText( const_cast<Unicode*>(static_cast<const Unicode*>(u)), len, gFalse, gTrue, gTrue, gFalse, &sLeft, &sTop, &sRight, &sBottom );
        else if ( dir == PrevRes )
            // FIXME: this doesn't work as expected (luckily backward search isn't yet used)
            found = textPage->findText( const_cast<Unicode*>(static_cast<const Unicode*>(u)), len, gTrue, gFalse, gFalse, gTrue, &sLeft, &sTop, &sRight, &sBottom );

        // if not found (even in case unsensitive search), terminate
        if ( !found )
            break;

        // check for case sensitivity
        if ( strictCase )
        {
            found = QString::fromUtf8( (textPage->getText( sLeft, sTop, sRight, sBottom ))->getCString() ) == text;
            if ( !found && dir == FromTop )
                dir = NextRes;
        }
    }

    // if the page was found, return a new normalizedRect
    if ( found )
    {
        RegularAreaRect *ret=new RegularAreaRect;
        ret->append (new NormalizedRect( sLeft / page->width(), sTop / page->height(), 
            sRight / page->width(), sBottom / page->height() ) );
        return ret;
    }
    return 0;
}

const DocumentInfo * PDFGenerator::generateDocumentInfo()
{
    if ( docInfoDirty )
    {
        docLock.lock();
        // compile internal structure reading properties from PDFDoc
        docInfo.set( "title", getDocumentInfo("Title"), i18n("Title") );
        docInfo.set( "subject", getDocumentInfo("Subject"), i18n("Subject") );
        docInfo.set( "author", getDocumentInfo("Author"), i18n("Author") );
        docInfo.set( "keywords", getDocumentInfo("Keywords"), i18n("Keywords") );
        docInfo.set( "creator", getDocumentInfo("Creator"), i18n("Creator") );
        docInfo.set( "producer", getDocumentInfo("Producer"), i18n("Producer") );
        docInfo.set( "creationDate", getDocumentDate("CreationDate"), i18n("Created") );
        docInfo.set( "modificationDate", getDocumentDate("ModDate"), i18n("Modified") );
        docInfo.set( "mimeType", "application/pdf" );
        if ( pdfdoc )
        {
            docInfo.set( "format", i18n( "PDF v. <version>", "PDF v. %1" )
                         .arg( QString::number( pdfdoc->getPDFVersion() ) ), i18n( "Format" ) );
            docInfo.set( "encryption", pdfdoc->isEncrypted() ? i18n( "Encrypted" ) : i18n( "Unencrypted" ),
                         i18n("Security") );
            docInfo.set( "optimization", pdfdoc->isLinearized() ? i18n( "Yes" ) : i18n( "No" ),
                         i18n("Optimized") );
        }
        else
        {
            docInfo.set( "format", "PDF", i18n( "Format" ) );
            docInfo.set( "encryption", i18n( "Unknown Encryption" ), i18n( "Security" ) );
            docInfo.set( "optimization", i18n( "Unknown Optimization" ), i18n( "Optimized" ) );
        }
        docInfo.set( "pages", QString::number( pdfdoc->getCatalog()->getNumPages() ), i18n("Pages") );
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

    Outline * outline = pdfdoc->getOutline();
    if ( !outline )
        return NULL;

    GList * items = outline->getItems();
    if ( !items || items->getLength() < 1 )
        return NULL;

    docLock.lock();
    docSyn = DocumentSynopsis();
    if ( items->getLength() > 0 )
        addSynopsisChildren( &docSyn, items );
    docLock.unlock();

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

    // fonts repository to check for duplicates
    Ref * fonts = NULL;
    int fontsLen = 0;
    int fontsSize = 0;

    // temporary inner-loop xpdf objects
    Object objAnn, objAnnStr, objAnnRes;

    // loop on all pages
    docLock.lock();
    int pagesNumber = pdfdoc->getNumPages();
    for ( int pg = 0; pg < pagesNumber; pg++ )
    {
        // look for fonts in page's resources
        Page * page = pdfdoc->getCatalog()->getPage( pg + 1 );
        Dict * resDict = page->getResourceDict();
        if ( resDict )
            addFonts( resDict, &fonts, fontsLen, fontsSize );

        // look for fonts in annotation's resources
        Annots * annots = new Annots( pdfdoc->getXRef(), page->getAnnots( &objAnn ) );
        objAnn.free();
        int annotsNumber = annots->getNumAnnots();
        for ( int i = 0; i < annotsNumber; i++ )
        {
            if ( annots->getAnnot(i)->getAppearance( &objAnnStr )->isStream() )
            {
                objAnnStr.streamGetDict()->lookup( "Resources", &objAnnRes );
                if ( objAnnRes.isDict() )
                    addFonts( objAnnRes.getDict(), &fonts, fontsLen, fontsSize );
                objAnnRes.free();
            }
            objAnnStr.free();
        }
        delete annots;
    }
    // empty the font list and release mutex
    gfree( fonts );
    docLock.unlock();
    return &docFonts;
}

bool PDFGenerator::isAllowed( int permissions )
{
#if !KPDF_FORCE_DRM
    if (kapp->authorize("skip_drm") && !Settings::obeyDRM()) return true;
#endif

    bool b = true;
    if (permissions & KPDFDocument::AllowModify) b = b && pdfdoc->okToChange();
    if (permissions & KPDFDocument::AllowCopy) b = b && pdfdoc->okToCopy();
    if (permissions & KPDFDocument::AllowPrint) b = b && pdfdoc->okToPrint();
    if (permissions & KPDFDocument::AllowNotes) b = b && pdfdoc->okToAddNotes();
    return b;
}

bool PDFGenerator::canGeneratePixmap()
{
    return ready;
}

void PDFGenerator::generatePixmap( PixmapRequest * request )
{
#ifndef NDEBUG
    if ( !ready )
        kdDebug() << "calling generatePixmap() when not in READY state!" << endl;
#endif
    // update busy state (not really needed here, because the flag needs to
    // be set only to prevent asking a pixmap while the thread is running)
    ready = false;

    // debug requests to this (xpdf) generator
    //kdDebug() << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "]." << endl;

    /** asyncronous requests (generation in PDFPixmapGeneratorThread::run() **/
    if ( request->async )
    {
        // start the generation into the thread
        generatorThread->startGeneration( request );
        return;
    }

    /** syncronous request: in-place generation **/
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
    kpdfOutputDev->setParams( request->width, request->height, genTextPage, genObjectRects, genObjectRects, false );
    pdfdoc->displayPage( kpdfOutputDev, page->number() + 1, fakeDpiX, fakeDpiY, 0, true, genObjectRects );

    // 2. Take data from outputdev and attach it to the Page
    page->setPixmap( request->id, kpdfOutputDev->takePixmap() );
    if ( genTextPage )
        page->setSearchPage( abstractTextPage(kpdfOutputDev->takeTextPage(), page->height(), page->width()));
    if ( genObjectRects )
        page->setObjectRects( kpdfOutputDev->takeObjectRects() );

    // 3. UNLOCK [re-enables shared access]
    docLock.unlock();

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
    // build a TextPage using the lightweight KPDFTextDev generator..
    KPDFTextDev td;
    docLock.lock();
    pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, true, false );
    // ..and attach it to the page
    page->setSearchPage( abstractTextPage(td.takeTextPage(), page->height(), page->width()) );
    docLock.unlock();
}

bool PDFGenerator::print( KPrinter& printer )
{
    QString ps = printer.option("PageSize");
    if (ps.find(QRegExp("w\\d+h\\d+")) == 0)
    {
        // size not supported by Qt, KPrinter gives us the size as wWIDTHhHEIGHT
        // remove the w
        ps = ps.mid(1);
        int hPos = ps.find("h");
        globalParams->setPSPaperWidth(ps.left(hPos).toInt());
        globalParams->setPSPaperHeight(ps.mid(hPos+1).toInt());
    }
    else
    {
        // size is supported by Qt, we get either the pageSize name or nothing because the default pageSize is used
        QPrinter dummy(QPrinter::PrinterResolution);
        dummy.setFullPage(true);
        dummy.setPageSize((QPrinter::PageSize)(ps.isEmpty() ? KGlobal::locale()->pageSize() : pageNameToPageSize(ps)));

        QPaintDeviceMetrics metrics(&dummy);
        globalParams->setPSPaperWidth(metrics.width());
        globalParams->setPSPaperHeight(metrics.height());
    }

    KTempFile tf( QString::null, ".ps" );
    PSOutputDev *psOut = new PSOutputDev(tf.name().latin1(), pdfdoc->getXRef(), pdfdoc->getCatalog(), 1, pdfdoc->getNumPages(), psModePS);

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
            for(int i = 1; i <= pdfdoc->getNumPages(); i++) pages.push_back(i);
        }

        docLock.lock();
        pdfdoc->displayPages(psOut, pages, 72, 72, 0, globalParams->getPSCrop(), gFalse);
        docLock.unlock();

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

QString PDFGenerator::getMetaData( const QString & key, const QString & option )
{
    if ( key == "StartFullScreen" )
    {
        // asking for the 'start in fullscreen mode' (pdf property)
        if ( pdfdoc->getCatalog()->getPageMode() == Catalog::FullScreen )
            return "yes";
    }
    else if ( key == "NamedViewport" && !option.isEmpty() )
    {
        // asking for the page related to a 'named link destination'. the
        // option is the link name. @see addSynopsisChildren.
        DocumentViewport viewport;
        GString * namedDest = new GString( option.latin1() );
        docLock.lock();
        LinkDest * destination = pdfdoc->findDest( namedDest );
        if ( destination )
        {
            if ( !destination->isPageRef() )
                viewport.pageNumber = destination->getPageNum() - 1;
            else
            {
                Ref ref = destination->getPageRef();
                viewport.pageNumber = pdfdoc->findPage( ref.num, ref.gen ) - 1;
            }
        }
        docLock.unlock();
        delete namedDest;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    return QString();
}

bool PDFGenerator::reparseConfig()
{
    // load paper color from Settings or use the white default color
    QColor color = ( (Settings::renderMode() == Settings::EnumRenderMode::Paper ) &&
                     Settings::changeColors() ) ? Settings::paperColor() : Qt::white;
    // if paper color is changed we have to rebuild every visible pixmap in addition
    // to the outputDevice. it's the 'heaviest' case, other effect are just recoloring
    // over the page rendered on 'standard' white background.
    if ( color != paperColor || !kpdfOutputDev )
    {
        paperColor = color;
        SplashColor splashCol;
        splashCol.rgb8 = splashMakeRGB8( paperColor.red(), paperColor.green(), paperColor.blue() );
        // rebuild the output device using the new paper color and initialize it
        docLock.lock();
        delete kpdfOutputDev;
        kpdfOutputDev = new KPDFOutputDev( splashCol );
        if ( pdfdoc )
            kpdfOutputDev->initDevice( pdfdoc );
        docLock.unlock();
        return true;
    }
    return false;
}
//END Generator inherited functions

static QString unicodeToQString(Unicode* u, int len) {
    QString ret;
    // we dont want to move that pointer
    ret.setLength(len);
    QChar* qch = (QChar*) ret.unicode();
    for (;len;--len)
      *qch++ = (QChar) *u++;
    return ret;
}

inline void append (KPDFTextPage* ktp,
    QString s, double l, double b, double r, double t, 
    double baseline, double rot, bool eoline)
{
                ktp->append( s ,
                    new NormalizedRect(
                    l,
                    b,
                    r,
                    t
                    ),
                    baseline,
                    rot, eoline);
}

inline void append(KPDFTextPage* ktp,
    Unicode g, double l, double b, double r, double t, 
    double baseline, double rot, bool eoline)
{
                Unicode * glyph=new Unicode(g);
                QString s=unicodeToQString( glyph, 1);
                append(ktp,
                s, l, b, r, t, 
                baseline, rot, eoline);
                delete glyph;
}

KPDFTextPage * PDFGenerator::abstractTextPage(TextPage *tp, double height, double width)
{
    KPDFTextPage* ktp=new KPDFTextPage;
    TextWordList *list = tp->makeWordList(true);
    TextWord * word, *next; 
    double baseline;
    int wordCount=list->getLength();
    int charCount=0;
    int i,j,rot;
    QString s;
    NormalizedRect * wordRect = new NormalizedRect;
    for (i=0;i<wordCount;i++)
    {
        word=list->get(i);
        word->getBBox(&wordRect->left,&wordRect->bottom,&wordRect->right,&wordRect->top);
        charCount=word->getLength();
        rot=word->getRotation();
        baseline=word->getBaseline();
        next=word->nextWord();

        switch (rot)
        {
            case 0:
            // 0 degrees, normal word boundaries are top and bottom
            // only x boundaries change the order of letters is normal not reversed
            for (j=0;j<charCount;j++)
            {
                append(ktp,word->getChar(j) ,
                    // this letters boundary
                    word->getEdge(j)/width,
                    wordRect->bottom/height,
                    // next letters boundary
                    word->getEdge(j+1)/width,
                    wordRect->top/height,
                    baseline,
                    rot,
                    !next);
            }
            break;
            
            case 1:
            // 90 degrees, x boundaries not changed
            // y ones change, the order of letters is normal not reversed
            for (j=0;j<charCount;j++)
            {
                append(ktp,word->getChar(j) ,
                    wordRect->left/width,
                    word->getEdge(j)/height,
                    wordRect->right/width,
                    word->getEdge(j+1)/height,
                    baseline,
                    rot,
                    !next);
            }
            break;

            case 2:
            // same as case 0 but reversed order of letters
            for (j=0;j<charCount;j++)
            {
                append(ktp,word->getChar(j) ,
                    word->getEdge(j+1)/width,
                    wordRect->bottom/height,
                    word->getEdge(j)/width,
                    wordRect->top/height,
                    baseline,
                    rot,
                    !next);

            }
            break;
            
            case 3:
            for (j=0;j<charCount;j++)
            {
                append(ktp,word->getChar(j) ,
                    wordRect->left/width,
                    word->getEdge(j+1)/height,
                    wordRect->right/width,
                    word->getEdge(j)/height,
                    baseline,
                    rot,
                    !next);
            }
            break;
        }
        // if is followed by a space and is not last on the line 
        if ( word->hasSpaceAfter() == gTrue && next )
        {
            

            switch (rot)
            {
                case 0:
                    append(ktp,QString(" ")  ,
                        word->getEdge(charCount)/width,
                        wordRect->bottom/height,
                        next->getEdge(0)/width,
                        wordRect->top/height,
                        baseline,
                        rot,
                        false);
                break;
                
                case 1:
                    append(ktp,QString(" ")  ,
                        wordRect->left/width,
                        word->getEdge(charCount)/height,
                        wordRect->right/width,
                        next->getEdge(0)/height,
                        baseline,
                        rot,
                        false);
                break;
    
                case 2:
                    append(ktp,QString(" ") ,
                        next->getEdge(0)/width,
                        wordRect->bottom/height,
                        word->getEdge(charCount)/width,
                        wordRect->top/height,
                        baseline,
                        rot,
                        false);
    
                break;
                
                case 3:
                    append(ktp,QString(" ")  ,
                        wordRect->left/width,
                        next->getEdge(0)/height,
                        wordRect->right/width,
                        word->getEdge(charCount)/height,
                        baseline,
                        rot,
                        false);
                break;
            }

        }
    }

    delete wordRect;
    delete tp;
    return ktp;
}

QString PDFGenerator::getDocumentInfo( const QString & data ) const
// note: MUTEX is LOCKED while calling this
{
    // [Albert] Code adapted from pdfinfo.cc on xpdf
    Object info;
    if ( !pdfdoc )
        return i18n( "Unknown" );

    pdfdoc->getDocInfo( &info );
    if ( !info.isDict() )
        return i18n( "Unknown" );

    QString result;
    Object obj;
    GString *s1;
    GBool isUnicode;
    Unicode u;
    int i;
    Dict *infoDict = info.getDict();

    if ( infoDict->lookup( data.latin1(), &obj )->isString() )
    {
        s1 = obj.getString();
        if ( ( s1->getChar(0) & 0xff ) == 0xfe && ( s1->getChar(1) & 0xff ) == 0xff )
        {
            isUnicode = gTrue;
            i = 2;
        }
        else
        {
            isUnicode = gFalse;
            i = 0;
        }
        while ( i < obj.getString()->getLength() )
        {
            if ( isUnicode )
            {
                u = ( ( s1->getChar(i) & 0xff ) << 8 ) | ( s1->getChar(i+1) & 0xff );
                i += 2;
            }
            else
            {
                u = s1->getChar(i) & 0xff;
                ++i;
            }
            result += unicodeToQString( &u, 1 );
        }
        obj.free();
        info.free();
        return result;
    }
    obj.free();
    info.free();
    return i18n( "Unknown" );
}

QString PDFGenerator::getDocumentDate( const QString & data ) const
// note: MUTEX is LOCKED while calling this
{
    // [Albert] Code adapted from pdfinfo.cc on xpdf
    if ( !pdfdoc )
        return i18n( "Unknown Date" );

    Object info;
    pdfdoc->getDocInfo( &info );
    if ( !info.isDict() )
        return i18n( "Unknown Date" );

    Object obj;
    char *s;
    int year, mon, day, hour, min, sec;
    Dict *infoDict = info.getDict();
    UnicodeMap *uMap = globalParams->getTextEncoding();
    QString result;

    if ( !uMap )
        return i18n( "Unknown Date" );

    if ( infoDict->lookup( data.latin1(), &obj )->isString() )
    {
        s = obj.getString()->getCString();
        if ( s[0] == 'D' && s[1] == ':' )
            s += 2;

        if ( sscanf( s, "%4d%2d%2d%2d%2d%2d", &year, &mon, &day, &hour, &min, &sec ) == 6 )
        {
            QDate d( year, mon, day );  //CHECK: it was mon-1, Jan->0 (??)
            QTime t( hour, min, sec );
            if ( d.isValid() && t.isValid() )
                result = KGlobal::locale()->formatDateTime( QDateTime(d, t), false, true );
            else
                result = s;
        }
        else
            result = s;
    }
    else
        result = i18n( "Unknown Date" );
    obj.free();
    info.free();
    return result;
}

void PDFGenerator::addSynopsisChildren( QDomNode * parent, GList * items )
{
    int numItems = items->getLength();
    for ( int i = 0; i < numItems; ++i )
    {
        // iterate over every object in 'items'
        OutlineItem * outlineItem = (OutlineItem *)items->get( i );

        // 1. create element using outlineItem's title as tagName
        QString name;
        Unicode * uniChar = outlineItem->getTitle();
        int titleLength = outlineItem->getTitleLength();
        name = unicodeToQString(uniChar, titleLength);
        if ( name.isEmpty() )
            continue;
        QDomElement item = docSyn.createElement( name );
        parent->appendChild( item );

        // 2. find the page the link refers to
        LinkAction * a = outlineItem->getAction();
        if ( a && a->getKind() == actionGoTo )
        {
            // page number is contained/referenced in a LinkGoTo
            LinkGoTo * g = static_cast< LinkGoTo * >( a );
            LinkDest * destination = g->getDest();
            if ( !destination && g->getNamedDest() )
            {
                // no 'destination' but an internal 'named reference'. we could
                // get the destination for the page now, but it's VERY time consuming,
                // so better storing the reference and provide the viewport as metadata
                // on demand
                item.setAttribute( "ViewportName", g->getNamedDest()->getCString() );
            }
            else if ( destination->isOk() )
            {
                // we have valid 'destination' -> get page number
                int pageNumber = destination->getPageNum() - 1;
                if ( destination->isPageRef() )
                {
                    Ref ref = destination->getPageRef();
                    pageNumber = pdfdoc->findPage( ref.num, ref.gen ) - 1;
                }
                // set page as attribute to node
                // TODO add other attributes to the viewport (taken from link)
                item.setAttribute( "Viewport", DocumentViewport( pageNumber ).toString() );
            }
	    }
	    else if ( a && a->getKind() == actionGoToR )
        {
            LinkGoToR * g = static_cast< LinkGoToR * >( a );
            LinkDest * destination = g->getDest();
            if ( !destination && g->getNamedDest() )
            {
                item.setAttribute( "ViewportName", g->getNamedDest()->getCString() );
            }
 
            item.setAttribute( "ExternalFileName", g->getFileName()->getCString() );
        }

        // 3. recursively descend over children
        outlineItem->open();
        GList * children = outlineItem->getKids();
        if ( children )
            addSynopsisChildren( &item, children );
    }
}

void PDFGenerator::addFonts( Dict *resDict, Ref **fonts, int &fontsLen, int &fontsSize )
{
    Object obj1, obj2;

    // get the Font dictionary in current resource dictionary
    GfxFontDict * gfxFontDict = NULL;
    resDict->lookupNF( "Font", &obj1 );
    if ( obj1.isRef() )
    {
        obj1.fetch( pdfdoc->getXRef(), &obj2 );
        if ( obj2.isDict() )
        {
            Ref r = obj1.getRef();
            gfxFontDict = new GfxFontDict( pdfdoc->getXRef(), &r, obj2.getDict() );
        }
        obj2.free();
    }
    else if ( obj1.isDict() )
        gfxFontDict = new GfxFontDict( pdfdoc->getXRef(), NULL, obj1.getDict() );

    // add all fonts in the Font dictionary
    if ( gfxFontDict )
    {
        for ( int i = 0; i < gfxFontDict->getNumFonts(); i++ )
        {
            GfxFont * font = gfxFontDict->getFont( i );
            if ( !font )
                continue;

            // skip already added fonts
            Ref fontRef = *font->getID();
            bool alreadyIn = false;
            for ( int j = 0; j < fontsLen && !alreadyIn; j++ )
                if ( fontRef.num == (*fonts)[j].num && fontRef.gen == (*fonts)[j].gen )
                    alreadyIn = true;
            if ( alreadyIn )
                continue;

            // 0. add font element
            QDomElement fontElem = docFonts.createElement( "font" );
            docFonts.firstChild().appendChild( fontElem );

            // 1. set Name
            GString * name = font->getOrigName();
            fontElem.setAttribute( "Name", name ? name->getCString() : i18n("[none]") );

            // 2. set Type
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
            fontElem.setAttribute( "Type", fontTypeNames[ font->getType() ] );

            // 3. set Embedded
            Ref embRef;
            bool emb = font->getType() == fontType3 ? true : font->getEmbeddedFontID( &embRef );
            fontElem.setAttribute( "Embedded", emb ? i18n("Yes") : i18n("No") );

            // 4. set Path
            QString sPath = i18n("-");
            if ( name && !emb  )
            {
                DisplayFontParam *dfp = globalParams->getDisplayFont( name );
                if ( dfp )
                {
                    if ( dfp -> kind == displayFontT1 ) sPath = dfp->t1.fileName->getCString();
                    else sPath = dfp->tt.fileName->getCString();
                }
            }
            fontElem.setAttribute( "File", sPath );

            // enlarge font list if needed
            if ( fontsLen == fontsSize )
            {
                fontsSize += 32;
                *fonts = (Ref *)grealloc( *fonts, fontsSize * sizeof(Ref) );
            }
            // add this font to the list
            (*fonts)[fontsLen++] = fontRef;
        }
        delete gfxFontDict;
    }
    obj1.free();

    // recursively scan any resource dictionaries in objects in this dictionary
    Object xObjDict, xObj, resObj;
    resDict->lookup( "XObject", &xObjDict );
    if ( xObjDict.isDict() )
    {
        for ( int i = 0; i < xObjDict.dictGetLength(); i++ )
        {
            xObjDict.dictGetVal(i, &xObj);
            if ( xObj.isStream() )
            {
                xObj.streamGetDict()->lookup( "Resources", &resObj );
                if ( resObj.isDict() )
                    addFonts( resObj.getDict(), fonts, fontsLen, fontsSize );
                resObj.free();
            }
            xObj.free();
        }
    }
    xObjDict.free();
}


class XPDFReader
{
    public:
        // find named symbol and parse it
        static void lookupName( Dict *, const char *, QString & dest );
        static void lookupString( Dict *, const char *, QString & dest );
        static void lookupBool( Dict *, const char *, bool & dest );
        static void lookupInt( Dict *, const char *, int & dest );
        static void lookupNum( Dict *, const char *, double & dest );
        static int lookupNumArray( Dict *, const char *, double * dest, int len );
        static void lookupColor( Dict *, const char *, QColor & color );
        static void lookupIntRef( Dict *, const char *, int & dest );
        static void lookupDate( Dict *, const char *, QDateTime & dest );
        // transform from user coords to normalized ones using the matrix M
        static inline void transform( double * M, double x, double y, double * tx, double * ty );
};

void XPDFReader::lookupName( Dict * dict, const char * type, QString & dest )
{
    Object nameObj;
    dict->lookup( type, &nameObj );
    if ( nameObj.isNull() )
        return;
    if ( nameObj.isName() )
        dest = nameObj.getName();
    else
        kdDebug() << type << " is not Name." << endl;
    nameObj.free();
}

void XPDFReader::lookupString( Dict * dict, const char * type, QString & dest )
{
    Object stringObj;
    dict->lookup( type, &stringObj );
    if ( stringObj.isNull() )
        return;
    if ( stringObj.isString() )
        dest = stringObj.getString()->getCString();
    else
        kdDebug() << type << " is not String." << endl;
    stringObj.free();
}

void XPDFReader::lookupBool( Dict * dict, const char * type, bool & dest )
{
    Object boolObj;
    dict->lookup( type, &boolObj );
    if ( boolObj.isNull() )
        return;
    if ( boolObj.isBool() )
        dest = boolObj.getBool() == gTrue;
    else
        kdDebug() << type << " is not Bool." << endl;
    boolObj.free();
}

void XPDFReader::lookupInt( Dict * dict, const char * type, int & dest )
{
    Object intObj;
    dict->lookup( type, &intObj );
    if ( intObj.isNull() )
        return;
    if ( intObj.isInt() )
        dest = intObj.getInt();
    else
        kdDebug() << type << " is not Int." << endl;
    intObj.free();
}

void XPDFReader::lookupNum( Dict * dict, const char * type, double & dest )
{
    Object numObj;
    dict->lookup( type, &numObj );
    if ( numObj.isNull() )
        return;
    if ( numObj.isNum() )
        dest = numObj.getNum();
    else
        kdDebug() << type << " is not Num." << endl;
    numObj.free();
}

int XPDFReader::lookupNumArray( Dict * dict, const char * type, double * dest, int len )
{
    Object arrObj;
    dict->lookup( type, &arrObj );
    if ( arrObj.isNull() )
        return 0;
    Object numObj;
    if ( arrObj.isArray() )
    {
        len = QMIN( len, arrObj.arrayGetLength() );
        for ( int i = 0; i < len; i++ )
        {
            dest[i] = arrObj.arrayGet( i, &numObj )->getNum();
            numObj.free();
        }
    }
    else
    {
        len = 0;
        kdDebug() << type << "is not Array." << endl;
    }
    arrObj.free();
    return len;
}

void XPDFReader::lookupColor( Dict * dict, const char * type, QColor & dest )
{
    double c[3];
    if ( XPDFReader::lookupNumArray( dict, type, c, 3 ) == 3 )
        dest = QColor( (int)(c[0]*255.0), (int)(c[1]*255.0), (int)(c[2]*255.0));
}

void XPDFReader::lookupIntRef( Dict * dict, const char * type, int & dest )
{
    Object refObj;
    dict->lookupNF( type, &refObj );
    if ( refObj.isNull() )
        return;
    if ( refObj.isRef() )
        dest = refObj.getRefNum();
    else
        kdDebug() << type << " is not Ref." << endl;
    refObj.free();
}

void XPDFReader::lookupDate( Dict * dict, const char * type, QDateTime & dest )
{
    Object dateObj;
    dict->lookup( type, &dateObj );
    if ( dateObj.isNull() )
        return;
    if ( dateObj.isString() )
    {
        const char * s = dateObj.getString()->getCString();
        if ( s[0] == 'D' && s[1] == ':' )
            s += 2;
        int year, mon, day, hour, min, sec;
        if ( sscanf( s, "%4d%2d%2d%2d%2d%2d", &year, &mon, &day, &hour, &min, &sec ) == 6 )
        {
            QDate d( year, mon, day );
            QTime t( hour, min, sec );
            if ( d.isValid() && t.isValid() )
                dest = QDateTime(d, t);
        }
        else
            kdDebug() << "Wrong Date format '" << s << "' for '" << type << "'." << endl;
    }
    else
        kdDebug() << type << " is not Date" << endl;
    dateObj.free();
}

void XPDFReader::transform( double * M, double x, double y, double * tx, double * ty )
{
    *tx = M[0] * x + M[2] * y + M[4];
    *ty = M[1] * x + M[3] * y + M[5];
}

/** @short Helper classes for CROSSDEPS resolving and DS conversion. */
struct ResolveRevision
{
    int           prevAnnotationID; // ID of the annotation to be reparended
    int           nextAnnotationID; // (only needed for speeding up resolving)
    Annotation *  nextAnnotation;   // annotation that will act as parent
    Annotation::RevScope nextScope; // scope of revision (Reply)
    Annotation::RevType  nextType;  // type of revision (None)
};

struct ResolveWindow
{
    int           popupWindowID;    // ID of the (maybe shared) window
    Annotation *  annotation;       // annotation having the popup window
};

struct PostProcessText              // this handles a special pdf case conversion
{
    Annotation *  textAnnotation;   // a popup text annotation (not FreeText)
    bool          opened;           // pdf property to convert to window flags
};

struct PopupWindow
{
    Annotation *  dummyAnnotation;  // window properties (in pdf as Annotation)
    bool          shown;            // converted to Annotation::Hidden flag
};

void PDFGenerator::addAnnotations( Page * pdfPage, KPDFPage * page )
// called on opening when MUTEX is not used
{
    Object annotArray;
    pdfPage->getAnnots( &annotArray );
    if ( !annotArray.isArray() || annotArray.arrayGetLength() < 1 )
        return;

    // ID to Annotation/PopupWindow maps
    QMap< int, Annotation * > annotationsMap;
    QMap< int, PopupWindow * > popupsMap;
    // lists of Windows and Revisions that needs resolution
    QValueList< ResolveRevision > resolveRevList;
    QValueList< ResolveWindow > resolvePopList;
    QValueList< PostProcessText > ppTextList;

    // build a normalized transform matrix for this page at 100% scale
    GfxState * gfxState = new GfxState( 72.0, 72.0, pdfPage->getMediaBox(), pdfPage->getRotate(), gTrue );
    double * gfxCTM = gfxState->getCTM();
    double MTX[6];
    for ( int i = 0; i < 6; i+=2 )
    {
        MTX[i] = gfxCTM[i] / pdfPage->getWidth();
        MTX[i+1] = gfxCTM[i+1] / pdfPage->getHeight();
    }
    delete gfxState;

    /** 1 - PARSE ALL ANNOTATIONS AND POPUPS FROM THE PAGE */
    Object annot;
    Object annotRef;    // no need to free this (impl. dependent!)
    uint numAnnotations = annotArray.arrayGetLength();
    for ( uint j = 0; j < numAnnotations; j++ )
    {
        // get the j-th annotation
        annotArray.arrayGet( j, &annot );
        if ( !annot.isDict() )
        {
            kdDebug() << "PDFGenerator: annot not dictionary." << endl;
            annot.free();
            continue;
        }

        Annotation * annotation = 0;
        Dict * annotDict = annot.getDict();
        int annotID = annotArray.arrayGetNF( j, &annotRef )->getRefNum();
        bool parseMarkup = true,    // nearly all supported annots are markup
             addToPage = true;      // Popup annots are added to custom queue

        /** 1.1. GET Subtype */
        QString subType;
        XPDFReader::lookupName( annotDict, "Subtype", subType );
        if ( subType.isEmpty() )
        {
            kdDebug() << "annot has no Subtype" << endl;
            annot.free();
            continue;
        }

        /** 1.2. CREATE Annotation / PopupWindow and PARSE specific params */
        if ( subType == "Text" || subType == "FreeText" )
        {
            // parse TextAnnotation params
            TextAnnotation * t = new TextAnnotation();
            annotation = t;

            if ( subType == "Text" )
            {
                // -> textType
                t->textType = TextAnnotation::Linked;
                // -> textIcon
                XPDFReader::lookupName( annotDict, "Name", t->textIcon );
                if ( !t->textIcon.isEmpty() )
                {
                    t->textIcon = t->textIcon.lower();
                    t->textIcon.remove( ' ' );
                }
                // request for postprocessing window geometry
                PostProcessText request;
                request.textAnnotation = t;
                request.opened = false;
                XPDFReader::lookupBool( annotDict, "Open", request.opened );
                ppTextList.append( request );
            }
            else
            {
                // NOTE: please provide testcases for FreeText (don't have any) - Enrico
                // -> textType
                t->textType = TextAnnotation::InPlace;
                // -> textFont
                QString textFormat;
                XPDFReader::lookupString( annotDict, "DA", textFormat );
                // TODO, fill t->textFont using textFormat if not empty
                // -> inplaceAlign
                XPDFReader::lookupInt( annotDict, "Q", t->inplaceAlign );
                // -> inplaceText (simple)
                XPDFReader::lookupString( annotDict, "DS", t->inplaceText );
                // -> inplaceText (complex override)
                XPDFReader::lookupString( annotDict, "RC", t->inplaceText );
                // -> inplaceCallout
                double c[6];
                int n = XPDFReader::lookupNumArray( annotDict, "CL", c, 6 );
                if ( n >= 4 )
                {
                    XPDFReader::transform( MTX, c[0], c[1], &t->inplaceCallout[0].x, &t->inplaceCallout[0].y );
                    XPDFReader::transform( MTX, c[2], c[3], &t->inplaceCallout[1].x, &t->inplaceCallout[1].y );
                    if ( n == 6 )
                        XPDFReader::transform( MTX, c[4], c[5], &t->inplaceCallout[2].x, &t->inplaceCallout[2].y );
                }
                // -> inplaceIntent
                QString intentName;
                XPDFReader::lookupString( annotDict, "IT", intentName );
                if ( intentName == "FreeTextCallout" )
                    t->inplaceIntent = TextAnnotation::Callout;
                else if ( intentName == "FreeTextTypeWriter" )
                    t->inplaceIntent = TextAnnotation::TypeWriter;
            }
        }
        else if ( subType == "Line" || subType == "Polygon" || subType == "PolyLine" )
        {
            // parse LineAnnotation params
            LineAnnotation * l = new LineAnnotation();
            annotation = l;

            // -> linePoints
            double c[100];
            int num = XPDFReader::lookupNumArray( annotDict, (subType == "Line") ? "L" : "Vertices", c, 100 );
            if ( num < 4 || (num % 2) != 0 )
            {
                kdDebug() << "L/Vertices wrong fol Line/Poly." << endl;
                delete annotation;
                annot.free();
                continue;
            }
            for ( int i = 0; i < num; i += 2 )
            {
                NormalizedPoint p;
                XPDFReader::transform( MTX, c[i], c[i+1], &p.x, &p.y );
                l->linePoints.push_back( p );
            }
            // -> lineStartStyle, lineEndStyle
            Object leArray;
            annotDict->lookup( "LE", &leArray );
            if ( leArray.isArray() && leArray.arrayGetLength() == 2 )
            {
                // -> lineStartStyle
                Object styleObj;
                leArray.arrayGet( 0, &styleObj );
                if ( styleObj.isName() )
                {
                    const char * name = styleObj.getName();
                    if ( !strcmp( name, "Square" ) )
                        l->lineStartStyle = LineAnnotation::Square;
                    else if ( !strcmp( name, "Circle" ) )
                        l->lineStartStyle = LineAnnotation::Circle;
                    else if ( !strcmp( name, "Diamond" ) )
                        l->lineStartStyle = LineAnnotation::Diamond;
                    else if ( !strcmp( name, "OpenArrow" ) )
                        l->lineStartStyle = LineAnnotation::OpenArrow;
                    else if ( !strcmp( name, "ClosedArrow" ) )
                        l->lineStartStyle = LineAnnotation::ClosedArrow;
                    else if ( !strcmp( name, "None" ) )
                        l->lineStartStyle = LineAnnotation::None;
                    else if ( !strcmp( name, "Butt" ) )
                        l->lineStartStyle = LineAnnotation::Butt;
                    else if ( !strcmp( name, "ROpenArrow" ) )
                        l->lineStartStyle = LineAnnotation::ROpenArrow;
                    else if ( !strcmp( name, "RClosedArrow" ) )
                        l->lineStartStyle = LineAnnotation::RClosedArrow;
                    else if ( !strcmp( name, "Slash" ) )
                        l->lineStartStyle = LineAnnotation::Slash;
                }
                styleObj.free();
                // -> lineEndStyle
                leArray.arrayGet( 1, &styleObj );
                if ( styleObj.isName() )
                {
                    const char * name = styleObj.getName();
                    if ( !strcmp( name, "Square" ) )
                        l->lineEndStyle = LineAnnotation::Square;
                    else if ( !strcmp( name, "Circle" ) )
                        l->lineEndStyle = LineAnnotation::Circle;
                    else if ( !strcmp( name, "Diamond" ) )
                        l->lineEndStyle = LineAnnotation::Diamond;
                    else if ( !strcmp( name, "OpenArrow" ) )
                        l->lineEndStyle = LineAnnotation::OpenArrow;
                    else if ( !strcmp( name, "ClosedArrow" ) )
                        l->lineEndStyle = LineAnnotation::ClosedArrow;
                    else if ( !strcmp( name, "None" ) )
                        l->lineEndStyle = LineAnnotation::None;
                    else if ( !strcmp( name, "Butt" ) )
                        l->lineEndStyle = LineAnnotation::Butt;
                    else if ( !strcmp( name, "ROpenArrow" ) )
                        l->lineEndStyle = LineAnnotation::ROpenArrow;
                    else if ( !strcmp( name, "RClosedArrow" ) )
                        l->lineEndStyle = LineAnnotation::RClosedArrow;
                    else if ( !strcmp( name, "Slash" ) )
                        l->lineEndStyle = LineAnnotation::Slash;
                }
                styleObj.free();
            }
            leArray.free();
            // -> lineClosed
            l->lineClosed = subType == "Polygon";
            // -> lineInnerColor
            XPDFReader::lookupColor( annotDict, "IC", l->lineInnerColor );
            // -> lineLeadingFwdPt
            XPDFReader::lookupNum( annotDict, "LL", l->lineLeadingFwdPt );
            // -> lineLeadingBackPt
            XPDFReader::lookupNum( annotDict, "LLE", l->lineLeadingBackPt );
            // -> lineShowCaption
            XPDFReader::lookupBool( annotDict, "Cap", l->lineShowCaption );
            // -> lineIntent
            QString intentName;
            XPDFReader::lookupString( annotDict, "IT", intentName );
            if ( intentName == "LineArrow" )
                l->lineIntent = LineAnnotation::Arrow;
            else if ( intentName == "LineDimension" )
                l->lineIntent = LineAnnotation::Dimension;
            else if ( intentName == "PolygonCloud" )
                l->lineIntent = LineAnnotation::PolygonCloud;
        }
        else if ( subType == "Square" || subType == "Circle" )
        {
            // parse GeomAnnotation params
            GeomAnnotation * g = new GeomAnnotation();
            annotation = g;

            // -> geomType
            if ( subType == "Square" )
                g->geomType = GeomAnnotation::InscribedSquare;
            else
                g->geomType = GeomAnnotation::InscribedCircle;
            // -> geomInnerColor
            XPDFReader::lookupColor( annotDict, "IC", g->geomInnerColor );
            // TODO RD
        }
        else if ( subType == "Highlight" || subType == "Underline" ||
                  subType == "Squiggly" || subType == "StrikeOut" )
        {
            // parse HighlightAnnotation params
            HighlightAnnotation * h = new HighlightAnnotation();
            annotation = h;

            // -> highlightType
            if ( subType == "Highlight" )
                h->highlightType = HighlightAnnotation::Highlight;
            else if ( subType == "Underline" )
                h->highlightType = HighlightAnnotation::Underline;
            else if ( subType == "Squiggly" )
                h->highlightType = HighlightAnnotation::Squiggly;
            else if ( subType == "StrikeOut" )
                h->highlightType = HighlightAnnotation::StrikeOut;

            // -> highlightQuads
            double c[80];
            int num = XPDFReader::lookupNumArray( annotDict, "QuadPoints", c, 80 );
            if ( num < 8 || (num % 8) != 0 )
            {
                kdDebug() << "Wrong QuadPoints for a Highlight annotation." << endl;
                delete annotation;
                annot.free();
                continue;
            }
            for ( int q = 0; q < num; q += 8 )
            {
                HighlightAnnotation::Quad quad;
                for ( int p = 0; p < 4; p++ )
                    XPDFReader::transform( MTX, c[ q + p*2 ], c[ q + p*2 + 1 ],
                        &quad.points[ p ].x, &quad.points[ p ].y );
                // ### PDF1.6 specs says that point are in ccw order, but in fact
                // points 3 and 4 are swapped in every PDF around!
                NormalizedPoint tmpPoint = quad.points[ 2 ];
                quad.points[ 2 ] = quad.points[ 3 ];
                quad.points[ 3 ] = tmpPoint;
                // initialize other oroperties and append quad
                quad.capStart = true;       // unlinked quads are always capped
                quad.capEnd = true;         // unlinked quads are always capped
                quad.feather = 0.1;         // default feather
                h->highlightQuads.append( quad );
            }
        }
        else if ( subType == "Stamp" )
        {
            // parse StampAnnotation params
            StampAnnotation * s = new StampAnnotation();
            annotation = s;

            // -> stampIconName
            XPDFReader::lookupName( annotDict, "Name", s->stampIconName );
        }
        else if ( subType == "Ink" )
        {
            // parse InkAnnotation params
            InkAnnotation * k = new InkAnnotation();
            annotation = k;

            // -> inkPaths
            Object pathsArray;
            annotDict->lookup( "InkList", &pathsArray );
            if ( !pathsArray.isArray() || pathsArray.arrayGetLength() < 1 )
            {
                kdDebug() << "InkList not present for ink annot" << endl;
                delete annotation;
                annot.free();
                continue;
            }
            int pathsNumber = pathsArray.arrayGetLength();
            for ( int m = 0; m < pathsNumber; m++ )
            {
                // transform each path in a list of normalized points ..
                QValueList<NormalizedPoint> localList;
                Object pointsArray;
                pathsArray.arrayGet( m, &pointsArray );
                if ( pointsArray.isArray() )
                {
                    int pointsNumber = pointsArray.arrayGetLength();
                    for ( int n = 0; n < pointsNumber; n+=2 )
                    {
                        // get the x,y numbers for current point
                        Object numObj;
                        double x = pointsArray.arrayGet( n, &numObj )->getNum();
                        numObj.free();
                        double y = pointsArray.arrayGet( n+1, &numObj )->getNum();
                        numObj.free();
                        // add normalized point to localList
                        NormalizedPoint np;
                        XPDFReader::transform( MTX, x, y, &np.x, &np.y );
                        localList.push_back( np );
                    }
                }
                pointsArray.free();
                // ..and add it to the annotation
                k->inkPaths.push_back( localList );
            }
            pathsArray.free();
        }
        else if ( subType == "Popup" )
        {
            // create PopupWindow and add it to the popupsMap
            PopupWindow * popup = new PopupWindow();
            popupsMap[ annotID ] = popup;
            parseMarkup = false;
            addToPage = false;

            // get window specific properties if any
            popup->shown = false;
            XPDFReader::lookupBool( annotDict, "Open", popup->shown );
            // no need to parse parent annotation id
            //XPDFReader::lookupIntRef( annotDict, "Parent", popup->... );

            // use the 'dummy annotation' for getting other parameters
            popup->dummyAnnotation = new Annotation();
            annotation = popup->dummyAnnotation;
        }
        else if ( subType == "Link" )
        {
            // ignore links (this may change in future)
            annot.free();
            continue;
        }
        else
        {
            // MISSING: Caret, FileAttachment, Sound, Movie, Widget,
            //          Screen, PrinterMark, TrapNet, Watermark, 3D
            kdDebug() << "annotation '" << subType << "' not supported" << endl;
            annot.free();
            continue;
        }

        /** 1.3. PARSE common parameters */
        // -> boundary
        double r[4];
        if ( XPDFReader::lookupNumArray( annotDict, "Rect", r, 4 ) != 4 )
        {
            kdDebug() << "Rect is missing for annotation." << endl;
            annot.free();
            continue;
        }
        // transform annotation rect to uniform coords
        XPDFReader::transform( MTX, r[0], r[1], &annotation->boundary.left, &annotation->boundary.top );
        XPDFReader::transform( MTX, r[2], r[3], &annotation->boundary.right, &annotation->boundary.bottom );
        if ( annotation->boundary.left > annotation->boundary.right )
            qSwap<double>( annotation->boundary.left, annotation->boundary.right );
        if ( annotation->boundary.top > annotation->boundary.bottom )
            qSwap<double>( annotation->boundary.top, annotation->boundary.bottom );
           //annotation->rUnscaledWidth = (r[2] > r[0]) ? r[2] - r[0] : r[0] - r[2];
           //annotation->rUnscaledHeight = (r[3] > r[1]) ? r[3] - r[1] : r[1] - r[3];
        // -> contents
        XPDFReader::lookupString( annotDict, "Contents", annotation->contents );
        // -> uniqueName
        XPDFReader::lookupString( annotDict, "NM", annotation->uniqueName );
        // -> modifyDate (and -> creationDate)
        XPDFReader::lookupDate( annotDict, "M", annotation->modifyDate );
        if ( annotation->creationDate.isNull() && !annotation->modifyDate.isNull() )
            annotation->creationDate = annotation->modifyDate;
        // -> flags: set the external attribute since it's embedded on file
        annotation->flags |= Annotation::External;
        // -> flags
        int flags = 0;
        XPDFReader::lookupInt( annotDict, "F", flags );
        if ( flags & 0x2 )
            annotation->flags |= Annotation::Hidden;
        if ( flags & 0x8 )
            annotation->flags |= Annotation::FixedSize;
        if ( flags & 0x10 )
            annotation->flags |= Annotation::FixedRotation;
        if ( !(flags & 0x4) )
            annotation->flags |= Annotation::DenyPrint;
        if ( flags & 0x40 )
            annotation->flags |= (Annotation::DenyWrite | Annotation::DenyDelete);
        if ( flags & 0x80 )
            annotation->flags |= Annotation::DenyDelete;
        if ( flags & 0x100 )
            annotation->flags |= Annotation::ToggleHidingOnMouse;
        // -> style (Border(old spec), BS, BE)
        double border[3];
        int bn = XPDFReader::lookupNumArray( annotDict, "Border", border, 3 );
        if ( bn == 3 )
        {
            // -> style.xCorners
            annotation->style.xCorners = border[0];
            // -> style.yCorners
            annotation->style.yCorners = border[1];
            // -> style.width
            annotation->style.width = border[2];
        }
        Object bsObj;
        annotDict->lookup( "BS", &bsObj );
        if ( bsObj.isDict() )
        {
            // -> style.width
            XPDFReader::lookupNum( bsObj.getDict(), "W", annotation->style.width );
            // -> style.style
            QString styleName;
            XPDFReader::lookupName( bsObj.getDict(), "S", styleName );
            if ( styleName == "S" )
                annotation->style.style = Annotation::Solid;
            else if ( styleName == "D" )
                annotation->style.style = Annotation::Dashed;
            else if ( styleName == "B" )
                annotation->style.style = Annotation::Beveled;
            else if ( styleName == "I" )
                annotation->style.style = Annotation::Inset;
            else if ( styleName == "U" )
                annotation->style.style = Annotation::Underline;
            // -> style.marks and style.spaces
            Object dashArray;
            bsObj.getDict()->lookup( "D", &dashArray );
            if ( dashArray.isArray() )
            {
                int dashMarks = 3;
                int dashSpaces = 0;
                Object intObj;
                dashArray.arrayGet( 0, &intObj );
                if ( intObj.isInt() )
                    dashMarks = intObj.getInt();
                intObj.free();
                dashArray.arrayGet( 1, &intObj );
                if ( intObj.isInt() )
                    dashSpaces = intObj.getInt();
                intObj.free();
                annotation->style.marks = dashMarks;
                annotation->style.spaces = dashSpaces;
            }
            dashArray.free();
        }
        bsObj.free();
        Object beObj;
        annotDict->lookup( "BE", &beObj );
        if ( beObj.isDict() )
        {
            // -> style.effect
            QString effectName;
            XPDFReader::lookupName( beObj.getDict(), "S", effectName );
            if ( effectName == "C" )
                annotation->style.effect = Annotation::Cloudy;
            // -> style.effectIntensity
            int intensityInt = -1;
            XPDFReader::lookupInt( beObj.getDict(), "I", intensityInt );
            if ( intensityInt != -1 )
                annotation->style.effectIntensity = (double)intensityInt;
        }
        beObj.free();
        // -> style.color
        XPDFReader::lookupColor( annotDict, "C", annotation->style.color );

        /** 1.4. PARSE markup { common, Popup, Revision } parameters */
        if ( parseMarkup )
        {
            // -> creationDate
            XPDFReader::lookupDate( annotDict, "CreationDate", annotation->creationDate );
            // -> style.opacity
            XPDFReader::lookupNum( annotDict, "CA", annotation->style.opacity );
            // -> window.title and author
            XPDFReader::lookupString( annotDict, "T", annotation->window.title );
            annotation->author = annotation->window.title;
            // -> window.summary
            XPDFReader::lookupString( annotDict, "Subj", annotation->window.summary );
            // -> window.text
            XPDFReader::lookupString( annotDict, "RC", annotation->window.text );

            // if a popup is referenced, schedule for resolving it later
            int popupID = 0;
            XPDFReader::lookupIntRef( annotDict, "Popup", popupID );
            if ( popupID )
            {
                ResolveWindow request;
                request.popupWindowID = popupID;
                request.annotation = annotation;
                resolvePopList.append( request );
            }

            // if an older version is referenced, schedule for reparenting
            int parentID = 0;
            XPDFReader::lookupIntRef( annotDict, "IRT", parentID );
            if ( parentID )
            {
                ResolveRevision request;
                request.nextAnnotation = annotation;
                request.nextAnnotationID = annotID;
                request.prevAnnotationID = parentID;

                // -> request.nextScope
                request.nextScope = Annotation::Reply;
                Object revObj;
                annotDict->lookup( "RT", &revObj );
                if ( revObj.isName() )
                {
                    const char * name = revObj.getName();
                    if ( !strcmp( name, "R" ) )
                        request.nextScope = Annotation::Reply;
                    else if ( !strcmp( name, "Group" ) )
                        request.nextScope = Annotation::Group;
                }
                revObj.free();

                // -> revision.type (StateModel is deduced from type, not parsed)
                request.nextType = Annotation::None;
                annotDict->lookup( "State", &revObj );
                if ( revObj.isString() )
                {
                    const char * name = revObj.getString()->getCString();
                    if ( !strcmp( name, "Marked" ) )
                        request.nextType = Annotation::Marked;
                    else if ( !strcmp( name, "Unmarked" ) )
                        request.nextType = Annotation::Unmarked;
                    else if ( !strcmp( name, "Accepted" ) )
                        request.nextType = Annotation::Accepted;
                    else if ( !strcmp( name, "Rejected" ) )
                        request.nextType = Annotation::Rejected;
                    else if ( !strcmp( name, "Cancelled" ) )
                        request.nextType = Annotation::Cancelled;
                    else if ( !strcmp( name, "Completed" ) )
                        request.nextType = Annotation::Completed;
                    else if ( !strcmp( name, "None" ) )
                        request.nextType = Annotation::None;
                }
                revObj.free();

                // schedule for later reparenting
                resolveRevList.append( request );
            }
        }
        // free annot object
        annot.free();

        /** 1.5. ADD ANNOTATION to the annotationsMap  */
        if ( addToPage )
        {
            if ( annotationsMap.contains( annotID ) )
                kdDebug() << "PDFGenerator: clash for annotations with ID:" << annotID << endl;
            annotationsMap[ annotID ] = annotation;
        }
    } // end Annotation/PopupWindow parsing loop

    /** 2 - RESOLVE POPUPS (popup.* -> annotation.window) */
    if ( !resolvePopList.isEmpty() && !popupsMap.isEmpty() )
    {
        QValueList< ResolveWindow >::iterator it = resolvePopList.begin(),
                                              end = resolvePopList.end();
        for ( ; it != end; ++it )
        {
            const ResolveWindow & request = *it;
            if ( !popupsMap.contains( request.popupWindowID ) )
                // warn aboud problems in popup resolving logic
                kdDebug() << "PDFGenerator: can't resolve popup "
                          << request.popupWindowID << "." << endl;
            else
            {
                // set annotation's window properties taking ones from popup
                PopupWindow * pop = popupsMap[ request.popupWindowID ];
                Annotation * pa = pop->dummyAnnotation;
                Annotation::Window & w = request.annotation->window;

                // transfer properties to Annotation's window
                w.flags = pa->flags & (Annotation::Hidden |
                    Annotation::FixedSize | Annotation::FixedRotation);
                if ( !pop->shown )
                    w.flags |= Annotation::Hidden;
                w.topLeft.x = pa->boundary.left;
                w.topLeft.y = pa->boundary.top;
                w.width = (int)(page->width() * (pa->boundary.right - pa->boundary.left));
                w.height = (int)(page->height() * (pa->boundary.bottom - pa->boundary.top));
            }
        }

        // clear data
        QMap< int, PopupWindow * >::Iterator dIt = popupsMap.begin(), dEnd = popupsMap.end();
        for ( ; dIt != dEnd; ++dIt )
        {
            PopupWindow * p = dIt.data();
            delete p->dummyAnnotation;
            delete p;
        }
    }

    /** 3 - RESOLVE REVISIONS (parent.revisions.append( children )) */
    if ( !resolveRevList.isEmpty() )
    {
        // append children to parents
        int excludeIDs[ resolveRevList.count() ];   // can't even reach this size
        int excludeIndex = 0;                       // index in excludeIDs array
        QValueList< ResolveRevision >::iterator it = resolveRevList.begin(), end = resolveRevList.end();
        for ( ; it != end; ++it )
        {
            const ResolveRevision & request = *it;
            int parentID = request.prevAnnotationID;
            if ( !annotationsMap.contains( parentID ) )
                // warn about problems in reparenting logic
                kdDebug() << "PDFGenerator: can't reparent annotation to "
                          << parentID << "." << endl;
            else
            {
                // compile and add a Revision structure to the parent annotation
                Annotation::Revision childRevision;
                childRevision.annotation = request.nextAnnotation;
                childRevision.scope = request.nextScope;
                childRevision.type = request.nextType;
                annotationsMap[ parentID ]->revisions.append( childRevision );
                // exclude child annotation from being rooted in page
                excludeIDs[ excludeIndex++ ] = request.nextAnnotationID;
            }
        }

        // prevent children from being attached to page as roots
        for ( int i = 0; i < excludeIndex; i++ )
            annotationsMap.remove( excludeIDs[ i ] );
    }

    /** 4 - POSTPROCESS TextAnnotations (when window geom is embedded) */
    if ( !ppTextList.isEmpty() )
    {
        QValueList< PostProcessText >::const_iterator it = ppTextList.begin(), end = ppTextList.end();
        for ( ; it != end; ++it )
        {
            const PostProcessText & request = *it;
            Annotation::Window & window = request.textAnnotation->window;
            // if not present, 'create' the window in-place over the annotation
            if ( window.flags == -1 )
            {
                window.flags = 0;
                NormalizedRect & geom = request.textAnnotation->boundary;
                // initialize window geometry to annotation's one
                int width = (int)( page->width() * ( geom.right - geom.left ) ),
                    height = (int)( page->height() * ( geom.bottom - geom.top ) );
                window.topLeft.x = geom.left > 0.0 ? geom.left : 0.0;
                window.topLeft.y = geom.top > 0.0 ? geom.top : 0.0;
                window.width = width < 200 ? 200 : width;
                window.height = height < 120 ? 120 : width;
                // resize annotation's geometry to an icon
                geom.right = geom.left + 22.0 / page->width();
                geom.bottom = geom.top + 22.0 / page->height();
            }
            // (pdf) if text is not 'opened', force window hiding. if the window
            // was parsed from popup, the flag should already be set
            if ( !request.opened && window.flags != -1 )
                window.flags |= Annotation::Hidden;
        }
    }

    /** 5 - finally SET ANNOTATIONS to the page */
    QMap< int, Annotation * >::Iterator it = annotationsMap.begin(), end = annotationsMap.end();
    for ( ; it != end; ++it )
        page->addAnnotation( it.data() );
}

void PDFGenerator::addTransition( Page * pdfPage, KPDFPage * page )
// called on opening when MUTEX is not used
{
    PageTransition *pdfTransition = pdfPage->getTransition();
    if ( !pdfTransition || pdfTransition->getType() == PageTransition::Replace )
        return;

    KPDFPageTransition *transition = new KPDFPageTransition();
    switch ( pdfTransition->getType() ) {
        case PageTransition::Replace:
            // won't get here, added to avoid warning
            break;
        case PageTransition::Split:
            transition->setType( KPDFPageTransition::Split );
            break;
        case PageTransition::Blinds:
            transition->setType( KPDFPageTransition::Blinds );
            break;
        case PageTransition::Box:
            transition->setType( KPDFPageTransition::Box );
            break;
        case PageTransition::Wipe:
            transition->setType( KPDFPageTransition::Wipe );
            break;
        case PageTransition::Dissolve:
            transition->setType( KPDFPageTransition::Dissolve );
            break;
        case PageTransition::Glitter:
            transition->setType( KPDFPageTransition::Glitter );
            break;
        case PageTransition::Fly:
            transition->setType( KPDFPageTransition::Fly );
            break;
        case PageTransition::Push:
            transition->setType( KPDFPageTransition::Push );
            break;
        case PageTransition::Cover:
            transition->setType( KPDFPageTransition::Cover );
            break;
        case PageTransition::Uncover:
            transition->setType( KPDFPageTransition::Uncover );
            break;
        case PageTransition::Fade:
            transition->setType( KPDFPageTransition::Fade );
            break;
    }

    transition->setDuration( pdfTransition->getDuration() );

    switch ( pdfTransition->getAlignment() ) {
        case PageTransition::Horizontal:
            transition->setAlignment( KPDFPageTransition::Horizontal );
            break;
        case PageTransition::Vertical:
            transition->setAlignment( KPDFPageTransition::Vertical );
            break;
    }

    switch ( pdfTransition->getDirection() ) {
        case PageTransition::Inward:
            transition->setDirection( KPDFPageTransition::Inward );
            break;
        case PageTransition::Outward:
            transition->setDirection( KPDFPageTransition::Outward );
            break;
    }

    transition->setAngle( pdfTransition->getAngle() );
    transition->setScale( pdfTransition->getScale() );
    transition->setIsRectangular( pdfTransition->isRectangular() == gTrue );

    page->setTransition( transition );
}



void PDFGenerator::customEvent( QCustomEvent * event )
{
    // catch generator 'ready events' only
    if ( event->type() != TGE_DATAREADY_ID )
        return;

#if 0
    // check if thread is running (has to be stopped now)
    if ( generatorThread->running() )
    {
        // if so, wait for effective thread termination
        if ( !generatorThread->wait( 9999 /*10s timeout*/ ) )
        {
            kdWarning() << "PDFGenerator: thread sent 'data available' "
                        << "signal but had problems ending." << endl;
            return;
        }
}
#endif

    // 1. the mutex must be unlocked now
    if ( docLock.locked() )
    {
        kdWarning() << "PDFGenerator: 'data available' but mutex still "
                    << "held. Recovering." << endl;
        // syncronize GUI thread (must not happen)
        docLock.lock();
        docLock.unlock();
    }

    // 2. put thread's generated data into the KPDFPage
    PixmapRequest * request = static_cast< PixmapRequest * >( event->data() );
    QImage * outImage = generatorThread->takeImage();
    TextPage * outTextPage = generatorThread->takeTextPage();
    QValueList< ObjectRect * > outRects = generatorThread->takeObjectRects();

    request->page->setPixmap( request->id, new QPixmap( *outImage ) );
    delete outImage;
    if ( outTextPage )
        request->page->setSearchPage( abstractTextPage( outTextPage , 
            request->page->height(), request->page->width()));
    if ( !outRects.isEmpty() )
        request->page->setObjectRects( outRects );

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
    TextPage * m_textPage;
    QValueList< ObjectRect * > m_rects;
    bool m_rectsTaken;
};

PDFPixmapGeneratorThread::PDFPixmapGeneratorThread( PDFGenerator * gen )
    : d( new PPGThreadPrivate() )
{
    d->generator = gen;
    d->currentRequest = 0;
    d->m_image = 0;
    d->m_textPage = 0;
    d->m_rectsTaken = true;
}

PDFPixmapGeneratorThread::~PDFPixmapGeneratorThread()
{
    // delete internal objects if the class is deleted before the gui thread
    // takes the data
    delete d->m_image;
    delete d->m_textPage;
    if ( !d->m_rectsTaken && d->m_rects.count() )
    {
        QValueList< ObjectRect * >::iterator it = d->m_rects.begin(), end = d->m_rects.end();
        for ( ; it != end; ++it )
            delete *it;
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
        kdDebug() << "PDFPixmapGeneratorThread: requesting a pixmap "
                  << "when another is being generated." << endl;
        delete request;
        return;
    }

    // check if the mutex is already held
    if ( d->generator->docLock.locked() )
    {
        kdDebug() << "PDFPixmapGeneratorThread: requesting a pixmap "
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
        kdDebug() << "PDFPixmapGeneratorThread: 'end generation' called "
                  << "but generation was not started." << endl;
        return;
    }
#endif
    // reset internal members preparing for a new generation
    d->currentRequest = 0;
}

QImage * PDFPixmapGeneratorThread::takeImage() const
{
    QImage * img = d->m_image;
    d->m_image = 0;
    return img;
}

TextPage * PDFPixmapGeneratorThread::takeTextPage() const
{
    TextPage * tp = d->m_textPage;
    d->m_textPage = 0;
    return tp;
}

QValueList< ObjectRect * > PDFPixmapGeneratorThread::takeObjectRects() const
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
    d->generator->kpdfOutputDev->setParams( width, height, genTextPage,
                                            genObjectRects, genObjectRects, TRUE /*thread safety*/ );
    d->generator->pdfdoc->displayPage( d->generator->kpdfOutputDev, page->number() + 1,
                                       fakeDpiX, fakeDpiY,
                                       d->currentRequest->rotation , true, genObjectRects );

    // 2. grab data from the OutputDev and store it locally (note takeIMAGE)
#ifndef NDEBUG
    if ( d->m_image )
        kdDebug() << "PDFPixmapGeneratorThread: previous image not taken" << endl;
    if ( d->m_textPage )
        kdDebug() << "PDFPixmapGeneratorThread: previous textpage not taken" << endl;
#endif
    d->m_image = d->generator->kpdfOutputDev->takeImage();
    d->m_textPage = d->generator->kpdfOutputDev->takeTextPage();
    d->m_rects = d->generator->kpdfOutputDev->takeObjectRects();
    d->m_rectsTaken = false;

    // 3. [UNLOCK] mutex
    d->generator->docLock.unlock();

    // notify the GUI thread that data is pending and can be read
    QCustomEvent * readyEvent = new QCustomEvent( TGE_DATAREADY_ID );
    readyEvent->setData( d->currentRequest );
    QApplication::postEvent( d->generator, readyEvent );
}
