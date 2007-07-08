/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_pdf.h"

// qt/kde includes
#include <qcheckbox.h>
#include <qcolor.h>
#include <qfile.h>
#include <qimage.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qtextstream.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpassworddialog.h>
#include <kwallet.h>
#include <kprinter.h>
#include <ktemporaryfile.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kdeprint/kprintdialogpage.h>

#include <okular/core/action.h>
#include <okular/core/page.h>
#include <okular/core/annotations.h>
#include <okular/core/pagetransition.h>
#include <okular/core/sound.h>
#include <okular/core/sourcereference.h>
#include <okular/core/textpage.h>

// local includes
#include "settings.h"

#include <config-okular.h>

#ifdef HAVE_POPPLER_0_6
#include "formfields.h"
#endif

static const int PDFDebug = 4653;

class PDFEmbeddedFile : public Okular::EmbeddedFile
{
    public:
        PDFEmbeddedFile(Poppler::EmbeddedFile *f) : ef(f)
        {
        }
        
        QString name() const
        {
            return ef->name();
        }
        
        QString description() const
        {
            return ef->description();
        }
        
        QByteArray data() const
        {
            return ef->data();
        }
        
        int size() const
        {
#ifndef HAVE_POPPLER_0_6
            return -1;
#else
            int s = ef->size();
            return s <= 0 ? -1 : s;
#endif
        }
        
        QDateTime modificationDate() const
        {
            return ef->modDate();
        }
        
        QDateTime creationDate() const
        {
            return ef->createDate();
        }
    
    private:
        Poppler::EmbeddedFile *ef;
};

class PDFOptionsPage : public KPrintDialogPage
{
   public:
       PDFOptionsPage()
       {
           setTitle( i18n( "PDF Options" ) );
           QVBoxLayout *layout = new QVBoxLayout(this);
           m_forceRaster = new QCheckBox(i18n("Force rasterization"), this);
           m_forceRaster->setToolTip(i18n("Rasterize into an image before printing"));
           m_forceRaster->setWhatsThis(i18n("Forces the rasterization of each page into an image before printing it. This usually gives somewhat worse results, but is useful when printing documents that appear to print incorrectly."));
           layout->addWidget(m_forceRaster);
           layout->addStretch(1);
       }

       void getOptions( QMap<QString,QString>& opts, bool incldef = false )
       {
           Q_UNUSED(incldef);
           opts[ "kde-okular-poppler-forceRaster" ] = QString::number( m_forceRaster->isChecked() );
       }

       void setOptions( const QMap<QString,QString>& opts )
       {
           m_forceRaster->setChecked( opts[ "kde-okular-poppler-forceRaster" ].toInt() );
       }

    private:
        QCheckBox *m_forceRaster;
};


static void fillViewportFromLinkDestination( Okular::DocumentViewport &viewport, const Poppler::LinkDestination &destination, const Poppler::Document *pdfdoc )
{
#ifdef HAVE_POPPLER_0_6
    Q_UNUSED( pdfdoc )
#endif
    viewport.pageNumber = destination.pageNumber() - 1;

    if (!viewport.isValid()) return;

    // get destination position
    // TODO add other attributes to the viewport (taken from link)
//     switch ( destination->getKind() )
//     {
//         case destXYZ:
            if (destination.isChangeLeft() || destination.isChangeTop())
            {
                // TODO remember to change this if we implement DPI and/or rotation
                double left, top;
                left = destination.left();
                top = destination.top();

#ifndef HAVE_POPPLER_0_6
                Poppler::Page *page = pdfdoc->page( viewport.pageNumber );
                QSize pageSize = page->pageSize();
                delete page;
                viewport.rePos.normalizedX = (double)left / (double)pageSize.width();
                viewport.rePos.normalizedY = (double)top / (double)pageSize.height();
#else
                viewport.rePos.normalizedX = left;
                viewport.rePos.normalizedY = top;
#endif
                viewport.rePos.enabled = true;
                viewport.rePos.pos = Okular::DocumentViewport::TopLeft;
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

static Okular::Action* createLinkFromPopplerLink(const Poppler::Link *popplerLink, const Poppler::Document *pdfdoc)
{
	Okular::Action *link = 0;
	const Poppler::LinkGoto *popplerLinkGoto;
	const Poppler::LinkExecute *popplerLinkExecute;
	const Poppler::LinkBrowse *popplerLinkBrowse;
	const Poppler::LinkAction *popplerLinkAction;
#ifdef HAVE_POPPLER_0_6
	const Poppler::LinkSound *popplerLinkSound;
#endif
	Okular::DocumentViewport viewport;
	
	switch(popplerLink->linkType())
	{
		case Poppler::Link::None:
		break;
	
		case Poppler::Link::Goto:
			popplerLinkGoto = static_cast<const Poppler::LinkGoto *>(popplerLink);
			fillViewportFromLinkDestination( viewport, popplerLinkGoto->destination(), pdfdoc );
			link = new Okular::GotoAction(popplerLinkGoto->fileName(), viewport);
		break;
		
		case Poppler::Link::Execute:
			popplerLinkExecute = static_cast<const Poppler::LinkExecute *>(popplerLink);
			link = new Okular::ExecuteAction( popplerLinkExecute->fileName(), popplerLinkExecute->parameters() );
		break;
		
		case Poppler::Link::Browse:
			popplerLinkBrowse = static_cast<const Poppler::LinkBrowse *>(popplerLink);
			link = new Okular::BrowseAction( popplerLinkBrowse->url() );
		break;
		
		case Poppler::Link::Action:
			popplerLinkAction = static_cast<const Poppler::LinkAction *>(popplerLink);
			link = new Okular::DocumentAction( (Okular::DocumentAction::DocumentActionType)popplerLinkAction->actionType() );
		break;
		
#ifdef HAVE_POPPLER_0_6
		case Poppler::Link::Sound:
		{
			popplerLinkSound = static_cast<const Poppler::LinkSound *>(popplerLink);
			Poppler::SoundObject *popplerSound = popplerLinkSound->sound();
			Okular::Sound *sound = popplerSound->soundType() == Poppler::SoundObject::Embedded ? new Okular::Sound( popplerSound->data() ) : new Okular::Sound( popplerSound->url() );
			sound->setSamplingRate( popplerSound->samplingRate() );
			sound->setChannels( popplerSound->channels() );
			sound->setBitsPerSample( popplerSound->bitsPerSample() );
			switch ( popplerSound->soundEncoding() )
			{
				case Poppler::SoundObject::Raw:
					sound->setSoundEncoding( Okular::Sound::Raw );
					break;
				case Poppler::SoundObject::Signed:
					sound->setSoundEncoding( Okular::Sound::Signed );
					break;
				case Poppler::SoundObject::muLaw:
					sound->setSoundEncoding( Okular::Sound::muLaw );
					break;
				case Poppler::SoundObject::ALaw:
					sound->setSoundEncoding( Okular::Sound::ALaw );
					break;
			}
			link = new Okular::SoundAction( popplerLinkSound->volume(), popplerLinkSound->synchronous(), popplerLinkSound->repeat(), popplerLinkSound->mix(), sound );
		}
		break;
#endif
		
		case Poppler::Link::Movie:
			// not implemented
		break;
	}
	
	return link;
}

static QLinkedList<Okular::ObjectRect*> generateLinks( const QList<Poppler::Link*> &popplerLinks, int width, int height, const Poppler::Document *pdfdoc )
{
#ifdef HAVE_POPPLER_0_6
	Q_UNUSED( width )
	Q_UNUSED( height )
#endif
	QLinkedList<Okular::ObjectRect*> links;
	foreach(const Poppler::Link *popplerLink, popplerLinks)
	{
		QRectF linkArea = popplerLink->linkArea();
#ifdef HAVE_POPPLER_0_6
		double nl = linkArea.left(),
		       nt = linkArea.top(),
		       nr = linkArea.right(),
		       nb = linkArea.bottom();
#else
		double nl = linkArea.left() / (double)width,
		       nt = linkArea.top() / (double)height,
		       nr = linkArea.right() / (double)width,
		       nb = linkArea.bottom() / (double)height;
#endif
		// create the rect using normalized coords and attach the Okular::Link to it
		Okular::ObjectRect * rect = new Okular::ObjectRect( nl, nt, nr, nb, false, Okular::ObjectRect::Action, createLinkFromPopplerLink(popplerLink, pdfdoc) );
		// add the ObjectRect to the container
		links.push_front( rect );
	}
	qDeleteAll(popplerLinks);
	return links;
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

OKULAR_EXPORT_PLUGIN(PDFGenerator)

PDFGenerator::PDFGenerator()
    : Generator(), pdfdoc( 0 ), ready( true ),
    pixmapRequest( 0 ), docInfoDirty( true ), docSynopsisDirty( true ),
    docEmbeddedFilesDirty( true )
{
    setFeature( TextExtraction );
    setFeature( FontInfo );
#ifdef HAVE_POPPLER_0_6
    setFeature( ReadRawData );
#endif
    // update the configuration
    reparseConfig();
    // generate the pixmapGeneratorThread
    generatorThread = new PDFPixmapGeneratorThread( this );
    connect(generatorThread, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
}

PDFGenerator::~PDFGenerator()
{
    // stop and delete the generator thread
    if ( generatorThread )
    {
        generatorThread->wait();
        delete generatorThread;
    }
}

//BEGIN Generator inherited functions
bool PDFGenerator::loadDocument( const QString & filePath, QVector<Okular::Page*> & pagesVector )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kDebug(PDFDebug) << "PDFGenerator: multiple calls to loadDocument. Check it." << endl;
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::load( filePath, 0, 0 );
    bool success = init(pagesVector, filePath.section('/', -1, -1));
    if (success && QFile::exists(filePath + QLatin1String( "sync" )))
    {
        loadPdfSync(filePath, pagesVector);
    }
    return success;
}

bool PDFGenerator::loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector )
{
#ifdef HAVE_POPPLER_0_6
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kDebug(PDFDebug) << "PDFGenerator: multiple calls to loadDocument. Check it." << endl;
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::loadFromData( fileData, 0, 0 );
    return init(pagesVector, QString());
#else
    Q_UNUSED(fileData)
    Q_UNUSED(pagesVector)
    return false;
#endif
}

bool PDFGenerator::init(QVector<Okular::Page*> & pagesVector, const QString &walletKey)
{
    // if the file didn't open correctly it might be encrypted, so ask for a pass
    bool firstInput = true;
    bool triedWallet = false;
    KWallet::Wallet * wallet = 0;
    bool keep = true;
    while ( pdfdoc && pdfdoc->isLocked() )
    {
        QString password;

        // 1.A. try to retrieve the first password from the kde wallet system
        if ( !triedWallet && !walletKey.isNull() )
        {
            QString walletName = KWallet::Wallet::NetworkWallet();
            WId parentwid = 0;
            if ( document() && document()->widget() )
                parentwid = document()->widget()->winId();
            wallet = KWallet::Wallet::openWallet( walletName, parentwid );
            if ( wallet )
            {
                // use the KPdf folder (and create if missing)
                if ( !wallet->hasFolder( "KPdf" ) )
                    wallet->createFolder( "KPdf" );
                wallet->setFolder( "KPdf" );

                // look for the pass in that folder
                QString retrievedPass;
                if ( !wallet->readPassword( walletKey, retrievedPass ) )
                    password = retrievedPass;
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
            KPasswordDialog dlg( 0, wallet ? KPasswordDialog::ShowKeepPassword : KPasswordDialog::KPasswordDialogFlags() );
            dlg.setCaption( i18n( "Document Password" ) );
            dlg.setPrompt( prompt );
            if( !dlg.exec() )
                break;
            password = dlg.password();
            if ( wallet )
                keep = dlg.keepPassword();
        }

        // 2. reopen the document using the password
        pdfdoc->unlock( password.toLocal8Bit(), password.toLocal8Bit() );

        // 3. if the password is correct and the user chose to remember it, store it to the wallet
        if ( !pdfdoc->isLocked() && wallet && /*safety check*/ wallet->isOpen() && keep )
        {
            wallet->writePassword( walletKey, password );
        }
    }
    if ( !pdfdoc || pdfdoc->isLocked() )
    {
        delete pdfdoc;
        pdfdoc = 0;
        return false;
    }

    // build Pages (currentPage was set -1 by deletePages)
    uint pageCount = pdfdoc->numPages();
    pagesVector.resize(pageCount);
    rectsGenerated.fill(false, pageCount);

    loadPages(pagesVector, 0, false);

#ifdef HAVE_POPPLER_0_6
    pdfdoc->setRenderHint(Poppler::Document::Antialiasing);
    pdfdoc->setRenderHint(Poppler::Document::TextAntialiasing);
#endif

    // the file has been loaded correctly
    return true;
}

bool PDFGenerator::closeDocument()
{
    // remove internal objects
    docLock.lock();
    delete pdfdoc;
    pdfdoc = 0;
    docLock.unlock();
    docInfoDirty = true;
    docSynopsisDirty = true;
    docSyn.clear();
    docEmbeddedFilesDirty = true;
    qDeleteAll(docEmbeddedFiles);
    docEmbeddedFiles.clear();

    return true;
}

void PDFGenerator::loadPages(QVector<Okular::Page*> &pagesVector, int rotation, bool clear)
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
        Okular::Rotation orientation = Okular::Rotation0;
        switch (p->orientation())
        {
          case Poppler::Page::Landscape: orientation = Okular::Rotation90; break;
          case Poppler::Page::UpsideDown: orientation = Okular::Rotation180; break;
          case Poppler::Page::Seascape: orientation = Okular::Rotation270; break;
          case Poppler::Page::Portrait: orientation = Okular::Rotation0; break;
        }
        if (rotation % 2 == 1)
          qSwap(w,h);
        // init a Okular::page, add transition and annotation information
        Okular::Page * page = new Okular::Page( i, w, h, orientation );
        addTransition( p, page );
        if ( true ) //TODO real check
          addAnnotations( p, page );
#ifdef HAVE_POPPLER_0_6
        Poppler::Link * tmplink = p->action( Poppler::Page::Opening );
        if ( tmplink )
        {
            page->setPageAction( Okular::Page::Opening, createLinkFromPopplerLink( tmplink, pdfdoc ) );
            delete tmplink;
        }
        tmplink = p->action( Poppler::Page::Closing );
        if ( tmplink )
        {
            page->setPageAction( Okular::Page::Closing, createLinkFromPopplerLink( tmplink, pdfdoc ) );
            delete tmplink;
        }
        page->setDuration( p->duration() );
        page->setLabel( p->label() );

        addFormFields( p, page );
#endif
//        kWarning(PDFDebug) << page->width() << "x" << page->height() << endl;

#ifdef PDFGENERATOR_DEBUG
        kDebug(PDFDebug) << "load page " << i << " with rotation " << rotation << " and orientation " << orientation << endl;
#endif
	delete p;

        if (clear && pagesVector[i])
            delete pagesVector[i];
        // set the Okular::page at the right position in document's pages vector
        pagesVector[i] = page;
    }
}

const Okular::DocumentInfo * PDFGenerator::generateDocumentInfo()
{
    if ( docInfoDirty )
    {
        docLock.lock();
        
        docInfo.set( Okular::DocumentInfo::MimeType, "application/pdf" );
        
        if ( pdfdoc )
        {
            // compile internal structure reading properties from PDFDoc
            docInfo.set( Okular::DocumentInfo::Title, pdfdoc->info("Title") );
            docInfo.set( Okular::DocumentInfo::Subject, pdfdoc->info("Subject") );
            docInfo.set( Okular::DocumentInfo::Author, pdfdoc->info("Author") );
            docInfo.set( Okular::DocumentInfo::Keywords, pdfdoc->info("Keywords") );
            docInfo.set( Okular::DocumentInfo::Creator, pdfdoc->info("Creator") );
            docInfo.set( Okular::DocumentInfo::Producer, pdfdoc->info("Producer") );
            docInfo.set( Okular::DocumentInfo::CreationDate,
                         KGlobal::locale()->formatDateTime( pdfdoc->date("CreationDate"), KLocale::LongDate, true ) );
            docInfo.set( Okular::DocumentInfo::ModificationDate,
                         KGlobal::locale()->formatDateTime( pdfdoc->date("ModDate"), KLocale::LongDate, true ) );

            docInfo.set( "format", i18nc( "PDF v. <version>", "PDF v. %1",
                          pdfdoc->pdfVersion() ), i18n( "Format" ) );
            docInfo.set( "encryption", pdfdoc->isEncrypted() ? i18n( "Encrypted" ) : i18n( "Unencrypted" ),
                         i18n("Security") );
            docInfo.set( "optimization", pdfdoc->isLinearized() ? i18n( "Yes" ) : i18n( "No" ),
                         i18n("Optimized") );

            docInfo.set( Okular::DocumentInfo::Pages, QString::number( pdfdoc->numPages() ) );
        }
        else
        {
            // TODO not sure one can reach here, check and if it is not possible, remove the code
            docInfo.set( Okular::DocumentInfo::Title, i18n("Unknown") );
            docInfo.set( Okular::DocumentInfo::Subject, i18n("Unknown") );
            docInfo.set( Okular::DocumentInfo::Author, i18n("Unknown") );
            docInfo.set( Okular::DocumentInfo::Keywords, i18n("Unknown") );
            docInfo.set( Okular::DocumentInfo::Creator, i18n("Unknown") );
            docInfo.set( Okular::DocumentInfo::Producer, i18n("Unknown") );
            docInfo.set( Okular::DocumentInfo::CreationDate, i18n("Unknown Date") );
            docInfo.set( Okular::DocumentInfo::ModificationDate, i18n("Unknown Date") );

            docInfo.set( "format", "PDF", i18n( "Format" ) );
            docInfo.set( "encryption", i18n( "Unknown Encryption" ), i18n( "Security" ) );
            docInfo.set( "optimization", i18n( "Unknown Optimization" ), i18n( "Optimized" ) );

            docInfo.set( Okular::DocumentInfo::Pages, i18n("Unknown") );
        }
        docLock.unlock();

        // if pdfdoc is valid then we cached good info -> don't cache them again
        if ( pdfdoc )
            docInfoDirty = false;
    }
    return &docInfo;
}

const Okular::DocumentSynopsis * PDFGenerator::generateDocumentSynopsis()
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

    addSynopsisChildren(toc, &docSyn);
    delete toc;

    docSynopsisDirty = false;
    return &docSyn;
}

static Okular::FontInfo::FontType convertPopplerFontInfoTypeToOkularFontInfoType( Poppler::FontInfo::Type type )
{
    switch ( type )
    {
        case Poppler::FontInfo::Type1:
            return Okular::FontInfo::Type1;
            break;
        case Poppler::FontInfo::Type1C:
            return Okular::FontInfo::Type1C;
            break;
        case Poppler::FontInfo::Type3:
            return Okular::FontInfo::Type3;
            break;
        case Poppler::FontInfo::TrueType:
            return Okular::FontInfo::TrueType;
            break;
        case Poppler::FontInfo::CIDType0:
            return Okular::FontInfo::CIDType0;
            break;
        case Poppler::FontInfo::CIDType0C:
            return Okular::FontInfo::CIDType0C;
            break;
        case Poppler::FontInfo::CIDTrueType:
            return Okular::FontInfo::CIDTrueType;
            break;
#ifdef HAVE_POPPLER_0_6
        case Poppler::FontInfo::Type1COT:
            return Okular::FontInfo::Type1COT;
            break;
        case Poppler::FontInfo::TrueTypeOT:
            return Okular::FontInfo::TrueTypeOT;
            break;
        case Poppler::FontInfo::CIDType0COT:
            return Okular::FontInfo::CIDType0COT;
            break;
        case Poppler::FontInfo::CIDTrueTypeOT:
            return Okular::FontInfo::CIDTrueTypeOT;
            break;
#endif
        case Poppler::FontInfo::unknown:
        default: ;
     }
     return Okular::FontInfo::Unknown;
}

Okular::FontInfo::List PDFGenerator::fontsForPage( int /*page*/ )
{
    Okular::FontInfo::List list;

    docLock.lock();
    QList<Poppler::FontInfo> fonts;
    pdfdoc->scanForFonts( 1, &fonts );
    docLock.unlock();

    foreach (const Poppler::FontInfo &font, fonts)
    {
        Okular::FontInfo of;
        of.setName( font.name() );
        of.setType( convertPopplerFontInfoTypeToOkularFontInfoType( font.type() ) );
        of.setEmbedded( font.isEmbedded() );
        of.setFile( font.file() );

        list.append( of );
    }

    return list;
}

const QList<Okular::EmbeddedFile*> *PDFGenerator::embeddedFiles() const
{
    if (docEmbeddedFilesDirty)
    {
        docLock.lock();
        const QList<Poppler::EmbeddedFile*> &popplerFiles = pdfdoc->embeddedFiles();
        foreach(Poppler::EmbeddedFile* pef, popplerFiles)
        {
            docEmbeddedFiles.append(new PDFEmbeddedFile(pef));
        }
        docLock.unlock();

        docEmbeddedFilesDirty = false;
    }

    return &docEmbeddedFiles;
}

bool PDFGenerator::isAllowed( Okular::Permission permission ) const
{
    bool b = true;
    switch ( permission )
    {
        case Okular::AllowModify:
            b = pdfdoc->okToChange();
            break;
        case Okular::AllowCopy:
            b = pdfdoc->okToCopy();
            break;
        case Okular::AllowPrint:
            b = pdfdoc->okToPrint();
            break;
        case Okular::AllowNotes:
            b = pdfdoc->okToAddNotes();
            break;
        case Okular::AllowFillForms:
            b = pdfdoc->okToFillForm();
            break;
        default: ;
    }
    return b;
}

bool PDFGenerator::canGeneratePixmap() const
{
    return ready;
}

void PDFGenerator::generatePixmap( Okular::PixmapRequest * request )
{
#ifndef NDEBUG
    if ( !ready )
        kDebug(PDFDebug) << "calling generatePixmap() when not in READY state!" << endl;
#endif
    // update busy state (not really needed here, because the flag needs to
    // be set only to prevent asking a pixmap while the thread is running)
    ready = false;

    // debug requests to this (xpdf) generator
    //kDebug(PDFDebug) << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "]." << endl;

    /** asynchronous requests (generation in PDFPixmapGeneratorThread::run() **/
    if ( request->asynchronous() )
    {
        // start the generation into the thread
        generatorThread->startGeneration( request );
        return;
    }

    /** synchronous request: in-place generation **/
    // compute dpi used to get an image with desired width and height
    Okular::Page * page = request->page();

    double pageWidth = page->width(),
           pageHeight = page->height();

    if ( page->rotation() % 2 )
        qSwap( pageWidth, pageHeight );

    double fakeDpiX = request->width() * 72.0 / pageWidth,
           fakeDpiY = request->height() * 72.0 / pageHeight;

    // setup Okular:: output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasTextPage() && (request->width() == page->width()) &&
                       (request->height() == page->height());
    // generate links rects only the first time
    bool genObjectRects = !rectsGenerated.at( page->number() );

    // 0. LOCK [waits for the thread end]
    docLock.lock();

    // 1. Set OutputDev parameters and Generate contents
    // note: thread safety is set on 'false' for the GUI (this) thread
    Poppler::Page *p = pdfdoc->page(page->number());

    // 2. Take data from outputdev and attach it to the Page
#ifdef HAVE_POPPLER_0_6
    page->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( p->renderToImage(fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0 ) ) ) );
#else
    page->setPixmap( request->id(), p->splashRenderToPixmap(fakeDpiX, fakeDpiY, -1, -1, request->width(), request->height(), genObjectRects, Poppler::Page::Rotate0 ) );
#endif
    
    if ( genObjectRects )
    {
    	// TODO previously we extracted Image type rects too, but that needed porting to poppler
        // and as we are not doing anything with Image type rects i did not port it, have a look at
        // dead gp_outputdev.cpp on image extraction
        page->setObjectRects( generateLinks(p->links(), request->width(), request->height(), pdfdoc) );
        rectsGenerated[ request->page()->number() ] = true;
    }

    // 3. UNLOCK [re-enables shared access]
    docLock.unlock();
    if ( genTextPage )
    {
        QList<Poppler::TextBox*> textList = p->textList((Poppler::Page::Rotation)request->page()->orientation());
        page->setTextPage( abstractTextPage(textList, page->height(), page->width(), request->page()->orientation()) );
        qDeleteAll(textList);
    }
    delete p;
    
    // update ready state
    ready = true;

    // notify the new generation
    signalPixmapRequestDone( request );
}

Okular::TextPage* PDFGenerator::textPage( Okular::Page *page )
{
    kDebug(PDFDebug) << "calling textPage( Okular::Page * page )" << endl;
    // build a TextList...
    Poppler::Page *pp = pdfdoc->page( page->number() );
    docLock.lock();
    QList<Poppler::TextBox*> textList = pp->textList((Poppler::Page::Rotation)page->orientation());
    docLock.unlock();
    delete pp;

    const double pageWidth = ( page->rotation() % 2 ? page->height() : page->width() );
    const double pageHeight = ( page->rotation() % 2 ? page->width() : page->height() );

    Okular::TextPage *tp = abstractTextPage(textList, pageHeight, pageWidth, (Poppler::Page::Rotation)page->orientation());
    qDeleteAll(textList);
    return tp;
}

bool PDFGenerator::print( KPrinter& printer )
{
    int width, height;
    // PageSize is a CUPS artificially created setting 
    QString ps = printer.option( "PageSize" );
    QRegExp sizere( "w(\\d+)h(\\d+)" );
    int marginTop, marginLeft, marginRight, marginBottom;
    marginTop = (int)printer.option("kde-margin-top").toDouble();
    marginLeft = (int)printer.option("kde-margin-left").toDouble();
    marginRight = (int)printer.option("kde-margin-right").toDouble();
    marginBottom = (int)printer.option("kde-margin-bottom").toDouble();
    if ( sizere.exactMatch( ps ) )
    {
        // size not supported by Qt, CUPS gives us the size as wWIDTHhHEIGHT, at least on the printers i tested
        width = sizere.cap( 1 ).toInt();
        height = sizere.cap( 2 ).toInt();
    }
    else
    {
        // size is supported by Qt, we get either the pageSize name or nothing because the CUPS driver
        // does not do any translation, then use KPrinter::pageSize to get the page size
        KPrinter::PageSize qtPageSize; 
        if (!ps.isEmpty())
        {
            bool ok;
            qtPageSize = pageNameToPageSize(ps, &ok);
            // If we could not decode page size from the cups text try KPrinter::pageSize as last resort :-D
            if (!ok) qtPageSize = printer.pageSize();
        }
        else qtPageSize = printer.pageSize();

        QPrinter dummy(QPrinter::PrinterResolution);
        dummy.setOrientation((QPrinter::Orientation)printer.orientation());
        dummy.setFullPage(true);
        dummy.setPageSize((QPrinter::PageSize)qtPageSize);

        width = dummy.width();
        height = dummy.height();
    }

    KTemporaryFile tf;
    tf.setSuffix( ".ps" );
    if ( !tf.open() )
        return false;
    QString tempfilename = tf.fileName();
    tf.close();

    QList<int> pageList;
    if (!printer.previewOnly()) pageList = printer.pageList();
    else for(int i = 1; i <= pdfdoc->numPages(); i++) pageList.push_back(i);
    
    docLock.lock();
    // TODO rotation
#ifdef HAVE_POPPLER_0_6
    double xScale = ((double)width - (double)marginLeft - (double)marginRight) / (double)width;
    double yScale = ((double)height - (double)marginBottom - (double)marginTop) / (double)height;
    bool strictMargins = false;
    if ( abs((int)(xScale * 100) - (int)(yScale * 100)) > 5 ) {
        int result = KMessageBox::questionYesNo(document()->widget(),
                                               i18n("The margins you specified are changing the page aspect ratio. Do you want to print with the aspect ratio changed or do you want the margins to be adapted so that aspect ratio is preserved?"),
                                               i18n("Aspect ratio change"),
                                               KGuiItem( i18n("Print with specified margins") ),
                                               KGuiItem( i18n("Print adapting margins to keep aspect ratio") ),
                                               "kpdfStrictlyObeyMargins");
        if (result == KMessageBox::Yes) strictMargins = true;
    }
    QString pstitle = metaData(QLatin1String("Title"), QVariant()).toString();
    if ( pstitle.trimmed().isEmpty() )
    {
        pstitle = document()->currentDocument().path();
    }
    bool forceRasterize = printer.option("kde-okular-poppler-forceRaster").toInt();
    Poppler::PSConverter *psConverter = pdfdoc->psConverter();
    psConverter->setOutputFileName(tempfilename);
    psConverter->setPageList(pageList);
    psConverter->setPaperWidth(width);
    psConverter->setPaperHeight(height);
    psConverter->setRightMargin(marginRight);
    psConverter->setBottomMargin(marginBottom);
    psConverter->setLeftMargin(marginLeft);
    psConverter->setTopMargin(marginTop);
    psConverter->setStrictMargins(strictMargins);
    psConverter->setForceRasterize(forceRasterize);
    psConverter->setTitle(pstitle);
    if (psConverter->convert())
    {
        docLock.unlock();
        delete psConverter;
        return printer.printFiles(QStringList(tempfilename), true);
    }
    else
    {
        delete psConverter;
#else
    if (pdfdoc->print(tempfilename, pageList, 72, 72, 0, width, height))
    {
        docLock.unlock();
        return printer.printFiles(QStringList(tempfilename), true);
    }
    else
    {
#endif
        docLock.unlock();
        return false;
    }
	return false;
}

QVariant PDFGenerator::metaData( const QString & key, const QVariant & option ) const
{
    if ( key == "StartFullScreen" )
    {
        // asking for the 'start in fullscreen mode' (pdf property)
        if ( pdfdoc->pageMode() == Poppler::Document::FullScreen )
            return true;
    }
    else if ( key == "NamedViewport" && !option.toString().isEmpty() )
    {
        // asking for the page related to a 'named link destination'. the
        // option is the link name. @see addSynopsisChildren.
        Okular::DocumentViewport viewport;
        docLock.lock();
        Poppler::LinkDestination *ld = pdfdoc->linkDestination( option.toString() );
        docLock.unlock();
        if ( ld )
        {
            fillViewportFromLinkDestination( viewport, *ld, pdfdoc );
        }
        delete ld;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    else if ( key == "DocumentTitle" )
    {
        docLock.lock();
        QString title = pdfdoc->info( "Title" );
        docLock.unlock();
        return title;
    }
    else if ( key == "OpenTOC" )
    {
        if ( pdfdoc->pageMode() == Poppler::Document::UseOutlines )
            return true;
    }
    return QVariant();
}

bool PDFGenerator::reparseConfig()
{
    // load paper color from Settings or use the white default color
    QColor color = ( (Okular::Settings::renderMode() == Okular::Settings::EnumRenderMode::Paper ) &&
                     Okular::Settings::changeColors() ) ? Okular::Settings::paperColor() : Qt::white;
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

void PDFGenerator::addPages( KConfigDialog * )
{
}

Okular::ExportFormat::List PDFGenerator::exportFormats() const
{
    static Okular::ExportFormat::List formats;
    if ( formats.isEmpty() ) {
        formats.append( Okular::ExportFormat::standardFormat( Okular::ExportFormat::PlainText ) );
    }

    return formats;
}

bool PDFGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
{
    if ( format.mimeType()->name() == QLatin1String( "text/plain" ) ) {
        QFile f( fileName );
        if ( !f.open( QIODevice::WriteOnly ) )
            return false;

        QTextStream ts( &f );
        int num = document()->pages();
        for ( int i = 0; i < num; ++i )
        {
            docLock.lock();
            Poppler::Page *pp = pdfdoc->page(i);
            QString text = pp->text(QRect());
            docLock.unlock();
            ts << text;
            delete pp;
        }
        f.close();

        return true;
    }

    return false;
}

//END Generator inherited functions

inline void append (Okular::TextPage* ktp,
    const QString &s, double l, double b, double r, double t)
{
//       kWarning(PDFDebug) << "text: " << s << " at (" << l << "," << t << ")x(" << r <<","<<b<<")" << endl;
                ktp->append( s ,
                    new Okular::NormalizedRect(
                    l,
                    t,
                    r,
                    b
                    ));
}

Okular::TextPage * PDFGenerator::abstractTextPage(const QList<Poppler::TextBox*> &text, double height, double width,int rot)
{    
    Okular::TextPage* ktp=new Okular::TextPage;
    Poppler::TextBox *next; 
    kWarning(PDFDebug) << "getting text page in generator pdf - rotation: " << rot << endl;
    int charCount=0;
    int j;
    QString s;
    Okular::NormalizedRect * wordRect = new Okular::NormalizedRect;
    
    rot = rot % 4;
    
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
                append(ktp, (j==charCount-1 && !next ) ? (s + '\n') : s,
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
                append(ktp, (j==charCount-1 && !next ) ? (s + '\n') : s,
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
                append(ktp, (j==charCount-1 && !next ) ? (s + '\n') : s,
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
                append(ktp, (j==charCount-1 && !next ) ? (s + '\n') : s,
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
            Okular::DocumentViewport vp;
            fillViewportFromLinkDestination( vp, Poppler::LinkDestination(e.attribute("Destination")), pdfdoc );
            item.setAttribute( "Viewport", vp.toString() );
        }
        if (!e.attribute("Open").isNull()) item.setAttribute("Open", e.attribute("Open"));

        // descend recursively and advance to the next node
        if ( e.hasChildNodes() ) addSynopsisChildren( &n, &	item );
        n = n.nextSibling();
    }
}

void PDFGenerator::addAnnotations( Poppler::Page * popplerPage, Okular::Page * page )
{
    QList<Poppler::Annotation*> popplerAnnotations = popplerPage->annotations();
    foreach(Poppler::Annotation *a, popplerAnnotations)
    {
        a->window.width = (int)(page->width() * a->window.width);
        a->window.height = (int)(page->height() * a->window.height);
        //a->window.width = a->window.width < 200 ? 200 : a->window.width;
        // a->window.height = a->window.height < 120 ? 120 : a->window.height;
        // resize annotation's geometry to an icon
        // TODO okular geom.right = geom.left + 22.0 / page->width();
        // TODO okular geom.bottom = geom.top + 22.0 / page->height();
        /*
        QString szanno;
        QTextStream(&szanno)<<"PopplerAnnotation={author:"<<a->author
                <<", contents:"<<a->contents
                <<", uniqueName:"<<a->uniqueName
                <<", modifyDate:"<<a->modifyDate.toString("hh:mm:ss, dd.MM.yyyy")
                <<", creationDate:"<<a->creationDate.toString("hh:mm:ss, dd.MM.yyyy")
                <<", flags:"<<a->flags
                <<", boundary:"<<a->boundary.left()<<","<<a->boundary.top()<<","<<a->boundary.right()<<","<<a->boundary.bottom()
                <<", style.color:"<<a->style.color.name()
                <<", style.opacity:"<<a->style.opacity
                <<", style.width:"<<a->style.width
                <<", style.LineStyle:"<<a->style.style
                <<", style.xyCorners:"<<a->style.xCorners<<","<<a->style.yCorners
                <<", style.marks:"<<a->style.marks
                <<", style.spaces:"<<a->style.spaces
                <<", style.LineEffect:"<<a->style.effect
                <<", style.effectIntensity:"<<a->style.effectIntensity
                <<", window.flags:"<<a->window.flags
                <<", window.topLeft:"<<(a->window.topLeft.x())
                <<","<<(a->window.topLeft.y())
                <<", window.width,height:"<<a->window.width<<","<<a->window.height
                <<", window.title:"<<a->window.title
                <<", window.summary:"<<a->window.summary
                <<", window.text:"<<a->window.text;
        kDebug(PDFDebug)<<"astario:    "<<szanno<<endl;*/
        // this is uber ugly but i don't know a better way to do it without introducing a poppler::annotation dependency on core
        //TODO add annotations after poppler write feather is full suported
        QDomDocument doc;
        QDomElement root = doc.createElement("root");
        doc.appendChild(root);
        Poppler::AnnotationUtils::storeAnnotation(a, root, doc);
        Okular::Annotation * newann = Okular::AnnotationUtils::createAnnotation(root);
        if (newann)
        {
            // the Contents field has lines separated by \r
            QString contents = newann->contents();
            contents.replace( QLatin1Char( '\r' ), QLatin1Char( '\n' ) );
            newann->setContents( contents );
            page->addAnnotation(newann);
        }
    }
    qDeleteAll(popplerAnnotations);
}

void PDFGenerator::addTransition( Poppler::Page * pdfPage, Okular::Page * page )
// called on opening when MUTEX is not used
{
    Poppler::PageTransition *pdfTransition = pdfPage->transition();
    if ( !pdfTransition || pdfTransition->type() == Poppler::PageTransition::Replace )
        return;

    Okular::PageTransition *transition = new Okular::PageTransition();
    switch ( pdfTransition->type() ) {
        case Poppler::PageTransition::Replace:
            // won't get here, added to avoid warning
            break;
        case Poppler::PageTransition::Split:
            transition->setType( Okular::PageTransition::Split );
            break;
        case Poppler::PageTransition::Blinds:
            transition->setType( Okular::PageTransition::Blinds );
            break;
        case Poppler::PageTransition::Box:
            transition->setType( Okular::PageTransition::Box );
            break;
        case Poppler::PageTransition::Wipe:
            transition->setType( Okular::PageTransition::Wipe );
            break;
        case Poppler::PageTransition::Dissolve:
            transition->setType( Okular::PageTransition::Dissolve );
            break;
        case Poppler::PageTransition::Glitter:
            transition->setType( Okular::PageTransition::Glitter );
            break;
        case Poppler::PageTransition::Fly:
            transition->setType( Okular::PageTransition::Fly );
            break;
        case Poppler::PageTransition::Push:
            transition->setType( Okular::PageTransition::Push );
            break;
        case Poppler::PageTransition::Cover:
            transition->setType( Okular::PageTransition::Cover );
            break;
        case Poppler::PageTransition::Uncover:
            transition->setType( Okular::PageTransition::Uncover );
            break;
        case Poppler::PageTransition::Fade:
            transition->setType( Okular::PageTransition::Fade );
            break;
    }

    transition->setDuration( pdfTransition->duration() );

    switch ( pdfTransition->alignment() ) {
        case Poppler::PageTransition::Horizontal:
            transition->setAlignment( Okular::PageTransition::Horizontal );
            break;
        case Poppler::PageTransition::Vertical:
            transition->setAlignment( Okular::PageTransition::Vertical );
            break;
    }

    switch ( pdfTransition->direction() ) {
        case Poppler::PageTransition::Inward:
            transition->setDirection( Okular::PageTransition::Inward );
            break;
        case Poppler::PageTransition::Outward:
            transition->setDirection( Okular::PageTransition::Outward );
            break;
    }

    transition->setAngle( pdfTransition->angle() );
    transition->setScale( pdfTransition->scale() );
    transition->setIsRectangular( pdfTransition->isRectangular() );

    page->setTransition( transition );
}

void PDFGenerator::addFormFields( Poppler::Page * popplerPage, Okular::Page * page )
{
#ifdef HAVE_POPPLER_0_6
    QList<Poppler::FormField*> popplerFormFields = popplerPage->formFields();
    QLinkedList<Okular::FormField*> okularFormFields;
    foreach( Poppler::FormField *f, popplerFormFields )
    {
        Okular::FormField * of = 0;
        switch ( f->type() )
        {
            case Poppler::FormField::FormText:
                of = new PopplerFormFieldText( static_cast<Poppler::FormFieldText*>( f ) );
                break;
            case Poppler::FormField::FormChoice:
                of = new PopplerFormFieldChoice( static_cast<Poppler::FormFieldChoice*>( f ) );
                break;
            default: ;
        }
        if ( of )
            // form field created, good - it will take care of the Poppler::FormField
            okularFormFields.append( of );
        else
            // no form field available - delete the Poppler::FormField
            delete f;
    }
    if ( !okularFormFields.isEmpty() )
        page->setFormFields( okularFormFields );
#else
    Q_UNUSED( popplerPage )
    Q_UNUSED( page )
#endif
}

struct pdfsyncpoint
{
    QString file;
    qlonglong x;
    qlonglong y;
    int row;
    int column;
    int page;
};

void PDFGenerator::loadPdfSync( const QString & filePath, QVector<Okular::Page*> & pagesVector )
{
    QFile f( filePath + QLatin1String( "sync" ) );
    if ( !f.open( QIODevice::ReadOnly ) )
        return;

    QTextStream ts( &f );
    // first row: core name of the pdf output - we skip it
    ts.readLine();
    // second row: version string, in the form 'Version %u'
    QString versionstr = ts.readLine();
    QRegExp versionre( "Version (\\d+)" );
    versionre.setCaseSensitivity( Qt::CaseInsensitive );
    if ( !versionre.exactMatch( versionstr ) )
        return;

    QHash<int, pdfsyncpoint> points;
    QString currentfile;
    int currentpage = -1;
    QRegExp newfilere( "\\(\\s*([^\\s]+)" );
    QRegExp linere( "l\\s+(\\d+)\\s+(\\d+)(\\s+(\\d+))?" );
    QRegExp pagere( "s\\s+(\\d+)" );
    QRegExp locre( "p\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)" );
    QRegExp locstarre( "p\\*\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)" );

    QString line;
    while ( !ts.atEnd() )
    {
        line = ts.readLine();
        if ( line.startsWith( QLatin1Char( 'l' ) ) && linere.exactMatch( line ) )
        {
            int id = linere.cap( 1 ).toInt();
            QHash<int, pdfsyncpoint>::const_iterator it = points.find( id );
            if ( it == points.end() )
            {
                pdfsyncpoint pt;
                pt.x = 0;
                pt.y = 0;
                pt.row = linere.cap( 2 ).toInt();
                pt.column = 0; // TODO
                pt.page = -1;
                pt.file = currentfile;
                points[ id ] = pt;
            }
        }
        else if ( line.startsWith( QLatin1Char( 's' ) ) && pagere.exactMatch( line ) )
        {
            currentpage = pagere.cap( 1 ).toInt() - 1;
        }
        else if ( line.startsWith( QLatin1String( "p*" ) ) && locstarre.exactMatch( line ) )
        {
            // TODO
            kDebug(PDFDebug) << "PdfSync: 'p*' line ignored" << endl;
        }
        else if ( line.startsWith( QLatin1Char( 'p' ) ) && locre.exactMatch( line ) )
        {
            int id = locre.cap( 1 ).toInt();
            QHash<int, pdfsyncpoint>::iterator it = points.find( id );
            if ( it != points.end() )
            {
                it->x = locre.cap( 2 ).toInt();
                it->y = locre.cap( 3 ).toInt();
                it->page = currentpage;
            }
        }
        else if ( line.startsWith( QLatin1Char( '(' ) ) && newfilere.exactMatch( line ) )
        {
            QString newfile = newfilere.cap( 1 );
            if ( currentfile.isEmpty() )
            {
                currentfile = newfile;
            }
            else
                kDebug(PDFDebug) << "PdfSync: more than one file level: " << newfile << endl;
        }
        else if ( line == QLatin1String( ")" ) )
        {
            if ( !currentfile.isEmpty() )
            {
                currentfile.clear();
            }
            else
                kDebug(PDFDebug) << "PdfSync: going one level down: " << currentfile << endl;
        }
        else
            kDebug(PDFDebug) << "PdfSync: unknown line format: '" << line << "'" << endl;

    }

    QVector< QLinkedList< Okular::SourceRefObjectRect * > > refRects( pagesVector.size() );
    foreach ( const pdfsyncpoint& pt, points )
    {
        // drop pdfsync points not completely valid
        if ( pt.page < 0 || pt.page >= pagesVector.size() )
            continue;

        // maginc numbers for TeX's RSU's (Ridiculously Small Units) conversion to pixels
        Okular::NormalizedPoint p(
            ( pt.x * 72.0 ) / ( 72.27 * 65536.0 * pagesVector[pt.page]->width() ),
            ( pt.y * 72.0 ) / ( 72.27 * 65536.0 * pagesVector[pt.page]->height() )
            );
        QString file;
        if ( !pt.file.isEmpty() )
        {
            file = pt.file;
            int dotpos = file.lastIndexOf( QLatin1Char( '.' ) );
            QString ext;
            if ( dotpos == -1 )
                file += QString::fromLatin1( ".tex" );
        }
        else
        {
            file = filePath;
        }
        Okular::SourceReference * sourceRef = new Okular::SourceReference( file, pt.row, pt.column );
        refRects[ pt.page ].append( new Okular::SourceRefObjectRect( p, sourceRef ) );
    }
    for ( int i = 0; i < refRects.size(); ++i )
        if ( !refRects.at(i).isEmpty() )
            pagesVector[i]->setSourceReferences( refRects.at(i) );
}

KPrintDialogPage* PDFGenerator::printConfigurationWidget() const
{
#ifdef HAVE_POPPLER_0_6
    return new PDFOptionsPage();
#else
    return 0;
#endif
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
            kWarning(PDFDebug) << "PDFGenerator: thread sent 'data available' "
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
        kWarning(PDFDebug) << "PDFGenerator: 'data available' but mutex still "
                    << "held. Recovering." << endl;
        // synchronize GUI thread (must not happen)
        docLock.lock();
        docLock.unlock();
    }

    // 2. put thread's generated data into the Okular::Page
    Okular::PixmapRequest * request = generatorThread->request();
    QImage * outImage = generatorThread->takeImage();
    QList<Poppler::TextBox*> outText = generatorThread->takeText();
    QLinkedList< Okular::ObjectRect * > outRects = generatorThread->takeObjectRects();

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( *outImage ) ) );
    delete outImage;
    if ( !outText.isEmpty() )
    {
        request->page()->setTextPage( abstractTextPage( outText , 
            request->page()->height(), request->page()->width(),request->page()->orientation()));
        qDeleteAll(outText);
    }
    bool genObjectRects = !rectsGenerated.at( request->page()->number() );
    if (genObjectRects)
    {
        request->page()->setObjectRects( outRects );
        rectsGenerated[ request->page()->number() ] = true;
    }
    else
        qDeleteAll( outRects );

    // 3. tell generator that data has been taken
    generatorThread->endGeneration();

    // update ready state
    ready = true;
    // notify the new generation
    signalPixmapRequestDone( request );
}



/** The  PDF Pixmap Generator Thread  **/

struct PPGThreadPrivate
{
    // reference to main objects
    PDFGenerator * generator;
    Okular::PixmapRequest * currentRequest;

    // internal temp stored items. don't delete this.
    QImage * m_image;
    QList<Poppler::TextBox*> m_textList;
    QLinkedList< Okular::ObjectRect * > m_rects;
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

void PDFPixmapGeneratorThread::startGeneration( Okular::PixmapRequest * request )
{
#ifndef NDEBUG
    // check if a generation is already running
    if ( d->currentRequest )
    {
        kDebug(PDFDebug) << "PDFPixmapGeneratorThread: requesting a pixmap "
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
        kDebug(PDFDebug) << "PDFPixmapGeneratorThread: requesting a pixmap "
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
        kDebug(PDFDebug) << "PDFPixmapGeneratorThread: 'end generation' called "
                  << "but generation was not started." << endl;
        return;
    }
#endif
    // reset internal members preparing for a new generation
    d->currentRequest = 0;
}

Okular::PixmapRequest *PDFPixmapGeneratorThread::request() const
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

QLinkedList< Okular::ObjectRect * > PDFPixmapGeneratorThread::takeObjectRects() const
{
    d->m_rectsTaken = true;
    QLinkedList< Okular::ObjectRect * > newrects = d->m_rects;
    d->m_rects.clear();
    return newrects;
}

void PDFPixmapGeneratorThread::run()
// perform contents generation, when the MUTEX is already LOCKED
// @see PDFGenerator::generatePixmap( .. ) (and be aware to sync the code)
{
    // compute dpi used to get an image with desired width and height
    Okular::Page * page = d->currentRequest->page();
    int width = d->currentRequest->width(),
        height = d->currentRequest->height();
    double pageWidth = page->width(),
           pageHeight = page->height();

    if ( page->rotation() % 2 )
        qSwap( pageWidth, pageHeight );

    double fakeDpiX = width * 72.0 / pageWidth,
           fakeDpiY = height * 72.0 / pageHeight;

    // setup Okular:: output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasTextPage() &&
                       ( width == page->width() ) &&
                       ( height == page->height() );

    // generate links rects only the first time
    bool genObjectRects = !d->generator->rectsGenerated.at( page->number() );

    // 0. LOCK s[tart locking XPDF thread unsafe classes]
    d->generator->docLock.lock();

    // 1. set OutputDev parameters and Generate contents
    Poppler::Page *pp = d->generator->pdfdoc->page( page->number() );
    
    // 2. grab data from the OutputDev and store it locally (note takeIMAGE)
#ifndef NDEBUG
    if ( d->m_image )
        kDebug(PDFDebug) << "PDFPixmapGeneratorThread: previous image not taken" << endl;
    if ( !d->m_textList.isEmpty() )
        kDebug(PDFDebug) << "PDFPixmapGeneratorThread: previous text not taken" << endl;
#endif
#ifdef HAVE_POPPLER_0_6
    d->m_image = new QImage( pp->renderToImage( fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0 ) );
#else
    d->m_image = new QImage( pp->splashRenderToImage( fakeDpiX, fakeDpiY, -1, -1, width, height, genObjectRects, Poppler::Page::Rotate0 ) );
#endif
    
    if ( genObjectRects )
    {
    	d->m_rects = generateLinks(pp->links(), width, height, d->generator->pdfdoc);
    }
    else d->m_rectsTaken = false;

    if ( genTextPage )
    {
        d->m_textList = pp->textList((Poppler::Page::Rotation)d->currentRequest->page()->orientation());
    }
    delete pp;
    
    // 3. [UNLOCK] mutex
    d->generator->docLock.unlock();

    // by ending the thread notifies the GUI thread that data is pending and can be read
}
#include "generator_pdf.moc"

