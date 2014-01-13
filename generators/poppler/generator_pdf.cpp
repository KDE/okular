/***************************************************************************
 *   Copyright (C) 2004-2008 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2012 by Guillermo A. Amaral B. <gamaral@kde.org>        *
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
#include <kconfigdialog.h>
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

#include "ui_pdfsettingswidget.h"
#include "pdfsettings.h"

#include <config-okular-poppler.h>

#ifdef HAVE_POPPLER_0_20
#  include <poppler-media.h>
#endif

#include "annots.h"
#include "formfields.h"
#include "popplerembeddedfile.h"

Q_DECLARE_METATYPE(Poppler::Annotation*)
Q_DECLARE_METATYPE(Poppler::FontInfo)
#ifdef HAVE_POPPLER_0_20
Q_DECLARE_METATYPE(const Poppler::LinkMovie*)
#endif
#ifdef HAVE_POPPLER_0_22
Q_DECLARE_METATYPE(const Poppler::LinkRendition*)
#endif

static const int defaultPageWidth = 595;
static const int defaultPageHeight = 842;

class PDFOptionsPage : public QWidget
{
   public:
       PDFOptionsPage()
       {
           setWindowTitle( i18n( "PDF Options" ) );
           QVBoxLayout *layout = new QVBoxLayout(this);
           m_printAnnots = new QCheckBox(i18n("Print annotations"), this);
           m_printAnnots->setToolTip(i18n("Include annotations in the printed document"));
           m_printAnnots->setWhatsThis(i18n("Includes annotations in the printed document. You can disable this if you want to print the original unannotated document."));
           layout->addWidget(m_printAnnots);
           m_forceRaster = new QCheckBox(i18n("Force rasterization"), this);
           m_forceRaster->setToolTip(i18n("Rasterize into an image before printing"));
           m_forceRaster->setWhatsThis(i18n("Forces the rasterization of each page into an image before printing it. This usually gives somewhat worse results, but is useful when printing documents that appear to print incorrectly."));
           layout->addWidget(m_forceRaster);
           layout->addStretch(1);

#if defined(Q_WS_WIN) || !defined(HAVE_POPPLER_0_20)
           m_printAnnots->setVisible( false );
#endif
           setPrintAnnots( true ); // Default value
       }

       bool printAnnots()
       {
           return m_printAnnots->isChecked();
       }

       void setPrintAnnots( bool printAnnots )
       {
           m_printAnnots->setChecked( printAnnots );
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
        QCheckBox *m_printAnnots;
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

Okular::Movie* createMovieFromPopplerMovie( const Poppler::MovieObject *popplerMovie )
{
    Okular::Movie *movie = new Okular::Movie( popplerMovie->url() );
    movie->setSize( popplerMovie->size() );
    movie->setRotation( (Okular::Rotation)( popplerMovie->rotation() / 90 ) );
    movie->setShowControls( popplerMovie->showControls() );
    movie->setPlayMode( (Okular::Movie::PlayMode)popplerMovie->playMode() );
    movie->setAutoPlay( false ); // will be triggered by external MovieAnnotation
#ifdef HAVE_POPPLER_0_22
    movie->setShowPosterImage( popplerMovie->showPosterImage() );
    movie->setPosterImage( popplerMovie->posterImage() );
#endif
    return movie;
}

#ifdef HAVE_POPPLER_0_20
Okular::Movie* createMovieFromPopplerScreen( const Poppler::LinkRendition *popplerScreen )
{
    Poppler::MediaRendition *rendition = popplerScreen->rendition();
    Okular::Movie *movie = 0;
    if ( rendition->isEmbedded() )
        movie = new Okular::Movie( rendition->fileName(), rendition->data() );
    else
        movie = new Okular::Movie( rendition->fileName() );
    movie->setSize( rendition->size() );
    movie->setShowControls( rendition->showControls() );
    movie->setPlayMode( Okular::Movie::PlayOnce );
    movie->setAutoPlay( rendition->autoPlay() );
    return movie;
}
#endif

/**
 * Note: the function will take ownership of the popplerLink object.
 */
Okular::Action* createLinkFromPopplerLink(const Poppler::Link *popplerLink)
{
    Okular::Action *link = 0;
    const Poppler::LinkGoto *popplerLinkGoto;
    const Poppler::LinkExecute *popplerLinkExecute;
    const Poppler::LinkBrowse *popplerLinkBrowse;
    const Poppler::LinkAction *popplerLinkAction;
    const Poppler::LinkSound *popplerLinkSound;
    const Poppler::LinkJavaScript *popplerLinkJS;
#ifdef HAVE_POPPLER_0_20
    const Poppler::LinkMovie *popplerLinkMovie;
#endif
#ifdef HAVE_POPPLER_0_22
    const Poppler::LinkRendition *popplerLinkRendition;
#endif
    Okular::DocumentViewport viewport;

    bool deletePopplerLink = true;

    switch(popplerLink->linkType())
    {
        case Poppler::Link::None:
        break;

        case Poppler::Link::Goto:
        {
            popplerLinkGoto = static_cast<const Poppler::LinkGoto *>(popplerLink);
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

        case Poppler::Link::JavaScript:
        {
            popplerLinkJS = static_cast<const Poppler::LinkJavaScript *>(popplerLink);
            link = new Okular::ScriptAction( Okular::JavaScript, popplerLinkJS->script() );
        }
        break;

#ifdef HAVE_POPPLER_0_22
        case Poppler::Link::Rendition:
        {
            deletePopplerLink = false; // we'll delete it inside resolveMediaLinkReferences() after we have resolved all references

            popplerLinkRendition = static_cast<const Poppler::LinkRendition *>( popplerLink );

            Okular::RenditionAction::OperationType operation = Okular::RenditionAction::None;
            switch ( popplerLinkRendition->action() )
            {
                case Poppler::LinkRendition::NoRendition:
                    operation = Okular::RenditionAction::None;
                    break;
                case Poppler::LinkRendition::PlayRendition:
                    operation = Okular::RenditionAction::Play;
                    break;
                case Poppler::LinkRendition::StopRendition:
                    operation = Okular::RenditionAction::Stop;
                    break;
                case Poppler::LinkRendition::PauseRendition:
                    operation = Okular::RenditionAction::Pause;
                    break;
                case Poppler::LinkRendition::ResumeRendition:
                    operation = Okular::RenditionAction::Resume;
                    break;
            };

            Okular::Movie *movie = 0;
            if ( popplerLinkRendition->rendition() )
                movie = createMovieFromPopplerScreen( popplerLinkRendition );

            Okular::RenditionAction *renditionAction = new Okular::RenditionAction( operation, movie, Okular::JavaScript, popplerLinkRendition->script() );
            renditionAction->setNativeId( QVariant::fromValue( popplerLinkRendition ) );
            link = renditionAction;
        }
        break;
#endif

#ifdef HAVE_POPPLER_0_20
        case Poppler::Link::Movie:
        {
            deletePopplerLink = false; // we'll delete it inside resolveMediaLinkReferences() after we have resolved all references

            popplerLinkMovie = static_cast<const Poppler::LinkMovie *>( popplerLink );

            Okular::MovieAction::OperationType operation = Okular::MovieAction::Play;
            switch ( popplerLinkMovie->operation() )
            {
                case Poppler::LinkMovie::Play:
                    operation = Okular::MovieAction::Play;
                    break;
                case Poppler::LinkMovie::Stop:
                    operation = Okular::MovieAction::Stop;
                    break;
                case Poppler::LinkMovie::Pause:
                    operation = Okular::MovieAction::Pause;
                    break;
                case Poppler::LinkMovie::Resume:
                    operation = Okular::MovieAction::Resume;
                    break;
            };

            Okular::MovieAction *movieAction = new Okular::MovieAction( operation );
            movieAction->setNativeId( QVariant::fromValue( popplerLinkMovie ) );
            link = movieAction;
        }
        break;
#endif
    }

    if ( deletePopplerLink )
        delete popplerLink;

    return link;
}

/**
 * Note: the function will take ownership of the popplerLink objects.
 */
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

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_poppler",
         "okular_poppler",
         ki18n( "PDF Backend" ),
         "0.6.3",
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
    : Generator( parent, args ), pdfdoc( 0 ),
    docInfoDirty( true ), docSynopsisDirty( true ),
    docEmbeddedFilesDirty( true ), nextFontPage( 0 ),
    annotProxy( 0 ), synctex_scanner( 0 )
{
    setFeature( Threaded );
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
    setFeature( TiledRendering );

#ifdef HAVE_POPPLER_0_16
    // You only need to do it once not for each of the documents but it is cheap enough
    // so doing it all the time won't hurt either
    Poppler::setDebugErrorFunction(PDFGeneratorPopplerDebugFunction, QVariant());
#endif
}

PDFGenerator::~PDFGenerator()
{
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
    int pageCount = pdfdoc->numPages();
    if (pageCount < 0) {
        delete pdfdoc;
        pdfdoc = 0;
        return false;
    }
    pagesVector.resize(pageCount);
    rectsGenerated.fill(false, pageCount);

    annotationsHash.clear();

    loadPages(pagesVector, 0, false);

    // update the configuration
    reparseConfig();

    // create annotation proxy
    annotProxy = new PopplerAnnotationProxy( pdfdoc, userMutex() );

    // the file has been loaded correctly
    return true;
}

bool PDFGenerator::doCloseDocument()
{
    // remove internal objects
    userMutex()->lock();
    delete annotProxy;
    annotProxy = 0;
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
            w = pSize.width() / 72.0 * dpi().width();
            h = pSize.height() / 72.0 * dpi().height();
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
            }
            tmplink = p->action( Poppler::Page::Closing );
            if ( tmplink )
            {
                page->setPageAction( Okular::Page::Closing, createLinkFromPopplerLink( tmplink ) );
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

            int major, minor;
            pdfdoc->getPdfVersion(&major, &minor);
            docInfo.set( "format", i18nc( "PDF v. <version>", "PDF v. %1.%2",
                         major, minor ), i18n( "Format" ) );
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
        of.setCanBeExtracted( of.embedType() != Okular::FontInfo::NotEmbedded );

        QVariant nativeId;
        nativeId.setValue( font );
        of.setNativeId( nativeId );

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

QImage PDFGenerator::image( Okular::PixmapRequest * request )
{
    // debug requests to this (xpdf) generator
    //kDebug(PDFDebug) << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "].";

    // compute dpi used to get an image with desired width and height
    Okular::Page * page = request->page();

    double pageWidth = page->width(),
           pageHeight = page->height();

    if ( page->rotation() % 2 )
        qSwap( pageWidth, pageHeight );

    qreal fakeDpiX = request->width() / pageWidth * dpi().width();
    qreal fakeDpiY = request->height() / pageHeight * dpi().height();

    // generate links rects only the first time
    bool genObjectRects = !rectsGenerated.at( page->number() );

    // 0. LOCK [waits for the thread end]
    userMutex()->lock();

    // 1. Set OutputDev parameters and Generate contents
    // note: thread safety is set on 'false' for the GUI (this) thread
    Poppler::Page *p = pdfdoc->page(page->number());

    // 2. Take data from outputdev and attach it to the Page
    QImage img;
    if (p)
    {
        if ( request->isTile() )
        {
            QRect rect = request->normalizedRect().geometry( request->width(), request->height() );
            img = p->renderToImage( fakeDpiX, fakeDpiY, rect.x(), rect.y(), rect.width(), rect.height(), Poppler::Page::Rotate0 );
        }
        else
        {
            img = p->renderToImage(fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0 );
        }
    }
    else
    {
        img = QImage( request->width(), request->height(), QImage::Format_Mono );
        img.fill( Qt::white );
    }

    if ( p && genObjectRects )
    {
        // TODO previously we extracted Image type rects too, but that needed porting to poppler
        // and as we are not doing anything with Image type rects i did not port it, have a look at
        // dead gp_outputdev.cpp on image extraction
        page->setObjectRects( generateLinks(p->links()) );
        rectsGenerated[ request->page()->number() ] = true;

        resolveMediaLinkReferences( page );
    }

    // 3. UNLOCK [re-enables shared access]
    userMutex()->unlock();

    delete p;

    return img;
}

template <typename PopplerLinkType, typename OkularLinkType, typename PopplerAnnotationType, typename OkularAnnotationType>
void resolveMediaLinks( Okular::Action *action, enum Okular::Annotation::SubType subType, QHash<Okular::Annotation*, Poppler::Annotation*> &annotationsHash )
{
    OkularLinkType *okularAction = static_cast<OkularLinkType*>( action );

    const PopplerLinkType *popplerLink = action->nativeId().value<const PopplerLinkType*>();

    QHashIterator<Okular::Annotation*, Poppler::Annotation*> it( annotationsHash );
    while ( it.hasNext() )
    {
        it.next();

        if ( it.key()->subType() == subType )
        {
            const PopplerAnnotationType *popplerAnnotation = static_cast<const PopplerAnnotationType*>( it.value() );

            if ( popplerLink->isReferencedAnnotation( popplerAnnotation ) )
            {
                okularAction->setAnnotation( static_cast<OkularAnnotationType*>( it.key() ) );
                okularAction->setNativeId( QVariant() );
                delete popplerLink; // delete the associated Poppler::LinkMovie object, it's not needed anymore
                break;
            }
        }
    }
}

void PDFGenerator::resolveMediaLinkReference( Okular::Action *action )
{
#ifdef HAVE_POPPLER_0_20
    if ( !action )
        return;

#ifdef HAVE_POPPLER_0_22
    if ( (action->actionType() != Okular::Action::Movie) && (action->actionType() != Okular::Action::Rendition) )
        return;

    resolveMediaLinks<Poppler::LinkMovie, Okular::MovieAction, Poppler::MovieAnnotation, Okular::MovieAnnotation>( action, Okular::Annotation::AMovie, annotationsHash );
    resolveMediaLinks<Poppler::LinkRendition, Okular::RenditionAction, Poppler::ScreenAnnotation, Okular::ScreenAnnotation>( action, Okular::Annotation::AScreen, annotationsHash );
#else
    if ( action->actionType() != Okular::Action::Movie )
        return;

    resolveMediaLinks<Poppler::LinkMovie, Okular::MovieAction, Poppler::MovieAnnotation, Okular::MovieAnnotation>( action, Okular::Annotation::AMovie, annotationsHash );
#endif

#endif
}

void PDFGenerator::resolveMediaLinkReferences( Okular::Page *page )
{
    resolveMediaLinkReference( const_cast<Okular::Action*>( page->pageAction( Okular::Page::Opening ) ) );
    resolveMediaLinkReference( const_cast<Okular::Action*>( page->pageAction( Okular::Page::Closing ) ) );

    foreach ( Okular::Annotation *annotation, page->annotations() )
    {
        if ( annotation->subType() == Okular::Annotation::AScreen )
        {
            Okular::ScreenAnnotation *screenAnnotation = static_cast<Okular::ScreenAnnotation*>( annotation );
            resolveMediaLinkReference( screenAnnotation->additionalAction( Okular::Annotation::PageOpening ) );
            resolveMediaLinkReference( screenAnnotation->additionalAction( Okular::Annotation::PageClosing ) );
        }

        if ( annotation->subType() == Okular::Annotation::AWidget )
        {
            Okular::WidgetAnnotation *widgetAnnotation = static_cast<Okular::WidgetAnnotation*>( annotation );
            resolveMediaLinkReference( widgetAnnotation->additionalAction( Okular::Annotation::PageOpening ) );
            resolveMediaLinkReference( widgetAnnotation->additionalAction( Okular::Annotation::PageClosing ) );
        }
    }

    foreach ( Okular::FormField *field, page->formFields() )
        resolveMediaLinkReference( field->activationAction() );
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
    Poppler::FontInfo fi = font.nativeId().value<Poppler::FontInfo>();
    *data = pdfdoc->fontData(fi);
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

    if (width <= 0 || height <= 0)
    {
        lastPrintError = InvalidPageSizePrintError;
        return false;
    }

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

    bool printAnnots = true;
    bool forceRasterize = false;
    if ( pdfOptionsPage )
    {
        printAnnots = pdfOptionsPage->printAnnots();
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

#ifdef HAVE_POPPLER_0_20
    if (!printAnnots)
        psConverter->setPSOptions(psConverter->psOptions() | Poppler::PSConverter::HideAnnotations );
#endif

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
    else if ( key == "DocumentScripts" && option.toString() == "JavaScript" )
    {
        QMutexLocker ml(userMutex());
        return pdfdoc->scripts();
    }
    else if ( key == "HasUnsupportedXfaForm" )
    {
#ifdef HAVE_POPPLER_0_22
        QMutexLocker ml(userMutex());
        return pdfdoc->formType() == Poppler::Document::XfaForm;
#else
        return false;
#endif
    }
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

void PDFGenerator::addPages( KConfigDialog *dlg )
{
#ifdef HAVE_POPPLER_0_24
    Ui_PDFSettingsWidget pdfsw;
    QWidget* w = new QWidget(dlg);
    pdfsw.setupUi(w);
    dlg->addPage(w, PDFSettings::self(), i18n("PDF"), "application-pdf", i18n("PDF Backend Configuration") );
#endif
}

bool PDFGenerator::setDocumentRenderHints()
{
    bool changed = false;
    const Poppler::Document::RenderHints oldhints = pdfdoc->renderHints();
#define SET_HINT(hintname, hintdefvalue, hintflag) \
{ \
    bool newhint = documentMetaData(hintname, hintdefvalue).toBool(); \
    if (newhint != oldhints.testFlag(hintflag)) \
    { \
        pdfdoc->setRenderHint(hintflag, newhint); \
        changed = true; \
    } \
}
    SET_HINT("GraphicsAntialias", true, Poppler::Document::Antialiasing)
    SET_HINT("TextAntialias", true, Poppler::Document::TextAntialiasing)
#ifdef HAVE_POPPLER_0_12_1
    SET_HINT("TextHinting", false, Poppler::Document::TextHinting)
#endif
#undef SET_HINT
#ifdef HAVE_POPPLER_0_24
    // load thin line mode
    const int thinLineMode = PDFSettings::enhanceThinLines();
    const bool enableThinLineSolid = thinLineMode == PDFSettings::EnumEnhanceThinLines::Solid;
    const bool enableShapeLineSolid = thinLineMode == PDFSettings::EnumEnhanceThinLines::Shape;
    const bool thinLineSolidWasEnabled = (oldhints & Poppler::Document::ThinLineSolid) == Poppler::Document::ThinLineSolid;
    const bool thinLineShapeWasEnabled = (oldhints & Poppler::Document::ThinLineShape) == Poppler::Document::ThinLineShape;
    if (enableThinLineSolid != thinLineSolidWasEnabled) {
      pdfdoc->setRenderHint(Poppler::Document::ThinLineSolid, enableThinLineSolid);
      changed = true;
    }
    if (enableShapeLineSolid != thinLineShapeWasEnabled) {
      pdfdoc->setRenderHint(Poppler::Document::ThinLineShape, enableShapeLineSolid);
      changed = true;
    }
#endif
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
                text = pp->text(QRect()).normalized(QString::NormalizationForm_KC);
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
    ktp->append(s, new Okular::NormalizedRect(l, t, r, b));
}

Okular::TextPage * PDFGenerator::abstractTextPage(const QList<Poppler::TextBox*> &text, double height, double width,int rot)
{
    Q_UNUSED(rot);
    Okular::TextPage* ktp=new Okular::TextPage;
    Poppler::TextBox *next;
#ifdef PDFGENERATOR_DEBUG
    kDebug(PDFDebug) << "getting text page in generator pdf - rotation:" << rot;
#endif
    QString s;
    bool addChar;
    foreach (Poppler::TextBox *word, text)
    {
        const int qstringCharCount = word->text().length();
        next=word->nextWord();
        int textBoxChar = 0;
        for (int j = 0; j < qstringCharCount; j++)
        {
            const QChar c = word->text().at(j);
            if (c.isHighSurrogate())
            {
                s = c;
                addChar = false;
            }
            else if (c.isLowSurrogate())
            {
                s += c;
                addChar = true;
            }
            else
            {
                s = c;
                addChar = true;
            }

            if (addChar)
            {
                QRectF charBBox = word->charBoundingBox(textBoxChar);
                append(ktp, (j==qstringCharCount-1 && !next) ? (s + "\n") : s,
                    charBBox.left()/width,
                    charBBox.bottom()/height,
                    charBBox.right()/width,
                    charBBox.top()/height);
                textBoxChar++;
            }
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
        if ( e.hasChildNodes() ) addSynopsisChildren( &n, &    item );
        n = n.nextSibling();
    }
}

void PDFGenerator::addAnnotations( Poppler::Page * popplerPage, Okular::Page * page )
{
    QList<Poppler::Annotation*> popplerAnnotations = popplerPage->annotations();

    foreach(Poppler::Annotation *a, popplerAnnotations)
    {
        bool doDelete = true;
        Okular::Annotation * newann = createAnnotationFromPopplerAnnotation( a, &doDelete );
        if (newann)
        {
            page->addAnnotation(newann);

#ifdef HAVE_POPPLER_0_22
            if ( a->subType() == Poppler::Annotation::AScreen )
            {
                Poppler::ScreenAnnotation *annotScreen = static_cast<Poppler::ScreenAnnotation*>( a );
                Okular::ScreenAnnotation *screenAnnotation = static_cast<Okular::ScreenAnnotation*>( newann );

                // The activation action
                const Poppler::Link *actionLink = annotScreen->action();
                if ( actionLink )
                    screenAnnotation->setAction( createLinkFromPopplerLink( actionLink ) );

                // The additional actions
                const Poppler::Link *pageOpeningLink = annotScreen->additionalAction( Poppler::Annotation::PageOpeningAction );
                if ( pageOpeningLink )
                    screenAnnotation->setAdditionalAction( Okular::Annotation::PageOpening, createLinkFromPopplerLink( pageOpeningLink ) );

                const Poppler::Link *pageClosingLink = annotScreen->additionalAction( Poppler::Annotation::PageClosingAction );
                if ( pageClosingLink )
                    screenAnnotation->setAdditionalAction( Okular::Annotation::PageClosing, createLinkFromPopplerLink( pageClosingLink ) );
            }

            if ( a->subType() == Poppler::Annotation::AWidget )
            {
                Poppler::WidgetAnnotation *annotWidget = static_cast<Poppler::WidgetAnnotation*>( a );
                Okular::WidgetAnnotation *widgetAnnotation = static_cast<Okular::WidgetAnnotation*>( newann );

                // The additional actions
                const Poppler::Link *pageOpeningLink = annotWidget->additionalAction( Poppler::Annotation::PageOpeningAction );
                if ( pageOpeningLink )
                    widgetAnnotation->setAdditionalAction( Okular::Annotation::PageOpening, createLinkFromPopplerLink( pageOpeningLink ) );

                const Poppler::Link *pageClosingLink = annotWidget->additionalAction( Poppler::Annotation::PageClosingAction );
                if ( pageClosingLink )
                    widgetAnnotation->setAdditionalAction( Okular::Annotation::PageClosing, createLinkFromPopplerLink( pageClosingLink ) );
            }
#endif

            if ( !doDelete )
                annotationsHash.insert( newann, a );
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
            ( pt.x * dpi().width() ) / ( 72.27 * 65536.0 * pagesVector[pt.page]->width() ),
            ( pt.y * dpi().height() ) / ( 72.27 * 65536.0 * pagesVector[pt.page]->height() )
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

    if (synctex_edit_query(synctex_scanner, pageNr + 1, absX * 72. / dpi().width(), absY * 72. / dpi().height()) > 0)
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

            Okular::SourceReference * sourceRef = new Okular::SourceReference( QFile::decodeName( name ), line, col );
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
    if( synctex_display_query( synctex_scanner, QFile::encodeName(name), line, -1 ) > 0 )
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
            double px = (synctex_node_visible_h( node ) * dpi().width()) / 72.27;
            double py = (synctex_node_visible_v( node ) * dpi().height()) / 72.27;
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
        {
            // Saving files with /Encrypt is not supported before Poppler 0.22
#ifndef HAVE_POPPLER_0_22
            QMutexLocker locker( userMutex() );
            return pdfdoc->isEncrypted() ? false : true;
#else
            return true;
#endif
        }
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
#ifndef HAVE_POPPLER_0_22
                // This can only happen with Poppler before 0.22
                *errorText = i18n("Saving files with /Encrypt is not supported.");
#endif
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

Okular::AnnotationProxy* PDFGenerator::annotationProxy() const
{
    return annotProxy;
}

#include "generator_pdf.moc"

/* kate: replace-tabs on; indent-width 4; */
