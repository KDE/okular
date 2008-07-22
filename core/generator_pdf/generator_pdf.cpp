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
#include <qvariant.h>
#include <kapplication.h>
#include <klistview.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kwallet.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kdebug.h>

// xpdf includes
#include "xpdf/Object.h"
#include "xpdf/Dict.h"
#include "xpdf/Annot.h"
#include "xpdf/PSOutputDev.h"
#include "xpdf/TextOutputDev.h"
#include "xpdf/Link.h"
#include "xpdf/ErrorCodes.h"
#include "xpdf/UnicodeMap.h"
#include "xpdf/Outline.h"
#include "goo/GList.h"

// local includes
#include "generator_pdf.h"
#include "gp_outputdev.h"
#include "core/observer.h" //for PAGEVIEW_ID
#include "core/page.h"
#include "core/pagetransition.h"
#include "conf/settings.h"

#include <stdlib.h>

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

PDFGenerator::PDFGenerator( KPDFDocument * doc )
    : Generator( doc ), pdfdoc( 0 ), kpdfOutputDev( 0 ), ready( true ),
    pixmapRequest( 0 ), docInfoDirty( true ), docSynopsisDirty( true )
{
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
    int keep = 1;
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
            if ( KPasswordDialog::getPassword( password, prompt, wallet ? &keep : 0 ) != KPasswordDialog::Accepted )
            break;
        }

        // 2. reopen the document using the password
        GString * pwd2 = new GString( QString::fromLocal8Bit(password.data()).latin1() );
            delete pdfdoc;
        pdfdoc = new PDFDoc( new GString( QFile::encodeName( filePath ) ), pwd2, pwd2 );
            delete pwd2;

        // 3. if the password is correct and the user chose to remember it, store it to the wallet
        if ( pdfdoc->isOk() && wallet && /*safety check*/ wallet->isOpen() && keep > 0 )
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
    for ( uint i = 0; i < pageCount ; i++ )
    {
        KPDFPage * page = new KPDFPage( i, pdfdoc->getPageCropWidth(i+1),
                                        pdfdoc->getPageCropHeight(i+1),
                                        pdfdoc->getPageRotate(i+1) );
        addTransition( i, page );
        pagesVector[i] = page;
    }

    // the file has been loaded correctly
    return true;
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
            docInfo.set( "pages", QString::number( pdfdoc->getCatalog()->getNumPages() ), i18n("Pages") );
        }
        else
        {
            docInfo.set( "format", "PDF", i18n( "Format" ) );
            docInfo.set( "encryption", i18n( "Unknown Encryption" ), i18n( "Security" ) );
            docInfo.set( "optimization", i18n( "Unknown Optimization" ), i18n( "Optimized" ) );
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

bool PDFGenerator::isAllowed( int permissions )
{
#if !KPDF_FORCE_DRM
    if (kapp->authorize("skip_drm") && !KpdfSettings::obeyDRM()) return true;
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
    kpdfOutputDev->setParams( request->width, request->height, genObjectRects, genObjectRects, false );
    pdfdoc->displayPage( kpdfOutputDev, page->number() + 1, fakeDpiX, fakeDpiY, 0, false, true, false );
    if ( genObjectRects )
        pdfdoc->processLinks( kpdfOutputDev, page->number() + 1 );

    // 2. Take data from outputdev and attach it to the Page
    page->setPixmap( request->id, kpdfOutputDev->takePixmap() );
    if ( genObjectRects )
        page->setObjectRects( kpdfOutputDev->takeObjectRects() );

    // 3. UNLOCK [re-enables shared access]
    docLock.unlock();

    if ( genTextPage )
        generateSyncTextPage( page );

    // update ready state
    ready = true;

    // notify the new generation
    signalRequestDone( request );
}

void PDFGenerator::generateSyncTextPage( KPDFPage * page )
{
    // build a TextPage...
    TextOutputDev td(NULL, gTrue, gFalse, gFalse);
    docLock.lock();
    pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, false, true, false );
    // ..and attach it to the page
    page->setSearchPage( td.takeText() );
    docLock.unlock();
}

bool PDFGenerator::supportsSearching() const
{
    return true;
}

bool PDFGenerator::hasFonts() const
{
    return true;
}

void PDFGenerator::putFontInfo(KListView *list)
{
    Page *page;
    Dict *resDict;
    Annots *annots;
    Object obj1, obj2;
    int pg, i;

    Ref *fonts;
    int fontsLen;
    int fontsSize;

    list->addColumn(i18n("Name"));
    list->addColumn(i18n("Type"));
    list->addColumn(i18n("Embedded"));
    list->addColumn(i18n("File"));

    docLock.lock();

    fonts = NULL;
    fontsLen = fontsSize = 0;
    QValueVector<Ref> visitedXObjects;
    for (pg = 1; pg <= pdfdoc->getNumPages(); ++pg)
    {
        page = pdfdoc->getCatalog()->getPage(pg);
        if ((resDict = page->getResourceDict()))
        {
            scanFonts(resDict, list, &fonts, fontsLen, fontsSize, &visitedXObjects);
        }
        annots = new Annots(pdfdoc->getXRef(), pdfdoc->getCatalog(), page->getAnnots(&obj1));
        obj1.free();
        for (i = 0; i < annots->getNumAnnots(); ++i)
        {
            if (annots->getAnnot(i)->getAppearance(&obj1)->isStream())
            {
                obj1.streamGetDict()->lookup("Resources", &obj2);
                if (obj2.isDict())
                {
                    scanFonts(obj2.getDict(), list, &fonts, fontsLen, fontsSize, &visitedXObjects);
                }
                obj2.free();
            }
            obj1.free();
        }
        delete annots;
    }
    gfree(fonts);

    docLock.unlock();
}

bool PDFGenerator::print( KPrinter& printer )
{
    // PageSize is a CUPS artificially created setting
    QString ps = printer.option("PageSize");
    int paperWidth, paperHeight;
    int marginTop, marginLeft, marginRight, marginBottom;
    marginTop = (int)printer.option("kde-margin-top").toDouble();
    marginLeft = (int)printer.option("kde-margin-left").toDouble();
    marginRight = (int)printer.option("kde-margin-right").toDouble();
    marginBottom = (int)printer.option("kde-margin-bottom").toDouble();
    bool forceRasterize = printer.option("kde-kpdf-forceRaster").toInt();

    if (ps.find(QRegExp("w\\d+h\\d+")) == 0)
    {
        // size not supported by Qt, CUPS gives us the size as wWIDTHhHEIGHT, at least on the printers i tester
        // remove the w
        ps = ps.mid(1);
        int hPos = ps.find("h");
        paperWidth = ps.left(hPos).toInt();
        paperHeight = ps.mid(hPos+1).toInt();
    }
    else
    {
        // size is supported by Qt, we get either the pageSize name or nothing because the CUPS driver
        // does not do any translation, then use KPrinter::pageSize to get the page size
        KPrinter::PageSize qtPageSize;
        if (!ps.isEmpty()) qtPageSize = pageNameToPageSize(ps);
        else qtPageSize = printer.pageSize();

        QPrinter dummy(QPrinter::PrinterResolution);
        dummy.setFullPage(true);
        dummy.setPageSize((QPrinter::PageSize)qtPageSize);

        QPaintDeviceMetrics metrics(&dummy);
        paperWidth = metrics.width();
        paperHeight = metrics.height();
    }

    KTempFile tf( QString::null, ".ps" );
    globalParams->setPSPaperWidth(paperWidth);
    globalParams->setPSPaperHeight(paperHeight);
    QString pstitle = getDocumentInfo("Title", true);
    if ( pstitle.isEmpty() )
    {
        pstitle = m_document->currentDocument().fileName( false );
    }
    // this looks non-unicode-safe and it is. anything other than ASCII is not specified
    // and some printers actually stop printing when they encounter non-ASCII characters in the
    // Postscript %%Title tag
    QCString pstitle8Bit = pstitle.latin1();
    const char* pstitlechar;
    if (!pstitle.isEmpty())
    {
      pstitlechar = pstitle8Bit.data();
      for (unsigned char* p = (unsigned char*) pstitle8Bit.data(); *p; ++p)
          if (*p >= 0x80)
              *p = '?';

      printer.setDocName(pstitle);
    }
    else
    {
      pstitlechar = 0;
    }
    PSOutputDev *psOut = new PSOutputDev(const_cast<char*>(tf.name().latin1()), const_cast<char*>(pstitlechar), pdfdoc->getXRef(), pdfdoc->getCatalog(), 1, pdfdoc->getNumPages(), psModePS, marginLeft, marginBottom, paperWidth - marginRight, paperHeight - marginTop, forceRasterize);

    if (psOut->isOk())
    {
        double xScale = ((double)paperWidth - (double)marginLeft - (double)marginRight) / (double)paperWidth;
        double yScale = ((double)paperHeight - (double)marginBottom - (double)marginTop) / (double)paperHeight;

        if ( abs((int)(xScale * 100) - (int)(yScale * 100)) > 5 ) {
            int result = KMessageBox::questionYesNo(m_document->widget(),
                                       i18n("The margins you specified change the page aspect ratio. Do you want to print with the aspect ratio changed or do you want the margins to be adapted so that the aspect ratio is preserved?"),
                                       i18n("Aspect ratio change"),
                                       i18n("Print with specified margins"),
                                       i18n("Print adapting margins to keep aspect ratio"),
                                       "kpdfStrictlyObeyMargins");
            if (result == KMessageBox::Yes) psOut->setScale(xScale, yScale);
        }

        QValueList<int> pageList;

        if (!printer.previewOnly())
        {
            pageList = printer.pageList();
        }
        else
        {
            for(int i = 1; i <= pdfdoc->getNumPages(); i++) pageList.append(i);
        }

        QValueList<int>::const_iterator pIt = pageList.begin(), pEnd = pageList.end();
        docLock.lock();
        for ( ; pIt != pEnd; ++pIt )
        {
            pdfdoc->displayPage(psOut, *pIt, 72, 72, 0, false, globalParams->getPSCrop(), gTrue);
        }
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

static GString *QStringToGString(const QString &s) {
    int len = s.length();
    char *cstring = (char *)gmallocn(s.length(), sizeof(char));
    for (int i = 0; i < len; ++i)
      cstring[i] = s.at(i).unicode();
    return new GString(cstring, len);
}

static QString unicodeToQString(Unicode* u, int len) {
    QString ret;
    ret.setLength(len);
    QChar* qch = (QChar*) ret.unicode();
    for (;len;--len)
      *qch++ = (QChar) *u++;
    return ret;
}

static QString UnicodeParsedString(GString *s1) {
    GBool isUnicode;
    int i;
    Unicode u;
    QString result;
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
    while ( i < s1->getLength() )
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
    return result;
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
        GString * namedDest = QStringToGString(option);
        docLock.lock();
        LinkDest * destination = pdfdoc->findDest( namedDest );
        if ( destination )
        {
            fillViewportFromLink( viewport, destination );
        }
        docLock.unlock();
        delete namedDest;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    else if ( key == "OpenTOC" )
    {
        if ( pdfdoc->getCatalog()->getPageMode() == Catalog::UseOutlines )
            return "yes";
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
    if ( color != paperColor || !kpdfOutputDev )
    {
        paperColor = color;
        SplashColor splashCol;
        splashCol[0] = paperColor.red();
	splashCol[1] = paperColor.green();
	splashCol[2] = paperColor.blue();
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

void PDFGenerator::scanFonts(Dict *resDict, KListView *list, Ref **fonts, int &fontsLen, int &fontsSize, QValueVector<Ref> *visitedXObjects)
{
    Object obj1, obj2, xObjDict, xObj, xObj2, resObj;
    Ref r;
    GfxFontDict *gfxFontDict;
    GfxFont *font;
    int i;

    // scan the fonts in this resource dictionary
    gfxFontDict = NULL;
    resDict->lookupNF("Font", &obj1);
    if (obj1.isRef())
    {
        obj1.fetch(pdfdoc->getXRef(), &obj2);
        if (obj2.isDict())
        {
            r = obj1.getRef();
            gfxFontDict = new GfxFontDict(pdfdoc->getXRef(), &r, obj2.getDict());
        }
        obj2.free();
    }
    else if (obj1.isDict())
    {
        gfxFontDict = new GfxFontDict(pdfdoc->getXRef(), NULL, obj1.getDict());
    }
    if (gfxFontDict)
    {
        for (i = 0; i < gfxFontDict->getNumFonts(); ++i)
        {
            if ((font = gfxFontDict->getFont(i))) scanFont(font, list, fonts, fontsLen, fontsSize);
        }
        delete gfxFontDict;
    }
    obj1.free();

    // recursively scan any resource dictionaries in objects in this
    // resource dictionary
    resDict->lookup("XObject", &xObjDict);
    if (xObjDict.isDict()) {
        for (i = 0; i < xObjDict.dictGetLength(); ++i) {
            xObjDict.dictGetValNF(i, &xObj);
            if (xObj.isRef()) {
                bool alreadySeen = false;
                // check for an already-seen XObject
                for (int k = 0; k < visitedXObjects->count(); ++k) {
                    if (xObj.getRef().num == visitedXObjects->at(k).num &&
                        xObj.getRef().gen == visitedXObjects->at(k).gen) {
                            alreadySeen = true;
                    }
                }

                if (alreadySeen) {
                    xObj.free();
                    continue;
                }

                visitedXObjects->append(xObj.getRef());
            }

            xObj.fetch(pdfdoc->getXRef(), &xObj2);

            if (xObj2.isStream()) {
                xObj2.streamGetDict()->lookup("Resources", &resObj);
                if (resObj.isDict() && resObj.getDict() != resDict) {
                    scanFonts(resObj.getDict(), list, fonts, fontsLen, fontsSize, visitedXObjects);
                }
                resObj.free();
            }
            xObj.free();
            xObj2.free();
        }
    }
    xObjDict.free();
}

void PDFGenerator::scanFont(GfxFont *font, KListView *list, Ref **fonts, int &fontsLen, int &fontsSize)
{
    Ref fontRef, embRef;
    Object fontObj, toUnicodeObj;
    GString *name;
    GBool emb;
    int i;

    QString fontTypeNames[12] = {
        i18n("unknown"),
        i18n("Type 1"),
        i18n("Type 1C"),
        i18n("OT means OpenType", "Type 1C (OT)"),
        i18n("Type 3"),
        i18n("TrueType"),
        i18n("OT means OpenType", "TrueType (OT)"),
        i18n("CID Type 0"),
        i18n("CID Type 0C"),
        i18n("OT means OpenType", "CID Type 0C (OT)"),
        i18n("CID TrueType"),
        i18n("OT means OpenType", "CID TrueType (OT)")
    };

    fontRef = *font->getID();

    // check for an already-seen font
    for (i = 0; i < fontsLen; ++i)
    {
        if (fontRef.num == (*fonts)[i].num && fontRef.gen == (*fonts)[i].gen)
        {
            return;
        }
    }

    // font name
    name = font->getOrigName();

    // check for an embedded font
    if (font->getType() == fontType3) emb = gTrue;
    else emb = font->getEmbeddedFontID(&embRef);

    QString sName, sEmb, sPath;
    if (name)
    {
        sName = name->getCString();
        if (!emb)
        {
            DisplayFontParam *dfp = globalParams->getDisplayFont(name);
            if (dfp)
            {
                if (dfp -> kind == displayFontT1) sPath = dfp->t1.fileName->getCString();
                else sPath = dfp->tt.fileName->getCString();
            }
            else sPath = i18n("-");
        }
        else sPath = i18n("-");
    }
    else
    {
        sName = i18n("[none]");
        sPath = i18n("-");
    }
    sEmb = emb ? i18n("Yes") : i18n("No");
    new KListViewItem(list, sName, fontTypeNames[font->getType()], sEmb, sPath);

    // add this font to the list
    if (fontsLen == fontsSize)
    {
        fontsSize += 32;
        *fonts = (Ref *)grealloc(*fonts, fontsSize * sizeof(Ref));
    }
    (*fonts)[fontsLen++] = *font->getID();
}

QString PDFGenerator::getDocumentInfo( const QString & data, bool canReturnNull ) const
// note: MUTEX is LOCKED while calling this
{
    // [Albert] Code adapted from pdfinfo.cc on xpdf
    Object info;
    if ( !pdfdoc )
        return canReturnNull ? QString::null : i18n( "Unknown" );

    pdfdoc->getDocInfo( &info );
    if ( !info.isDict() )
        return canReturnNull ? QString::null : i18n( "Unknown" );

    Object obj;
    Dict *infoDict = info.getDict();

    if ( infoDict->lookup( (char*)data.latin1(), &obj )->isString() )
    {
        QString result = UnicodeParsedString(obj.getString());
        obj.free();
        info.free();
        return result;
    }
    obj.free();
    info.free();
    return canReturnNull ? QString::null : i18n( "Unknown" );
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
    int year, mon, day, hour, min, sec;
    Dict *infoDict = info.getDict();
    UnicodeMap *uMap = globalParams->getTextEncoding();
    QString result;

    if ( !uMap )
        return i18n( "Unknown Date" );

    if ( infoDict->lookup( (char*)data.latin1(), &obj )->isString() )
    {
        QString s = UnicodeParsedString(obj.getString());
        if ( s[0] == 'D' && s[1] == ':' )
            s = s.mid(2);

        if ( !s.isEmpty() && sscanf( s.latin1(), "%4d%2d%2d%2d%2d%2d", &year, &mon, &day, &hour, &min, &sec ) == 6 )
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
        if ( a && ( a->getKind() == actionGoTo || a->getKind() == actionGoToR ) )
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
                GString *s = g->getNamedDest();
                QChar *charArray = new QChar[s->getLength()];
                for (int i = 0; i < s->getLength(); ++i) charArray[i] = QChar(s->getCString()[i]);
                QString option(charArray, s->getLength());
                item.setAttribute( "ViewportName", option );
                delete[] charArray;
            }
            else if ( destination && destination->isOk() )
            {
                DocumentViewport vp;
                fillViewportFromLink( vp, destination );
                item.setAttribute( "Viewport", vp.toString() );
            }
            if ( a->getKind() == actionGoToR )
            {
                LinkGoToR * g2 = static_cast< LinkGoToR * >( a );
                item.setAttribute( "ExternalFileName", g2->getFileName()->getCString() );
            }
        }

        item.setAttribute( "Open", QVariant( (bool)outlineItem->isOpen() ).toString() );

        // 3. recursively descend over children
        outlineItem->open();
        GList * children = outlineItem->getKids();
        if ( children )
            addSynopsisChildren( &item, children );
    }
}

void PDFGenerator::fillViewportFromLink( DocumentViewport &viewport, LinkDest *destination )
{
    if ( !destination->isPageRef() )
        viewport.pageNumber = destination->getPageNum() - 1;
    else
    {
        Ref ref = destination->getPageRef();
        viewport.pageNumber = pdfdoc->findPage( ref.num, ref.gen ) - 1;
    }

    if (viewport.pageNumber < 0) return;
    if (viewport.pageNumber >= pdfdoc->getNumPages()) return;

    // get destination position
    // TODO add other attributes to the viewport (taken from link)
//     switch ( destination->getKind() )
//     {
//         case destXYZ:
            if (destination->getChangeLeft() || destination->getChangeTop())
            {
                double CTM[6];
                Page *page = pdfdoc->getCatalog()->getPage( viewport.pageNumber + 1 );
                // TODO remember to change this if we implement DPI and/or rotation
                page->getDefaultCTM(CTM, 72.0, 72.0, 0, gFalse, gTrue);

                int left, top;
                // this is OutputDev::cvtUserToDev
                left = (int)(CTM[0] * destination->getLeft() + CTM[2] * destination->getTop() + CTM[4] + 0.5);
                top = (int)(CTM[1] * destination->getLeft() + CTM[3] * destination->getTop() + CTM[5] + 0.5);

                viewport.rePos.normalizedX = (double)left / (double)page->getCropWidth();
                viewport.rePos.normalizedY = (double)top / (double)page->getCropHeight();
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

void PDFGenerator::addTransition( int pageNumber, KPDFPage * page )
{
    Page *pdfPage = pdfdoc->getCatalog()->getPage( pageNumber + 1 );
    if ( !pdfPage )
        return;

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
        request->page->setSearchPage( outTextPage );
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
    d->generator->kpdfOutputDev->setParams( width, height, 
                                            genObjectRects, genObjectRects, TRUE /*thread safety*/ );
    d->generator->pdfdoc->displayPage( d->generator->kpdfOutputDev, page->number() + 1,
                                       fakeDpiX, fakeDpiY, 0, false, true, false );
    if ( genObjectRects )
        d->generator->pdfdoc->processLinks( d->generator->kpdfOutputDev, page->number() + 1 );

    // 2. grab data from the OutputDev and store it locally (note takeIMAGE)
#ifndef NDEBUG
    if ( d->m_image )
        kdDebug() << "PDFPixmapGeneratorThread: previous image not taken" << endl;
    if ( d->m_textPage )
        kdDebug() << "PDFPixmapGeneratorThread: previous textpage not taken" << endl;
#endif
    d->m_image = d->generator->kpdfOutputDev->takeImage();
    d->m_rects = d->generator->kpdfOutputDev->takeObjectRects();
    d->m_rectsTaken = false;

    if ( genTextPage )
    {
        TextOutputDev td(NULL, gTrue, gFalse, gFalse);
        d->generator->pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, false, true, false );
        // ..and attach it to the page
        d->m_textPage = td.takeText();
    }
    
    // 3. [UNLOCK] mutex
    d->generator->docLock.unlock();

    // notify the GUI thread that data is pending and can be read
    QCustomEvent * readyEvent = new QCustomEvent( TGE_DATAREADY_ID );
    readyEvent->setData( d->currentRequest );
    QApplication::postEvent( d->generator, readyEvent );
}
