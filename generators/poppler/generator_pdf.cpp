/***************************************************************************
 *   Copyright (C) 2004-2008 by Albert Astals Cid <tsdgeos@terra.es>       *
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
#include <qmutex.h>
#include <qregexp.h>
#include <qstack.h>
#include <qtextstream.h>
#include <QtGui/QPrinter>
#include <QtGui/QPainter>

#include <kaboutdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpassworddialog.h>
#include <kwallet.h>
#include <ktemporaryfile.h>
#include <kdebug.h>
#include <kglobal.h>

#include <core/action.h>
#include <core/page.h>
#include <core/annotations.h>
#include <core/movie.h>
#include <core/pagetransition.h>
#include <core/sound.h>
#include <core/sourcereference.h>
#include <core/textpage.h>
#include <core/fileprinter.h>
#include <core/utils.h>

#include <config-okular-poppler.h>

#include "formfields.h"
#include "popplerembeddedfile.h"

#ifdef HAVE_POPPLER_0_9
Q_DECLARE_METATYPE(Poppler::FontInfo)
#endif

static const int PDFDebug = 4710;
static const int defaultPageWidth = 595;
static const int defaultPageHeight = 842;

class PDFOptionsPage : public QWidget
{
   public:
       PDFOptionsPage()
       {
           setWindowTitle( i18n( "PDF Options" ) );
           QVBoxLayout *layout = new QVBoxLayout(this);
           m_forceRaster = new QCheckBox(i18n("Force rasterization"), this);
           m_forceRaster->setToolTip(i18n("Rasterize into an image before printing"));
           m_forceRaster->setWhatsThis(i18n("Forces the rasterization of each page into an image before printing it. This usually gives somewhat worse results, but is useful when printing documents that appear to print incorrectly."));
           layout->addWidget(m_forceRaster);
           layout->addStretch(1);
       }

       bool printForceRaster()
       {
           return m_forceRaster->isChecked();
       }

       void setPrintForceRaster( bool forceRaster )
       {
           m_forceRaster->setChecked( forceRaster );
       }

    private:
        QCheckBox *m_forceRaster;
};


static void fillViewportFromLinkDestination( Okular::DocumentViewport &viewport, const Poppler::LinkDestination &destination )
{
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

                viewport.rePos.normalizedX = left;
                viewport.rePos.normalizedY = top;
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

Okular::Sound* createSoundFromPopplerSound( const Poppler::SoundObject *popplerSound )
{
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
    return sound;
}

#ifdef HAVE_POPPLER_0_9
Okular::Movie* createMovieFromPopplerMovie( const Poppler::MovieObject *popplerMovie )
{
    Okular::Movie *movie = new Okular::Movie( popplerMovie->url() );
    movie->setSize( popplerMovie->size() );
    movie->setRotation( (Okular::Rotation)( popplerMovie->rotation() / 90 ) );
    movie->setShowControls( popplerMovie->showControls() );
    movie->setPlayMode( (Okular::Movie::PlayMode)popplerMovie->playMode() );
    return movie;
}
#endif

Okular::Action* createLinkFromPopplerLink(const Poppler::Link *popplerLink)
{
	Okular::Action *link = 0;
	const Poppler::LinkGoto *popplerLinkGoto;
	const Poppler::LinkExecute *popplerLinkExecute;
	const Poppler::LinkBrowse *popplerLinkBrowse;
	const Poppler::LinkAction *popplerLinkAction;
	const Poppler::LinkSound *popplerLinkSound;
#ifdef HAVE_POPPLER_0_9
	const Poppler::LinkJavaScript *popplerLinkJS;
#endif
	Okular::DocumentViewport viewport;
	
	switch(popplerLink->linkType())
	{
		case Poppler::Link::None:
		break;
	
		case Poppler::Link::Goto:
		{
			popplerLinkGoto = static_cast<const Poppler::LinkGoto *>(popplerLink);
#ifdef HAVE_POPPLER_0_11
			const Poppler::LinkDestination dest = popplerLinkGoto->destination();
			const QString destName = dest.destinationName();
			if (destName.isEmpty())
			{
				fillViewportFromLinkDestination( viewport, dest );
				link = new Okular::GotoAction(popplerLinkGoto->fileName(), viewport);
			}
			else
			{
				link = new Okular::GotoAction(popplerLinkGoto->fileName(), destName);
			}
#else
			fillViewportFromLinkDestination( viewport, popplerLinkGoto->destination() );
			link = new Okular::GotoAction(popplerLinkGoto->fileName(), viewport);
#endif
		}
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
		
		case Poppler::Link::Sound:
		{
			popplerLinkSound = static_cast<const Poppler::LinkSound *>(popplerLink);
			Poppler::SoundObject *popplerSound = popplerLinkSound->sound();
			Okular::Sound *sound = createSoundFromPopplerSound( popplerSound );
			link = new Okular::SoundAction( popplerLinkSound->volume(), popplerLinkSound->synchronous(), popplerLinkSound->repeat(), popplerLinkSound->mix(), sound );
		}
		break;
		
#ifdef HAVE_POPPLER_0_9
		case Poppler::Link::JavaScript:
		{
			popplerLinkJS = static_cast<const Poppler::LinkJavaScript *>(popplerLink);
			link = new Okular::ScriptAction( Okular::JavaScript, popplerLinkJS->script() );
		}
		break;
#endif
		
		case Poppler::Link::Movie:
			// not implemented
		break;
	}
	
	return link;
}

static QLinkedList<Okular::ObjectRect*> generateLinks( const QList<Poppler::Link*> &popplerLinks )
{
	QLinkedList<Okular::ObjectRect*> links;
	foreach(const Poppler::Link *popplerLink, popplerLinks)
	{
		QRectF linkArea = popplerLink->linkArea();
		double nl = linkArea.left(),
		       nt = linkArea.top(),
		       nr = linkArea.right(),
		       nb = linkArea.bottom();
		// create the rect using normalized coords and attach the Okular::Link to it
		Okular::ObjectRect * rect = new Okular::ObjectRect( nl, nt, nr, nb, false, Okular::ObjectRect::Action, createLinkFromPopplerLink(popplerLink) );
		// add the ObjectRect to the container
		links.push_front( rect );
	}
	qDeleteAll(popplerLinks);
	return links;
}

extern Okular::Annotation* createAnnotationFromPopplerAnnotation( Poppler::Annotation *ann, bool * doDelete );

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

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_poppler",
         "okular_poppler",
         ki18n( "PDF Backend" ),
         "0.4",
         ki18n( "A PDF file renderer" ),
         KAboutData::License_GPL,
         ki18n( "© 2005-2008 Albert Astals Cid" )
    );
    aboutData.addAuthor( ki18n( "Albert Astals Cid" ), KLocalizedString(), "aacid@kde.org" );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN(PDFGenerator, createAboutData())

#ifdef HAVE_POPPLER_0_16
static void PDFGeneratorPopplerDebugFunction(const QString &message, const QVariant &closure)
{
    Q_UNUSED(closure);
    kDebug() << "[Poppler]" << message;
}
#endif

PDFGenerator::PDFGenerator( QObject *parent, const QVariantList &args )
    : Generator( parent, args ), pdfdoc( 0 ), ready( true ),
    pixmapRequest( 0 ), docInfoDirty( true ), docSynopsisDirty( true ),
    docEmbeddedFilesDirty( true ), nextFontPage( 0 ),
    dpiX( 72.0 /*Okular::Utils::dpiX()*/ ), dpiY( 72.0 /*Okular::Utils::dpiY()*/ ),
    synctex_scanner(0)
{
    setFeature( TextExtraction );
    setFeature( FontInfo );
#ifdef Q_OS_WIN32
    setFeature( PrintNative );
#else
    setFeature( PrintPostscript );
#endif
    if ( Okular::FilePrinter::ps2pdfAvailable() )
        setFeature( PrintToFile );
    setFeature( ReadRawData );
    // generate the pixmapGeneratorThread
    generatorThread = new PDFPixmapGeneratorThread( this );
    connect(generatorThread, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
    
#ifdef HAVE_POPPLER_0_16
    // You only need to do it once not for each of the documents but it is cheap enough
    // so doing it all the time won't hurt either
    Poppler::setDebugErrorFunction(PDFGeneratorPopplerDebugFunction, QVariant());
#endif
}

PDFGenerator::~PDFGenerator()
{
    // stop and delete the generator thread
    if ( generatorThread )
    {
        generatorThread->wait();
        delete generatorThread;
    }

    delete pdfOptionsPage;
}

//BEGIN Generator inherited functions
bool PDFGenerator::loadDocument( const QString & filePath, QVector<Okular::Page*> & pagesVector )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kDebug(PDFDebug) << "PDFGenerator: multiple calls to loadDocument. Check it.";
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::load( filePath, 0, 0 );
    bool success = init(pagesVector, filePath.section('/', -1, -1));
    if (success)
    {
        // no need to check for the existence of a synctex file, no parser will be
        // created if none exists
        initSynctexParser(filePath);
        if ( !synctex_scanner && QFile::exists(filePath + QLatin1String( "sync" ) ) )
        {
            loadPdfSync(filePath, pagesVector);
        }
    }
    return success;
}

bool PDFGenerator::loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kDebug(PDFDebug) << "PDFGenerator: multiple calls to loadDocument. Check it.";
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::loadFromData( fileData, 0, 0 );
    return init(pagesVector, QString());
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
                parentwid = document()->widget()->effectiveWinId();
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
                prompt = i18n( "Please enter the password to read the document:" );
            else
                prompt = i18n( "Incorrect password. Try again:" );
            firstInput = false;

            // if the user presses cancel, abort opening
            KPasswordDialog dlg( document()->widget(), wallet ? KPasswordDialog::ShowKeepPassword : KPasswordDialog::KPasswordDialogFlags() );
            dlg.setCaption( i18n( "Document Password" ) );
            dlg.setPrompt( prompt );
            if( !dlg.exec() )
                break;
            password = dlg.password();
            if ( wallet )
                keep = dlg.keepPassword();
        }

        // 2. reopen the document using the password
        pdfdoc->unlock( password.toLatin1(), password.toLatin1() );

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

    // update the configuration
    reparseConfig();

    // the file has been loaded correctly
    return true;
}

bool PDFGenerator::doCloseDocument()
{
    // remove internal objects
    userMutex()->lock();
    delete pdfdoc;
    pdfdoc = 0;
    userMutex()->unlock();
    docInfoDirty = true;
    docSynopsisDirty = true;
    docSyn.clear();
    docEmbeddedFilesDirty = true;
    qDeleteAll(docEmbeddedFiles);
    docEmbeddedFiles.clear();
    nextFontPage = 0;
    rectsGenerated.clear();
    if ( synctex_scanner )
    {
        synctex_scanner_free( synctex_scanner );
        synctex_scanner = 0;
    }

    return true;
}

void PDFGenerator::loadPages(QVector<Okular::Page*> &pagesVector, int rotation, bool clear)
{
    // TODO XPDF 3.01 check
    const int count = pagesVector.count();
    double w = 0, h = 0;
    for ( int i = 0; i < count ; i++ )
    {
        // get xpdf page
        Poppler::Page * p = pdfdoc->page( i );
        Okular::Page * page;
        if (p)
        {
            const QSizeF pSize = p->pageSizeF();
            w = pSize.width() / 72.0 * dpiX;
            h = pSize.height() / 72.0 * dpiY;
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
            page = new Okular::Page( i, w, h, orientation );
            addTransition( p, page );
            if ( true ) //TODO real check
            addAnnotations( p, page );
            Poppler::Link * tmplink = p->action( Poppler::Page::Opening );
            if ( tmplink )
            {
                page->setPageAction( Okular::Page::Opening, createLinkFromPopplerLink( tmplink ) );
                delete tmplink;
            }
            tmplink = p->action( Poppler::Page::Closing );
            if ( tmplink )
            {
                page->setPageAction( Okular::Page::Closing, createLinkFromPopplerLink( tmplink ) );
                delete tmplink;
            }
            page->setDuration( p->duration() );
            page->setLabel( p->label() );

            addFormFields( p, page );
//        kWarning(PDFDebug).nospace() << page->width() << "x" << page->height();

#ifdef PDFGENERATOR_DEBUG
            kDebug(PDFDebug) << "load page" << i << "with rotation" << rotation << "and orientation" << orientation;
#endif
            delete p;

            if (clear && pagesVector[i])
                delete pagesVector[i];
        }
        else
        {
            page = new Okular::Page( i, defaultPageWidth, defaultPageHeight, Okular::Rotation0 );
        }
        // set the Okular::page at the right position in document's pages vector
        pagesVector[i] = page;
    }
}

const Okular::DocumentInfo * PDFGenerator::generateDocumentInfo()
{
    if ( docInfoDirty )
    {
        userMutex()->lock();
        
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
                         QString::number( pdfdoc->pdfVersion() ) ), i18n( "Format" ) );
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
        userMutex()->unlock();

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

    userMutex()->lock();
    QDomDocument *toc = pdfdoc->toc();
    userMutex()->unlock();
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
        case Poppler::FontInfo::unknown:
        default: ;
     }
     return Okular::FontInfo::Unknown;
}

static Okular::FontInfo::EmbedType embedTypeForPopplerFontInfo( const Poppler::FontInfo &fi )
{
    Okular::FontInfo::EmbedType ret = Okular::FontInfo::NotEmbedded;
    if ( fi.isEmbedded() )
    {
        if ( fi.isSubset() )
        {
            ret = Okular::FontInfo::EmbeddedSubset;
        }
        else
        {
            ret = Okular::FontInfo::FullyEmbedded;
        }
    }
     return ret;
}

Okular::FontInfo::List PDFGenerator::fontsForPage( int page )
{
    Okular::FontInfo::List list;

    if ( page != nextFontPage )
        return list;

    QList<Poppler::FontInfo> fonts;
    userMutex()->lock();
    pdfdoc->scanForFonts( 1, &fonts );
    userMutex()->unlock();

    foreach (const Poppler::FontInfo &font, fonts)
    {
        Okular::FontInfo of;
        of.setName( font.name() );
        of.setType( convertPopplerFontInfoTypeToOkularFontInfoType( font.type() ) );
        of.setEmbedType( embedTypeForPopplerFontInfo( font) );
        of.setFile( font.file() );
#ifdef HAVE_POPPLER_0_9
        of.setCanBeExtracted( of.embedType() != Okular::FontInfo::NotEmbedded );
        
        QVariant nativeId;
        nativeId.setValue( font );
        of.setNativeId( nativeId );
#endif

        list.append( of );
    }

    ++nextFontPage;

    return list;
}

const QList<Okular::EmbeddedFile*> *PDFGenerator::embeddedFiles() const
{
    if (docEmbeddedFilesDirty)
    {
        userMutex()->lock();
        const QList<Poppler::EmbeddedFile*> &popplerFiles = pdfdoc->embeddedFiles();
        foreach(Poppler::EmbeddedFile* pef, popplerFiles)
        {
            docEmbeddedFiles.append(new PDFEmbeddedFile(pef));
        }
        userMutex()->unlock();

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
        kDebug(PDFDebug) << "calling generatePixmap() when not in READY state!";
#endif
    // update busy state (not really needed here, because the flag needs to
    // be set only to prevent asking a pixmap while the thread is running)
    ready = false;

    // debug requests to this (xpdf) generator
    //kDebug(PDFDebug) << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "].";

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

    double fakeDpiX = request->width() * dpiX / pageWidth,
           fakeDpiY = request->height() * dpiY / pageHeight;

    // setup Okular:: output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasTextPage() && (request->width() == page->width()) &&
                       (request->height() == page->height());
    // generate links rects only the first time
    bool genObjectRects = !rectsGenerated.at( page->number() );

    // 0. LOCK [waits for the thread end]
    userMutex()->lock();

    // 1. Set OutputDev parameters and Generate contents
    // note: thread safety is set on 'false' for the GUI (this) thread
    Poppler::Page *p = pdfdoc->page(page->number());

    // 2. Take data from outputdev and attach it to the Page
    if (p)
    {
        QImage img( p->renderToImage(fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0 ) );
        if ( !page->isBoundingBoxKnown() )
            updatePageBoundingBox( page->number(), Okular::Utils::imageBoundingBox( &img ) );

        page->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( img ) ) );
    }
    else
    {
        QPixmap *dummyPixmap = new QPixmap( request->width(), request->height() );
        dummyPixmap->fill( Qt::white );
        page->setPixmap( request->id(), dummyPixmap );
    }

    if ( p && genObjectRects )
    {
    	// TODO previously we extracted Image type rects too, but that needed porting to poppler
        // and as we are not doing anything with Image type rects i did not port it, have a look at
        // dead gp_outputdev.cpp on image extraction
        page->setObjectRects( generateLinks(p->links()) );
        rectsGenerated[ request->page()->number() ] = true;
    }

    // 3. UNLOCK [re-enables shared access]
    userMutex()->unlock();
    if ( p && genTextPage )
    {
        QList<Poppler::TextBox*> textList = p->textList();
        const QSizeF s = p->pageSizeF();
        Okular::TextPage *tp = abstractTextPage(textList, s.height(), s.width(), request->page()->orientation());
        page->setTextPage( tp );
        qDeleteAll(textList);
        
        // notify the new generation
        signalTextGenerationDone( page, tp );
    }
    delete p;
    
    // update ready state
    ready = true;

    // notify the new generation
    signalPixmapRequestDone( request );
}

Okular::TextPage* PDFGenerator::textPage( Okular::Page *page )
{
#ifdef PDFGENERATOR_DEBUG
    kDebug(PDFDebug) << "page" << page->number();
#endif
    // build a TextList...
    QList<Poppler::TextBox*> textList;
    double pageWidth, pageHeight;
    Poppler::Page *pp = pdfdoc->page( page->number() );
    if (pp)
    {
        userMutex()->lock();
        textList = pp->textList();
        userMutex()->unlock();

        QSizeF s = pp->pageSizeF();
        pageWidth = s.width();
        pageHeight = s.height();

        delete pp;
    }
    else   
    {
        pageWidth = defaultPageWidth;
        pageHeight = defaultPageHeight;
    }

    Okular::TextPage *tp = abstractTextPage(textList, pageHeight, pageWidth, (Poppler::Page::Rotation)page->orientation());
    qDeleteAll(textList);
    return tp;
}

void PDFGenerator::requestFontData(const Okular::FontInfo &font, QByteArray *data)
{
#ifdef HAVE_POPPLER_0_9
    Poppler::FontInfo fi = font.nativeId().value<Poppler::FontInfo>();
    *data = pdfdoc->fontData(fi);
#else
    Q_UNUSED( font )
    Q_UNUSED( data )
#endif
}

#define DUMMY_QPRINTER_COPY
bool PDFGenerator::print( QPrinter& printer )
{
#ifdef Q_WS_WIN
    QPainter painter;
    painter.begin(&printer);

    QList<int> pageList = Okular::FilePrinter::pageList( printer, pdfdoc->numPages(),
                                                         document()->currentPage() + 1,
                                                         document()->bookmarkedPageList() );
    for ( int i = 0; i < pageList.count(); ++i )
    {
        if ( i != 0 )
            printer.newPage();

        const int page = pageList.at( i ) - 1;
        userMutex()->lock();
        Poppler::Page *pp = pdfdoc->page( page );
        if (pp)
        {
            QImage img = pp->renderToImage(  printer.physicalDpiX(), printer.physicalDpiY() );
            painter.drawImage( painter.window(), img, QRectF(0, 0, img.width(), img.height()) );
            delete pp;
        }
        userMutex()->unlock();
    }
    painter.end();
    return true;

#else
#ifdef DUMMY_QPRINTER_COPY
    // Get the real page size to pass to the ps generator
    QPrinter dummy( QPrinter::PrinterResolution );
    dummy.setFullPage( true );
    dummy.setOrientation( printer.orientation() );
    dummy.setPageSize( printer.pageSize() );
    dummy.setPaperSize( printer.paperSize( QPrinter::Millimeter ), QPrinter::Millimeter );
    int width = dummy.width();
    int height = dummy.height();
#else
    int width = printer.width();
    int height = printer.height();
#endif
    // Create the tempfile to send to FilePrinter, which will manage the deletion
    KTemporaryFile tf;
    tf.setSuffix( ".ps" );
    if ( !tf.open() )
    {
        lastPrintError = TemporaryFileOpenPrintError;
        return false;
    }
    QString tempfilename = tf.fileName();

    // Generate the list of pages to be printed as selected in the print dialog
    QList<int> pageList = Okular::FilePrinter::pageList( printer, pdfdoc->numPages(),
                                                         document()->currentPage() + 1,
                                                         document()->bookmarkedPageList() );

    // TODO rotation

    tf.setAutoRemove(false);

    QString pstitle = metaData(QLatin1String("Title"), QVariant()).toString();
    if ( pstitle.trimmed().isEmpty() )
    {
        pstitle = document()->currentDocument().fileName();
    }

    bool forceRasterize = false;
    if ( pdfOptionsPage )
    {
        forceRasterize = pdfOptionsPage->printForceRaster();
    }

    Poppler::PSConverter *psConverter = pdfdoc->psConverter();

    psConverter->setOutputDevice(&tf);

    psConverter->setPageList(pageList);
    psConverter->setPaperWidth(width);
    psConverter->setPaperHeight(height);
    psConverter->setRightMargin(0);
    psConverter->setBottomMargin(0);
    psConverter->setLeftMargin(0);
    psConverter->setTopMargin(0);
    psConverter->setStrictMargins(false);
    psConverter->setForceRasterize(forceRasterize);
    psConverter->setTitle(pstitle);

    userMutex()->lock();
    if (psConverter->convert())
    {
        userMutex()->unlock();
        delete psConverter;
        tf.close();
        int ret = Okular::FilePrinter::printFile( printer, tempfilename,
                                                  document()->orientation(),
                                                  Okular::FilePrinter::SystemDeletesFiles,
                                                  Okular::FilePrinter::ApplicationSelectsPages,
                                                  document()->bookmarkedPageRange() );

        lastPrintError = Okular::FilePrinter::printError( ret );

        return (lastPrintError == NoPrintError);
    }
    else
    {
        lastPrintError = FileConversionPrintError;
        delete psConverter;
        userMutex()->unlock();
    }

    tf.close();

    return false;
#endif
}

QVariant PDFGenerator::metaData( const QString & key, const QVariant & option ) const
{
    if ( key == "StartFullScreen" )
    {
        QMutexLocker ml(userMutex());
        // asking for the 'start in fullscreen mode' (pdf property)
        if ( pdfdoc->pageMode() == Poppler::Document::FullScreen )
            return true;
    }
    else if ( key == "NamedViewport" && !option.toString().isEmpty() )
    {
        Okular::DocumentViewport viewport;
        QString optionString = option.toString();

        // if option starts with "src:" assume that we are handling a 
        // source reference
        if ( optionString.startsWith( "src:", Qt::CaseInsensitive ) )
        {
            fillViewportFromSourceReference( viewport, optionString );
        }
        else
        {
            // asking for the page related to a 'named link destination'. the
            // option is the link name. @see addSynopsisChildren.
            userMutex()->lock();
            Poppler::LinkDestination *ld = pdfdoc->linkDestination( optionString );
            userMutex()->unlock();
            if ( ld )
            {
                fillViewportFromLinkDestination( viewport, *ld );
            }
            delete ld;
        }
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    else if ( key == "DocumentTitle" )
    {
        userMutex()->lock();
        QString title = pdfdoc->info( "Title" );
        userMutex()->unlock();
        return title;
    }
    else if ( key == "OpenTOC" )
    {
        QMutexLocker ml(userMutex());
        if ( pdfdoc->pageMode() == Poppler::Document::UseOutlines )
            return true;
    }
#ifdef HAVE_POPPLER_0_9
    else if ( key == "DocumentScripts" && option.toString() == "JavaScript" )
    {
        QMutexLocker ml(userMutex());
        return pdfdoc->scripts();
    }
#endif
    return QVariant();
}

bool PDFGenerator::reparseConfig()
{
    if ( !pdfdoc )
        return false;

    bool somethingchanged = false;
    // load paper color
    QColor color = documentMetaData( "PaperColor", true ).value< QColor >();
    // if paper color is changed we have to rebuild every visible pixmap in addition
    // to the outputDevice. it's the 'heaviest' case, other effect are just recoloring
    // over the page rendered on 'standard' white background.
    if ( color != pdfdoc->paperColor() )
    {
        userMutex()->lock();
        pdfdoc->setPaperColor(color);
        userMutex()->unlock();
        somethingchanged = true;
    }
    bool aaChanged = setDocumentRenderHints();
    somethingchanged = somethingchanged || aaChanged;
    return somethingchanged;
}

void PDFGenerator::addPages( KConfigDialog * )
{
}

bool PDFGenerator::setDocumentRenderHints()
{
    bool changed = false;
    static Poppler::Document::RenderHints oldhints = 0;
#define SET_HINT(hintname, hintdefvalue, hintflag) \
{ \
    bool newhint = documentMetaData(hintname, hintdefvalue).toBool(); \
    if (newhint != (oldhints & hintflag)) \
    { \
        pdfdoc->setRenderHint(hintflag, newhint); \
        if (newhint) \
            oldhints |= hintflag; \
        else \
            oldhints &= ~(int)hintflag; \
        changed = true; \
    } \
}
    SET_HINT("GraphicsAntialias", true, Poppler::Document::Antialiasing)
    SET_HINT("TextAntialias", true, Poppler::Document::TextAntialiasing)
#ifdef HAVE_POPPLER_0_12_1
    SET_HINT("TextHinting", false, Poppler::Document::TextHinting)
#endif
#undef SET_HINT
    return changed;
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
            QString text;
            userMutex()->lock();
            Poppler::Page *pp = pdfdoc->page(i);
            if (pp)
            {
                text = pp->text(QRect());
            }
            userMutex()->unlock();
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
//    kWarning(PDFDebug).nospace() << "text: " << s << " at (" << l << "," << t << ")x(" << r <<","<<b<<")";
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
    Q_UNUSED(rot);
    Okular::TextPage* ktp=new Okular::TextPage;
    Poppler::TextBox *next; 
#ifdef PDFGENERATOR_DEBUG
    kDebug(PDFDebug) << "getting text page in generator pdf - rotation:" << rot;
#endif
    int charCount=0;
    int j;
    QString s;
    foreach (Poppler::TextBox *word, text)
    {
        charCount=word->text().length();
        next=word->nextWord();
        for (j = 0; j < charCount; j++)
        {
            s = word->text().at(j);
            QRectF charBBox = word->charBoundingBox(j);
            append(ktp, (j==charCount-1 && !next ) ? (s + '\n') : s,
                charBBox.left()/width,
                charBBox.bottom()/height,
                charBBox.right()/width,
                charBBox.top()/height);
        }

        if ( word->hasSpaceAfter() && next )
        {
            // TODO Check with a document with vertical text
            // probably won't work and we will need to do comparisons
            // between wordBBox and nextWordBBox to see if they are
            // vertically or horizontally aligned
            QRectF wordBBox = word->boundingBox();
            QRectF nextWordBBox = next->boundingBox();
            append(ktp, " ", 
                     wordBBox.right()/width,
                     wordBBox.bottom()/height,
                     nextWordBBox.left()/width,
                     wordBBox.top()/height);
        }
    }
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
            fillViewportFromLinkDestination( vp, Poppler::LinkDestination(e.attribute("Destination")) );
            item.setAttribute( "Viewport", vp.toString() );
        }
        if (!e.attribute("Open").isNull()) item.setAttribute("Open", e.attribute("Open"));
        if (!e.attribute("DestinationURI").isNull()) item.setAttribute("URL", e.attribute("DestinationURI"));

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
        kDebug(PDFDebug) << "astario:    " << szanno; */
        //TODO add annotations after poppler write feather is full suported
        bool doDelete = true;
        Okular::Annotation * newann = createAnnotationFromPopplerAnnotation( a, &doDelete );
        if (newann)
        {
            // the Contents field has lines separated by \r
            QString contents = newann->contents();
            contents.replace( QLatin1Char( '\r' ), QLatin1Char( '\n' ) );
            newann->setContents( contents );
            // explicitly mark as external
            newann->setFlags( newann->flags() | Okular::Annotation::External );
            page->addAnnotation(newann);
        }
        if ( doDelete )
            delete a;
    }
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
    QList<Poppler::FormField*> popplerFormFields = popplerPage->formFields();
    QLinkedList<Okular::FormField*> okularFormFields;
    foreach( Poppler::FormField *f, popplerFormFields )
    {
        Okular::FormField * of = 0;
        switch ( f->type() )
        {
            case Poppler::FormField::FormButton:
                of = new PopplerFormFieldButton( static_cast<Poppler::FormFieldButton*>( f ) );
                break;
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
    // first row: core name of the pdf output
    const QString coreName = ts.readLine();
    // second row: version string, in the form 'Version %u'
    QString versionstr = ts.readLine();
    QRegExp versionre( "Version (\\d+)" );
    versionre.setCaseSensitivity( Qt::CaseInsensitive );
    if ( !versionre.exactMatch( versionstr ) )
        return;

    QHash<int, pdfsyncpoint> points;
    QStack<QString> fileStack;
    int currentpage = -1;
    const QLatin1String texStr( ".tex" );
    const QChar spaceChar = QChar::fromLatin1( ' ' );

    fileStack.push( coreName + texStr );

    QString line;
    while ( !ts.atEnd() )
    {
        line = ts.readLine();
        const QStringList tokens = line.split( spaceChar, QString::SkipEmptyParts );
        const int tokenSize = tokens.count();
        if ( tokenSize < 1 )
            continue;
        if ( tokens.first() == QLatin1String( "l" ) && tokenSize >= 3 )
        {
            int id = tokens.at( 1 ).toInt();
            QHash<int, pdfsyncpoint>::const_iterator it = points.constFind( id );
            if ( it == points.constEnd() )
            {
                pdfsyncpoint pt;
                pt.x = 0;
                pt.y = 0;
                pt.row = tokens.at( 2 ).toInt();
                pt.column = 0; // TODO
                pt.page = -1;
                pt.file = fileStack.top();
                points[ id ] = pt;
            }
        }
        else if ( tokens.first() == QLatin1String( "s" ) && tokenSize >= 2 )
        {
            currentpage = tokens.at( 1 ).toInt() - 1;
        }
        else if ( tokens.first() == QLatin1String( "p*" ) && tokenSize >= 4 )
        {
            // TODO
            kDebug(PDFDebug) << "PdfSync: 'p*' line ignored";
        }
        else if ( tokens.first() == QLatin1String( "p" ) && tokenSize >= 4 )
        {
            int id = tokens.at( 1 ).toInt();
            QHash<int, pdfsyncpoint>::iterator it = points.find( id );
            if ( it != points.end() )
            {
                it->x = tokens.at( 2 ).toInt();
                it->y = tokens.at( 3 ).toInt();
                it->page = currentpage;
            }
        }
        else if ( line.startsWith( QLatin1Char( '(' ) ) && tokenSize == 1 )
        {
            QString newfile = line;
            // chop the leading '('
            newfile.remove( 0, 1 );
            if ( !newfile.endsWith( texStr ) )
            {
                newfile += texStr;
            }
            fileStack.push( newfile );
        }
        else if ( line == QLatin1String( ")" ) )
        {
            if ( !fileStack.isEmpty() )
            {
                fileStack.pop();
            }
            else
                kDebug(PDFDebug) << "PdfSync: going one level down too much";
        }
        else
            kDebug(PDFDebug).nospace() << "PdfSync: unknown line format: '" << line << "'";

    }

    QVector< QLinkedList< Okular::SourceRefObjectRect * > > refRects( pagesVector.size() );
    foreach ( const pdfsyncpoint& pt, points )
    {
        // drop pdfsync points not completely valid
        if ( pt.page < 0 || pt.page >= pagesVector.size() )
            continue;

        // magic numbers for TeX's RSU's (Ridiculously Small Units) conversion to pixels
        Okular::NormalizedPoint p(
            ( pt.x * dpiX ) / ( 72.27 * 65536.0 * pagesVector[pt.page]->width() ),
            ( pt.y * dpiY ) / ( 72.27 * 65536.0 * pagesVector[pt.page]->height() )
            );
        QString file = pt.file;
        Okular::SourceReference * sourceRef = new Okular::SourceReference( file, pt.row, pt.column );
        refRects[ pt.page ].append( new Okular::SourceRefObjectRect( p, sourceRef ) );
    }
    for ( int i = 0; i < refRects.size(); ++i )
        if ( !refRects.at(i).isEmpty() )
            pagesVector[i]->setSourceReferences( refRects.at(i) );
}

void PDFGenerator::initSynctexParser( const QString& filePath )
{
    synctex_scanner = synctex_scanner_new_with_output_file( QFile::encodeName( filePath ), 0, 1);
}

const Okular::SourceReference * PDFGenerator::dynamicSourceReference( int pageNr, double absX, double absY )
{
    if  ( !synctex_scanner )
        return 0;

    if (synctex_edit_query(synctex_scanner, pageNr + 1, absX * 72. / dpiX, absY * 72. / dpiY) > 0)
    {
        synctex_node_t node;
        // TODO what should we do if there is really more than one node?
        while (( node = synctex_next_result( synctex_scanner ) ))
        {
            int line = synctex_node_line(node);
            int col = synctex_node_column(node);
            // column extraction does not seem to be implemented in synctex so far. set the SourceReference default value.
            if ( col == -1 )
            {
                col = 0;
            }
            const char *name = synctex_scanner_get_name( synctex_scanner, synctex_node_tag( node ) );

            Okular::SourceReference * sourceRef = new Okular::SourceReference( name, line, col );
            return sourceRef;
        }
    }
    return 0;
}

PDFGenerator::PrintError PDFGenerator::printError() const
{
    return lastPrintError;
}

void PDFGenerator::fillViewportFromSourceReference( Okular::DocumentViewport & viewport, const QString & reference ) const
{
    if ( !synctex_scanner )
        return;

    // The reference is of form "src:1111Filename", where "1111"
    // points to line number 1111 in the file "Filename".
    // Extract the file name and the numeral part from the reference string.
    // This will fail if Filename starts with a digit.
    QString name, lineString;
    // Remove "src:". Presence of substring has been checked before this 
    // function is called.
    name = reference.mid( 4 );
    // split
    int nameLength = name.length();
    int i = 0;
    for( i = 0; i < nameLength; ++i )
    {
        if ( !name[i].isDigit() ) break;
    }
    lineString = name.left( i );
    name = name.mid( i );
    // Remove spaces.
    name = name.trimmed();
    lineString = lineString.trimmed();
    // Convert line to integer. 
    bool ok;
    int line = lineString.toInt( &ok );
    if (!ok) line = -1;
    
    // Use column == -1 for now.
    if( synctex_display_query( synctex_scanner, name.toLatin1(), line, -1 ) > 0 )
    {
        synctex_node_t node;
        // For now use the first hit. Could possibly be made smarter
        // in case there are multiple hits.
        while( ( node = synctex_next_result( synctex_scanner ) ) ) 
        {
            // TeX pages start at 1.
            viewport.pageNumber = synctex_node_page( node ) - 1;

            if ( !viewport.isValid() ) return;

            // TeX small points ...
            double px = (synctex_node_visible_h( node ) * dpiX) / 72.27;
            double py = (synctex_node_visible_v( node ) * dpiY) / 72.27;
            viewport.rePos.normalizedX = px / document()->page(viewport.pageNumber)->width();
            viewport.rePos.normalizedY = ( py + 0.5 ) / document()->page(viewport.pageNumber)->height();
            viewport.rePos.enabled = true;
            viewport.rePos.pos = Okular::DocumentViewport::Center;
           
            return;
        }
    }
}

QWidget* PDFGenerator::printConfigurationWidget() const
{
    if ( !pdfOptionsPage )
    {
        const_cast<PDFGenerator*>(this)->pdfOptionsPage = new PDFOptionsPage();
    }
    return pdfOptionsPage;
}

bool PDFGenerator::supportsOption( SaveOption option ) const
{
    switch ( option )
    {
        case SaveChanges:
            return true;
        default: ;
    }
    return false;
}

bool PDFGenerator::save( const QString &fileName, SaveOptions options, QString *errorText )
{
    Poppler::PDFConverter *pdfConv = pdfdoc->pdfConverter();

    pdfConv->setOutputFileName( fileName );
    if ( options & SaveChanges )
        pdfConv->setPDFOptions( pdfConv->pdfOptions() | Poppler::PDFConverter::WithChanges );

    QMutexLocker locker( userMutex() );
    bool success = pdfConv->convert();
#ifdef HAVE_POPPLER_0_12_1
    if (!success)
    {
        switch (pdfConv->lastError())
        {
            case Poppler::BaseConverter::NotSupportedInputFileError:
                *errorText = i18n("Saving files with /Encrypt is not supported.");
            break;

            case Poppler::BaseConverter::NoError:
            case Poppler::BaseConverter::FileLockedError:
                // we can't get here
            break;

            case Poppler::BaseConverter::OpenOutputError:
                // the default text message is good for this case
            break;
        }
    }
#endif
    delete pdfConv;
    return success;
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
                        << "signal but had problems ending.";
            return;
        }
}
#endif

    // 1. the mutex must be unlocked now
    bool isLocked = true;
    if (userMutex()->tryLock()) {
        userMutex()->unlock();
        isLocked = false;
    }
    if ( isLocked )
    {
        kWarning(PDFDebug) << "PDFGenerator: 'data available' but mutex still "
                    << "held. Recovering.";
        // synchronize GUI thread (must not happen)
        userMutex()->lock();
        userMutex()->unlock();
    }

    // 2. put thread's generated data into the Okular::Page
    Okular::PixmapRequest * request = generatorThread->request();
    QImage * outImage = generatorThread->takeImage();
    QList<Poppler::TextBox*> outText = generatorThread->takeText();
    QLinkedList< Okular::ObjectRect * > outRects = generatorThread->takeObjectRects();

    if ( !request->page()->isBoundingBoxKnown() )
        updatePageBoundingBox( request->page()->number(), Okular::Utils::imageBoundingBox( outImage ) );
    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( *outImage ) ) );
    delete outImage;
    if ( !outText.isEmpty() )
    {
        Poppler::Page *pp = pdfdoc->page( request->page()->number() );
        const QSizeF s = pp->pageSizeF();
        Okular::TextPage *tp = abstractTextPage( outText, s.height(), 
                                                 s.width(),request->page()->orientation());
        request->page()->setTextPage( tp );
        qDeleteAll(outText);
        delete pp;
        
        // notify the new generation
        signalTextGenerationDone( request->page(), tp );
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
                  << "when another is being generated.";
        delete request;
        return;
    }

    // check if the mutex is already held
    bool isLocked = true;
    if (d->generator->userMutex()->tryLock()) {
        d->generator->userMutex()->unlock();
        isLocked = false;
    }
    if ( isLocked )
    {
        kDebug(PDFDebug) << "PDFPixmapGeneratorThread: requesting a pixmap "
                  << "with the mutex already held.";
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
                  << "but generation was not started.";
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

    // setup Okular:: output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasTextPage() &&
                       ( width == page->width() ) &&
                       ( height == page->height() );

    // generate links rects only the first time
    bool genObjectRects = !d->generator->rectsGenerated.at( page->number() );

    // 0. LOCK s[tart locking XPDF thread unsafe classes]
    d->generator->userMutex()->lock();

    // 1. set OutputDev parameters and Generate contents
    Poppler::Page *pp = d->generator->pdfdoc->page( page->number() );
    
    if (pp)
    {
        double fakeDpiX = width * d->generator->dpiX / pageWidth,
            fakeDpiY = height * d->generator->dpiY / pageHeight;

        // 2. grab data from the OutputDev and store it locally (note takeIMAGE)
#ifndef NDEBUG
        if ( d->m_image )
            kDebug(PDFDebug) << "PDFPixmapGeneratorThread: previous image not taken";
        if ( !d->m_textList.isEmpty() )
            kDebug(PDFDebug) << "PDFPixmapGeneratorThread: previous text not taken";
#endif
        d->m_image = new QImage( pp->renderToImage( fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0 ) );
        
        if ( genObjectRects )
        {
            d->m_rects = generateLinks(pp->links());
        }
        else d->m_rectsTaken = false;

        if ( genTextPage )
        {
            d->m_textList = pp->textList();
        }
        delete pp;
    }
    else
    {
        d->m_image = new QImage( width, height, QImage::Format_ARGB32 );
        d->m_image->fill( Qt::white );
    }
    
    // 3. [UNLOCK] mutex
    d->generator->userMutex()->unlock();

    // by ending the thread notifies the GUI thread that data is pending and can be read
}

#include "generator_pdf.moc"

/* kate: replace-tabs on; indent-width 4; */
