/***************************************************************************
 *   Copyright (C) 2004-2008 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2012 by Guillermo A. Amaral B. <gamaral@kde.org>        *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
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
#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qlayout.h>
#include <qmutex.h>
#include <qregexp.h>
#include <qstack.h>
#include <qtemporaryfile.h>
#include <qtextstream.h>
#include <QPrinter>
#include <QPainter>
#include <QTimer>
#include <QtCore/QDebug>

#include <KAboutData>
#include <kconfigdialog.h>
#include <KLocalizedString>
#include <kmessagebox.h>

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

#include <poppler-media.h>

#include "debug_pdf.h"
#include "annots.h"
#include "formfields.h"
#include "popplerembeddedfile.h"

Q_DECLARE_METATYPE(Poppler::Annotation*)
Q_DECLARE_METATYPE(Poppler::FontInfo)
Q_DECLARE_METATYPE(const Poppler::LinkMovie*)
Q_DECLARE_METATYPE(const Poppler::LinkRendition*)
#ifdef HAVE_POPPLER_0_50
Q_DECLARE_METATYPE(const Poppler::LinkOCGState*)
#endif

static const int defaultPageWidth = 595;
static const int defaultPageHeight = 842;

class PDFOptionsPage : public QWidget
{
    Q_OBJECT

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

#if defined(Q_OS_WIN) && !defined HAVE_POPPLER_0_60
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
    movie->setShowPosterImage( popplerMovie->showPosterImage() );
    movie->setPosterImage( popplerMovie->posterImage() );
    return movie;
}

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
    if ( rendition->repeatCount() == 0 ) {
        movie->setPlayMode( Okular::Movie::PlayRepeat );
    } else {
        movie->setPlayMode( Okular::Movie::PlayLimited );
        movie->setPlayRepetitions( rendition->repeatCount() );
    }
    movie->setAutoPlay( rendition->autoPlay() );
    return movie;
}

#ifdef HAVE_POPPLER_0_36
QPair<Okular::Movie*, Okular::EmbeddedFile*> createMovieFromPopplerRichMedia( const Poppler::RichMediaAnnotation *popplerRichMedia )
{
    const QPair<Okular::Movie*, Okular::EmbeddedFile*> emptyResult(0, 0);

    /**
     * To convert a Flash/Video based RichMedia annotation to a movie, we search for the first
     * Flash/Video richmedia instance and parse the flashVars parameter for the 'source' identifier.
     * That identifier is then used to find the associated embedded file through the assets
     * mapping.
     */
    const Poppler::RichMediaAnnotation::Content *content = popplerRichMedia->content();
    if ( !content )
        return emptyResult;

    const QList<Poppler::RichMediaAnnotation::Configuration*> configurations = content->configurations();
    if ( configurations.isEmpty() )
        return emptyResult;

    const Poppler::RichMediaAnnotation::Configuration *configuration = configurations[0];

    const QList<Poppler::RichMediaAnnotation::Instance*> instances = configuration->instances();
    if ( instances.isEmpty() )
        return emptyResult;

    const Poppler::RichMediaAnnotation::Instance *instance = instances[0];

    if ( ( instance->type() != Poppler::RichMediaAnnotation::Instance::TypeFlash ) &&
         ( instance->type() != Poppler::RichMediaAnnotation::Instance::TypeVideo ) )
        return emptyResult;

    const Poppler::RichMediaAnnotation::Params *params = instance->params();
    if ( !params )
        return emptyResult;

    QString sourceId;
    bool playbackLoops = false;

    const QStringList flashVars = params->flashVars().split( QLatin1Char( '&' ) );
    foreach ( const QString & flashVar, flashVars ) {
        const int pos = flashVar.indexOf( QLatin1Char( '=' ) );
        if ( pos == -1 )
            continue;

        const QString key = flashVar.left( pos );
        const QString value = flashVar.mid( pos + 1 );

        if ( key == QLatin1String( "source" ) )
            sourceId = value;
        else if ( key == QLatin1String( "loop" ) )
            playbackLoops = ( value == QLatin1String( "true" ) ? true : false );
    }

    if ( sourceId.isEmpty() )
        return emptyResult;

    const QList<Poppler::RichMediaAnnotation::Asset*> assets = content->assets();
    if ( assets.isEmpty() )
        return emptyResult;

    Poppler::RichMediaAnnotation::Asset *matchingAsset = 0;
    foreach ( Poppler::RichMediaAnnotation::Asset *asset, assets ) {
        if ( asset->name() == sourceId ) {
            matchingAsset = asset;
            break;
        }
    }

    if ( !matchingAsset )
        return emptyResult;

    Poppler::EmbeddedFile *embeddedFile = matchingAsset->embeddedFile();
    if ( !embeddedFile )
        return emptyResult;

    Okular::EmbeddedFile *pdfEmbeddedFile = new PDFEmbeddedFile( embeddedFile );

    Okular::Movie *movie = new Okular::Movie( embeddedFile->name(), embeddedFile->data() );
    movie->setPlayMode( playbackLoops ? Okular::Movie::PlayRepeat : Okular::Movie::PlayLimited );

    if ( popplerRichMedia && popplerRichMedia->settings() && popplerRichMedia->settings()->activation() ) {
        if ( popplerRichMedia->settings()->activation()->condition() == Poppler::RichMediaAnnotation::Activation::PageOpened ||
             popplerRichMedia->settings()->activation()->condition() == Poppler::RichMediaAnnotation::Activation::PageVisible ) {
            movie->setAutoPlay( true );
        } else {
            movie->setAutoPlay( false );
        }

    } else {
        movie->setAutoPlay( false );
    }

    return qMakePair(movie, pdfEmbeddedFile);
}
#endif

/**
 * Note: the function will take ownership of the popplerLink object.
 */
Okular::Action* createLinkFromPopplerLink(const Poppler::Link *popplerLink)
{
    if (!popplerLink)
        return nullptr;

    Okular::Action *link = 0;
    const Poppler::LinkGoto *popplerLinkGoto;
    const Poppler::LinkExecute *popplerLinkExecute;
    const Poppler::LinkBrowse *popplerLinkBrowse;
    const Poppler::LinkAction *popplerLinkAction;
    const Poppler::LinkSound *popplerLinkSound;
    const Poppler::LinkJavaScript *popplerLinkJS;
    const Poppler::LinkMovie *popplerLinkMovie;
    const Poppler::LinkRendition *popplerLinkRendition;
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
            link = new Okular::BrowseAction( QUrl(popplerLinkBrowse->url()) );
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

OKULAR_EXPORT_PLUGIN(PDFGenerator, "libokularGenerator_poppler.json")

static void PDFGeneratorPopplerDebugFunction(const QString &message, const QVariant &closure)
{
    Q_UNUSED(closure);
    qCDebug(OkularPdfDebug) << "[Poppler]" << message;
}

PDFGenerator::PDFGenerator( QObject *parent, const QVariantList &args )
    : Generator( parent, args ), pdfdoc( 0 ),
    docSynopsisDirty( true ),
    docEmbeddedFilesDirty( true ), nextFontPage( 0 ),
    annotProxy( 0 )
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
    setFeature( SwapBackingFile );

    // You only need to do it once not for each of the documents but it is cheap enough
    // so doing it all the time won't hurt either
    Poppler::setDebugErrorFunction(PDFGeneratorPopplerDebugFunction, QVariant());
}

PDFGenerator::~PDFGenerator()
{
    delete pdfOptionsPage;
}

//BEGIN Generator inherited functions
Okular::Document::OpenResult PDFGenerator::loadDocumentWithPassword( const QString & filePath, QVector<Okular::Page*> & pagesVector, const QString &password )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        qCDebug(OkularPdfDebug) << "PDFGenerator: multiple calls to loadDocument. Check it.";
        return Okular::Document::OpenError;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::load( filePath, 0, 0 );
    return init(pagesVector, password);
}

Okular::Document::OpenResult PDFGenerator::loadDocumentFromDataWithPassword( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector, const QString &password )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        qCDebug(OkularPdfDebug) << "PDFGenerator: multiple calls to loadDocument. Check it.";
        return Okular::Document::OpenError;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::loadFromData( fileData, 0, 0 );
    return init(pagesVector, password);
}

Okular::Document::OpenResult PDFGenerator::init(QVector<Okular::Page*> & pagesVector, const QString &password)
{
    if ( !pdfdoc )
        return Okular::Document::OpenError;

    if ( pdfdoc->isLocked() )
    {
        pdfdoc->unlock( password.toLatin1(), password.toLatin1() );

        if ( pdfdoc->isLocked() ) {
            delete pdfdoc;
            pdfdoc = 0;
            return Okular::Document::OpenNeedsPassword;
        }
    }

    // build Pages (currentPage was set -1 by deletePages)
    int pageCount = pdfdoc->numPages();
    if (pageCount < 0) {
        delete pdfdoc;
        pdfdoc = 0;
        return Okular::Document::OpenError;
    }
    pagesVector.resize(pageCount);
    rectsGenerated.fill(false, pageCount);

    annotationsOnOpenHash.clear();

    loadPages(pagesVector, 0, false);

    // update the configuration
    reparseConfig();

    // create annotation proxy
    annotProxy = new PopplerAnnotationProxy( pdfdoc, userMutex(), &annotationsOnOpenHash );

    // the file has been loaded correctly
    return Okular::Document::OpenSuccess;
}

PDFGenerator::SwapBackingFileResult PDFGenerator::swapBackingFile( QString const &newFileName, QVector<Okular::Page*> & newPagesVector )
{
    doCloseDocument();
    auto openResult = loadDocumentWithPassword(newFileName, newPagesVector, QString());
    if (openResult != Okular::Document::OpenSuccess)
        return SwapBackingFileError;

    return SwapBackingFileReloadInternalData;
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
    docSynopsisDirty = true;
    docSyn.clear();
    docEmbeddedFilesDirty = true;
    qDeleteAll(docEmbeddedFiles);
    docEmbeddedFiles.clear();
    nextFontPage = 0;
    rectsGenerated.clear();

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
            qCDebug(OkularPdfDebug) << "load page" << i << "with rotation" << rotation << "and orientation" << orientation;
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

Okular::DocumentInfo PDFGenerator::generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const
{
    Okular::DocumentInfo docInfo;
    docInfo.set( Okular::DocumentInfo::MimeType, QStringLiteral("application/pdf") );

    userMutex()->lock();


    if ( pdfdoc )
    {
        // compile internal structure reading properties from PDFDoc
        if ( keys.contains( Okular::DocumentInfo::Title ) )
            docInfo.set( Okular::DocumentInfo::Title, pdfdoc->info(QStringLiteral("Title")) );
        if ( keys.contains( Okular::DocumentInfo::Subject ) )
            docInfo.set( Okular::DocumentInfo::Subject, pdfdoc->info(QStringLiteral("Subject")) );
        if ( keys.contains( Okular::DocumentInfo::Author ) )
            docInfo.set( Okular::DocumentInfo::Author, pdfdoc->info(QStringLiteral("Author")) );
        if ( keys.contains( Okular::DocumentInfo::Keywords ) )
            docInfo.set( Okular::DocumentInfo::Keywords, pdfdoc->info(QStringLiteral("Keywords")) );
        if ( keys.contains( Okular::DocumentInfo::Creator ) )
            docInfo.set( Okular::DocumentInfo::Creator, pdfdoc->info(QStringLiteral("Creator")) );
        if ( keys.contains( Okular::DocumentInfo::Producer ) )
            docInfo.set( Okular::DocumentInfo::Producer, pdfdoc->info(QStringLiteral("Producer")) );
        if ( keys.contains( Okular::DocumentInfo::CreationDate ) )
            docInfo.set( Okular::DocumentInfo::CreationDate, QLocale().toString( pdfdoc->date(QStringLiteral("CreationDate")), QLocale::LongFormat ) );
        if ( keys.contains( Okular::DocumentInfo::ModificationDate ) )
            docInfo.set( Okular::DocumentInfo::ModificationDate, QLocale().toString( pdfdoc->date(QStringLiteral("ModDate")), QLocale::LongFormat ) );
        if ( keys.contains( Okular::DocumentInfo::CustomKeys ) )
        {
            int major, minor;
            pdfdoc->getPdfVersion(&major, &minor);
            docInfo.set( QStringLiteral("format"), i18nc( "PDF v. <version>", "PDF v. %1.%2", major, minor ), i18n( "Format" ) );
            docInfo.set( QStringLiteral("encryption"), pdfdoc->isEncrypted() ? i18n( "Encrypted" ) : i18n( "Unencrypted" ), i18n("Security") );
            docInfo.set( QStringLiteral("optimization"), pdfdoc->isLinearized() ? i18n( "Yes" ) : i18n( "No" ), i18n("Optimized") );
        }

        docInfo.set( Okular::DocumentInfo::Pages, QString::number( pdfdoc->numPages() ) );
    }
    userMutex()->unlock();

    return docInfo;
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

    Poppler::FontIterator* it = pdfdoc->newFontIterator(page);
    if (it->hasNext()) {
        fonts = it->next();
    }
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

QAbstractItemModel* PDFGenerator::layersModel() const
{
    return pdfdoc->hasOptionalContent() ? pdfdoc->optionalContentModel() : NULL;
}

void PDFGenerator::opaqueAction( const Okular::BackendOpaqueAction *action )
{
#ifdef HAVE_POPPLER_0_50
    const Poppler::LinkOCGState *popplerLink = action->nativeId().value<const Poppler::LinkOCGState *>();
    pdfdoc->optionalContentModel()->applyLink( const_cast< Poppler::LinkOCGState* >( popplerLink ) );
#else
    (void)action;
#endif
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

#ifdef HAVE_POPPLER_0_62
struct PartialUpdatePayload
{
    PartialUpdatePayload(PDFGenerator *g, Okular::PixmapRequest *r) :
        generator(g), request(r)
    {
        // Don't report partial updates for the first 500 ms
        timer.setInterval(500);
        timer.setSingleShot(true);
        timer.start();
    }

    PDFGenerator *generator;
    Okular::PixmapRequest *request;
    QTimer timer;
};
Q_DECLARE_METATYPE(PartialUpdatePayload*)

static bool shouldDoPartialUpdateCallback(const QVariant &vPayload)
{
    auto payload = vPayload.value<PartialUpdatePayload *>();

    // Since the timer lives in a thread without an event loop we need to stop it ourselves
    // when the remaining time has reached 0
    if (payload->timer.isActive() && payload->timer.remainingTime() == 0) {
        payload->timer.stop();
    }

    return !payload->timer.isActive();
}

static void partialUpdateCallback(const QImage &image, const QVariant &vPayload)
{
    auto payload = vPayload.value<PartialUpdatePayload *>();
    QMetaObject::invokeMethod(payload->generator, "signalPartialPixmapRequest", Qt::QueuedConnection, Q_ARG(Okular::PixmapRequest*, payload->request), Q_ARG(QImage, image));
}
#endif

QImage PDFGenerator::image( Okular::PixmapRequest * request )
{
    // debug requests to this (xpdf) generator
    //qCDebug(OkularPdfDebug) << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "].";

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
#ifdef HAVE_POPPLER_0_62
            if ( request->partialUpdatesWanted() )
            {
                PartialUpdatePayload payload( this, request );
                img = p->renderToImage( fakeDpiX, fakeDpiY, rect.x(), rect.y(), rect.width(), rect.height(), Poppler::Page::Rotate0,
                                        partialUpdateCallback, shouldDoPartialUpdateCallback, QVariant::fromValue( &payload ) );
            }
            else
#endif
            {
                img = p->renderToImage( fakeDpiX, fakeDpiY, rect.x(), rect.y(), rect.width(), rect.height(), Poppler::Page::Rotate0 );
            }
        }
        else
        {
#ifdef HAVE_POPPLER_0_62
            if ( request->partialUpdatesWanted() )
            {
                PartialUpdatePayload payload(this, request);
                img = p->renderToImage( fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0,
                                        partialUpdateCallback, shouldDoPartialUpdateCallback, QVariant::fromValue( &payload ) );
            }
            else
#endif
            {
                img = p->renderToImage(fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0 );
            }
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
    if ( !action )
        return;

    if ( (action->actionType() != Okular::Action::Movie) && (action->actionType() != Okular::Action::Rendition) )
        return;

    resolveMediaLinks<Poppler::LinkMovie, Okular::MovieAction, Poppler::MovieAnnotation, Okular::MovieAnnotation>( action, Okular::Annotation::AMovie, annotationsOnOpenHash );
    resolveMediaLinks<Poppler::LinkRendition, Okular::RenditionAction, Poppler::ScreenAnnotation, Okular::ScreenAnnotation>( action, Okular::Annotation::AScreen, annotationsOnOpenHash );
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
    qCDebug(OkularPdfDebug) << "page" << page->number();
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
    bool printAnnots = true;
    bool forceRasterize = false;

    if ( pdfOptionsPage )
    {
        printAnnots = pdfOptionsPage->printAnnots();
        forceRasterize = pdfOptionsPage->printForceRaster();
    }

#ifdef Q_OS_WIN
    // Windows can only print by rasterization, because that is
    // currently the only way Okular implements printing without using UNIX-specific
    // tools like 'lpr'.
    forceRasterize = true;
#ifndef HAVE_POPPLER_0_60
    // The Document::HideAnnotations flags was introduced in poppler 0.60
    printAnnots = true;
#endif
#endif

#ifdef HAVE_POPPLER_0_60
    if ( forceRasterize )
    {
        pdfdoc->setRenderHint(Poppler::Document::HideAnnotations, !printAnnots);
#else
    if ( forceRasterize && printAnnots)
    {
#endif
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
#ifdef Q_OS_WIN
            QImage img = pp->renderToImage(  printer.physicalDpiX(), printer.physicalDpiY() );
#else
            // UNIX: Same resolution as the postscript rasterizer; see discussion at https://git.reviewboard.kde.org/r/130218/
            QImage img = pp->renderToImage( 300, 300 );
#endif
            painter.drawImage( painter.window(), img, QRectF(0, 0, img.width(), img.height()) );
            delete pp;
        }
        userMutex()->unlock();
    }
    painter.end();
    return true;
    }

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
    QTemporaryFile tf(QDir::tempPath() + QLatin1String("/okular_XXXXXX.ps"));
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

    QString pstitle = metaData(QStringLiteral("Title"), QVariant()).toString();
    if ( pstitle.trimmed().isEmpty() )
    {
        pstitle = document()->currentDocument().fileName();
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

    if (!printAnnots)
        psConverter->setPSOptions(psConverter->psOptions() | Poppler::PSConverter::HideAnnotations );

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
}

QVariant PDFGenerator::metaData( const QString & key, const QVariant & option ) const
{
    if ( key == QLatin1String("StartFullScreen") )
    {
        QMutexLocker ml(userMutex());
        // asking for the 'start in fullscreen mode' (pdf property)
        if ( pdfdoc->pageMode() == Poppler::Document::FullScreen )
            return true;
    }
    else if ( key == QLatin1String("NamedViewport") && !option.toString().isEmpty() )
    {
        Okular::DocumentViewport viewport;
        QString optionString = option.toString();

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
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    else if ( key == QLatin1String("DocumentTitle") )
    {
        userMutex()->lock();
        QString title = pdfdoc->info( QStringLiteral("Title") );
        userMutex()->unlock();
        return title;
    }
    else if ( key == QLatin1String("OpenTOC") )
    {
        QMutexLocker ml(userMutex());
        if ( pdfdoc->pageMode() == Poppler::Document::UseOutlines )
            return true;
    }
    else if ( key == QLatin1String("DocumentScripts") && option.toString() == QLatin1String("JavaScript") )
    {
        QMutexLocker ml(userMutex());
        return pdfdoc->scripts();
    }
    else if ( key == QLatin1String("HasUnsupportedXfaForm") )
    {
        QMutexLocker ml(userMutex());
        return pdfdoc->formType() == Poppler::Document::XfaForm;
    }
    else if ( key == QLatin1String("FormCalculateOrder") )
    {
#ifdef HAVE_POPPLER_0_53
        QMutexLocker ml(userMutex());
        return QVariant::fromValue<QVector<int>>(pdfdoc->formCalculateOrder());
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
    QColor color = documentMetaData( PaperColorMetaData, true ).value< QColor >();
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
    dlg->addPage(w, PDFSettings::self(), i18n("PDF"), QStringLiteral("application-pdf"), i18n("PDF Backend Configuration") );
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
    SET_HINT(GraphicsAntialiasMetaData, true, Poppler::Document::Antialiasing)
    SET_HINT(TextAntialiasMetaData, true, Poppler::Document::TextAntialiasing)
    SET_HINT(TextHintingMetaData, false, Poppler::Document::TextHinting)
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
    if ( format.mimeType().inherits( QStringLiteral( "text/plain" ) ) ) {
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
    qCDebug(OkularPdfDebug) << "getting text page in generator pdf - rotation:" << rot;
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
                append(ktp, (j==qstringCharCount-1 && !next) ? (s + QLatin1Char('\n')) : s,
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
            append(ktp, QStringLiteral(" "),
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

        if (!e.attribute(QStringLiteral("ExternalFileName")).isNull()) item.setAttribute(QStringLiteral("ExternalFileName"), e.attribute(QStringLiteral("ExternalFileName")));
        if (!e.attribute(QStringLiteral("DestinationName")).isNull()) item.setAttribute(QStringLiteral("ViewportName"), e.attribute(QStringLiteral("DestinationName")));
        if (!e.attribute(QStringLiteral("Destination")).isNull())
        {
            Okular::DocumentViewport vp;
            fillViewportFromLinkDestination( vp, Poppler::LinkDestination(e.attribute(QStringLiteral("Destination"))) );
            item.setAttribute( QStringLiteral("Viewport"), vp.toString() );
        }
        if (!e.attribute(QStringLiteral("Open")).isNull()) item.setAttribute(QStringLiteral("Open"), e.attribute(QStringLiteral("Open")));
        if (!e.attribute(QStringLiteral("DestinationURI")).isNull()) item.setAttribute(QStringLiteral("URL"), e.attribute(QStringLiteral("DestinationURI")));

        // descend recursively and advance to the next node
        if ( e.hasChildNodes() ) addSynopsisChildren( &n, &    item );
        n = n.nextSibling();
    }
}

void PDFGenerator::addAnnotations( Poppler::Page * popplerPage, Okular::Page * page )
{
#ifdef HAVE_POPPLER_0_28
    QSet<Poppler::Annotation::SubType> subtypes;
    subtypes << Poppler::Annotation::AFileAttachment
        << Poppler::Annotation::ASound
        << Poppler::Annotation::AMovie
        << Poppler::Annotation::AWidget
        << Poppler::Annotation::AScreen
        << Poppler::Annotation::AText
        << Poppler::Annotation::ALine
        << Poppler::Annotation::AGeom
        << Poppler::Annotation::AHighlight
        << Poppler::Annotation::AInk
        << Poppler::Annotation::AStamp
        << Poppler::Annotation::ACaret;

    QList<Poppler::Annotation*> popplerAnnotations = popplerPage->annotations( subtypes );
#else
    QList<Poppler::Annotation*> popplerAnnotations = popplerPage->annotations();
#endif

    foreach(Poppler::Annotation *a, popplerAnnotations)
    {
        bool doDelete = true;
        Okular::Annotation * newann = createAnnotationFromPopplerAnnotation( a, &doDelete );
        if (newann)
        {
            page->addAnnotation(newann);

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

            if ( !doDelete )
                annotationsOnOpenHash.insert( newann, a );
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

#ifdef HAVE_POPPLER_0_37
    transition->setDuration( pdfTransition->durationReal() );
#else
    transition->setDuration( pdfTransition->duration() );
#endif

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

PDFGenerator::PrintError PDFGenerator::printError() const
{
    return lastPrintError;
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
            return true;
        }
        default: ;
    }
    return false;
}

bool PDFGenerator::save( const QString &fileName, SaveOptions options, QString *errorText )
{
    Q_UNUSED(errorText);
    Poppler::PDFConverter *pdfConv = pdfdoc->pdfConverter();

    pdfConv->setOutputFileName( fileName );
    if ( options & SaveChanges )
        pdfConv->setPDFOptions( pdfConv->pdfOptions() | Poppler::PDFConverter::WithChanges );

    QMutexLocker locker( userMutex() );

    QHashIterator<Okular::Annotation*, Poppler::Annotation*> it( annotationsOnOpenHash );
    while ( it.hasNext() )
    {
        it.next();

        if ( it.value()->uniqueName().isEmpty() )
        {
            it.value()->setUniqueName( it.key()->uniqueName() );
        }
    }

    bool success = pdfConv->convert();
    if (!success)
    {
        switch (pdfConv->lastError())
        {
            case Poppler::BaseConverter::NotSupportedInputFileError:
                // This can only happen with Poppler before 0.22 which did not have qt5 version
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
    delete pdfConv;
    return success;
}

Okular::AnnotationProxy* PDFGenerator::annotationProxy() const
{
    return annotProxy;
}

#include "generator_pdf.moc"

Q_LOGGING_CATEGORY(OkularPdfDebug, "org.kde.okular.generators.pdf", QtWarningMsg)

/* kate: replace-tabs on; indent-width 4; */
