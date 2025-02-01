/*
    SPDX-FileCopyrightText: 2004-2008 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2012 Guillermo A. Amaral B. <gamaral@kde.org>
    SPDX-FileCopyrightText: 2019 Oliver Sander <oliver.sander@tu-dresden.de>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <memory>

#include "generator_pdf.h"

// qt/kde includes
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QLayout>
#include <QMutex>
#include <QPainter>
#include <QPrinter>
#include <QStack>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTimeZone>
#include <QTimer>

#include <KAboutData>
#include <KConfigDialog>
#include <KLocalizedString>

#include <core/action.h>
#include <core/annotations.h>
#include <core/fileprinter.h>
#include <core/movie.h>
#include <core/page.h>
#include <core/pagetransition.h>
#include <core/signatureutils.h>
#include <core/sound.h>
#include <core/sourcereference.h>
#include <core/textpage.h>
#include <core/utils.h>

#include "pdfsettings.h"

#include <poppler-media.h>
#include <poppler-version.h>

#include "annots.h"
#include "debug_pdf.h"
#include "formfields.h"
#include "imagescaling.h"
#include "pdfsettingswidget.h"
#include "pdfsignatureutils.h"
#include "popplerembeddedfile.h"

#include <functional>

Q_DECLARE_METATYPE(Poppler::Annotation *)
Q_DECLARE_METATYPE(Poppler::FontInfo)

#define POPPLER_VERSION_MACRO ((POPPLER_VERSION_MAJOR << 16) | (POPPLER_VERSION_MINOR << 8) | (POPPLER_VERSION_MICRO))

static const int defaultPageWidth = 595;
static const int defaultPageHeight = 842;

class PDFOptionsPage : public Okular::PrintOptionsWidget
{
    Q_OBJECT

public:
    enum ScaleMode { FitToPrintableArea, FitToPage, None };
    Q_ENUM(ScaleMode)

    PDFOptionsPage()
    {
        setWindowTitle(i18n("PDF Options"));
        QVBoxLayout *layout = new QVBoxLayout(this);
        m_printAnnots = new QCheckBox(i18n("Print annotations"), this);
        m_printAnnots->setToolTip(i18n("Include annotations in the printed document"));
        m_printAnnots->setWhatsThis(i18n("Includes annotations in the printed document. You can disable this if you want to print the original unannotated document."));
        layout->addWidget(m_printAnnots);
        m_forceRaster = new QCheckBox(i18n("Force rasterization"), this);
        m_forceRaster->setToolTip(i18n("Rasterize into an image before printing"));
        m_forceRaster->setWhatsThis(i18n("Forces the rasterization of each page into an image before printing it. This usually gives somewhat worse results, but is useful when printing documents that appear to print incorrectly."));
        layout->addWidget(m_forceRaster);

        QWidget *formWidget = new QWidget(this);
        QFormLayout *printBackendLayout = new QFormLayout(formWidget);

        m_scaleMode = new QComboBox;
        m_scaleMode->insertItem(FitToPrintableArea, i18n("Fit to printable area"), FitToPrintableArea);
        m_scaleMode->insertItem(FitToPage, i18n("Fit to full page"), FitToPage);
        m_scaleMode->insertItem(None, i18n("None; print original size"), None);
        m_scaleMode->setToolTip(i18n("Scaling mode for the printed pages"));
        printBackendLayout->addRow(i18n("Scale mode:"), m_scaleMode);

        // Set scale mode to users default
        m_scaleMode->setCurrentIndex(PDFSettings::printScaleMode());

        // Set rasterizing if scale mode is 1 or 2
        if (m_scaleMode->currentIndex() != 0) {
            m_forceRaster->setCheckState(Qt::Checked);
        }

        // If the user selects a scaling mode that requires the use of the
        // "Force rasterization" feature, enable it automatically so they don't
        // have to 1) know this and 2) do it manually
        connect(m_scaleMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=, this](int index) { m_forceRaster->setChecked(index != 0); });

        layout->addWidget(formWidget);

        layout->addStretch(1);

        setPrintAnnots(true); // Default value
    }

    bool ignorePrintMargins() const override
    {
        return scaleMode() == FitToPage;
    }

    bool printAnnots()
    {
        return m_printAnnots->isChecked();
    }

    void setPrintAnnots(bool printAnnots)
    {
        m_printAnnots->setChecked(printAnnots);
    }

    bool printForceRaster()
    {
        return m_forceRaster->isChecked();
    }

    void setPrintForceRaster(bool forceRaster)
    {
        m_forceRaster->setChecked(forceRaster);
    }

    ScaleMode scaleMode() const
    {
        return m_scaleMode->currentData().value<ScaleMode>();
    }

private:
    QCheckBox *m_printAnnots;
    QCheckBox *m_forceRaster;
    QComboBox *m_scaleMode;
};

static void fillViewportFromLinkDestination(Okular::DocumentViewport &viewport, const Poppler::LinkDestination &destination)
{
    viewport.pageNumber = destination.pageNumber() - 1;

    if (!viewport.isValid()) {
        return;
    }

    // get destination position
    // TODO add other attributes to the viewport (taken from link)
    //     switch ( destination->getKind() )
    //     {
    //         case destXYZ:
    if (destination.isChangeLeft() || destination.isChangeTop()) {
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

Okular::Sound *createSoundFromPopplerSound(const Poppler::SoundObject *popplerSound)
{
    Okular::Sound *sound = popplerSound->soundType() == Poppler::SoundObject::Embedded ? new Okular::Sound(popplerSound->data()) : new Okular::Sound(popplerSound->url());
    sound->setSamplingRate(popplerSound->samplingRate());
    sound->setChannels(popplerSound->channels());
    sound->setBitsPerSample(popplerSound->bitsPerSample());
    switch (popplerSound->soundEncoding()) {
    case Poppler::SoundObject::Raw:
        sound->setSoundEncoding(Okular::Sound::Raw);
        break;
    case Poppler::SoundObject::Signed:
        sound->setSoundEncoding(Okular::Sound::Signed);
        break;
    case Poppler::SoundObject::muLaw:
        sound->setSoundEncoding(Okular::Sound::muLaw);
        break;
    case Poppler::SoundObject::ALaw:
        sound->setSoundEncoding(Okular::Sound::ALaw);
        break;
    }
    return sound;
}

Okular::Movie *createMovieFromPopplerMovie(const Poppler::MovieObject *popplerMovie)
{
    Okular::Movie *movie = new Okular::Movie(popplerMovie->url());
    movie->setSize(popplerMovie->size());
    movie->setRotation((Okular::Rotation)(popplerMovie->rotation() / 90));
    movie->setShowControls(popplerMovie->showControls());
    movie->setPlayMode((Okular::Movie::PlayMode)popplerMovie->playMode());
    movie->setAutoPlay(false); // will be triggered by external MovieAnnotation
    movie->setStartPaused(false);
    movie->setShowPosterImage(popplerMovie->showPosterImage());
    movie->setPosterImage(popplerMovie->posterImage());
    return movie;
}

Okular::Movie *createMovieFromPopplerScreen(const Poppler::LinkRendition *popplerScreen)
{
    Poppler::MediaRendition *rendition = popplerScreen->rendition();
    Okular::Movie *movie = nullptr;
    if (rendition->isEmbedded()) {
        movie = new Okular::Movie(rendition->fileName(), rendition->data());
    } else {
        movie = new Okular::Movie(rendition->fileName());
    }
    movie->setSize(rendition->size());
    movie->setShowControls(rendition->showControls());
    if (rendition->repeatCount() == 0) {
        movie->setPlayMode(Okular::Movie::PlayRepeat);
    } else {
        movie->setPlayMode(Okular::Movie::PlayLimited);
        movie->setPlayRepetitions(rendition->repeatCount());
    }
    /**
     * Warning: Confusing flag name from PDF spec. Described as:
     * > If true, the media should automatically play when activated.
     * > If false, the media should be initially paused when activated
     * To set autoplay, page actions are used.
     */
    movie->setStartPaused(!rendition->autoPlay());
    return movie;
}

QPair<Okular::Movie *, Okular::EmbeddedFile *> createMovieFromPopplerRichMedia(const Poppler::RichMediaAnnotation *popplerRichMedia)
{
    const QPair<Okular::Movie *, Okular::EmbeddedFile *> emptyResult(nullptr, nullptr);

    /**
     * To convert a Flash/Video based RichMedia annotation to a movie, we search for the first
     * Flash/Video richmedia instance and parse the flashVars parameter for the 'source' identifier.
     * That identifier is then used to find the associated embedded file through the assets
     * mapping.
     */
    const Poppler::RichMediaAnnotation::Content *content = popplerRichMedia->content();
    if (!content) {
        return emptyResult;
    }

    const QList<Poppler::RichMediaAnnotation::Configuration *> configurations = content->configurations();
    if (configurations.isEmpty()) {
        return emptyResult;
    }

    const Poppler::RichMediaAnnotation::Configuration *configuration = configurations[0];

    const QList<Poppler::RichMediaAnnotation::Instance *> instances = configuration->instances();
    if (instances.isEmpty()) {
        return emptyResult;
    }

    const Poppler::RichMediaAnnotation::Instance *instance = instances[0];

    if ((instance->type() != Poppler::RichMediaAnnotation::Instance::TypeFlash) && (instance->type() != Poppler::RichMediaAnnotation::Instance::TypeVideo)) {
        return emptyResult;
    }

    const Poppler::RichMediaAnnotation::Params *params = instance->params();
    if (!params) {
        return emptyResult;
    }

    QString sourceId;
    bool playbackLoops = false;

    const QStringList flashVars = params->flashVars().split(QLatin1Char('&'));
    for (const QString &flashVar : flashVars) {
        const int pos = flashVar.indexOf(QLatin1Char('='));
        if (pos == -1) {
            continue;
        }

        const QString key = flashVar.left(pos);
        const QString value = flashVar.mid(pos + 1);

        if (key == QLatin1String("source")) {
            sourceId = value;
        } else if (key == QLatin1String("loop")) {
            playbackLoops = (value == QLatin1String("true") ? true : false);
        }
    }

    if (sourceId.isEmpty()) {
        return emptyResult;
    }

    const QList<Poppler::RichMediaAnnotation::Asset *> assets = content->assets();
    if (assets.isEmpty()) {
        return emptyResult;
    }

    Poppler::RichMediaAnnotation::Asset *matchingAsset = nullptr;
    for (Poppler::RichMediaAnnotation::Asset *asset : assets) {
        if (asset->name() == sourceId) {
            matchingAsset = asset;
            break;
        }
    }

    if (!matchingAsset) {
        return emptyResult;
    }

    Poppler::EmbeddedFile *embeddedFile = matchingAsset->embeddedFile();
    if (!embeddedFile) {
        return emptyResult;
    }

    Okular::EmbeddedFile *pdfEmbeddedFile = new PDFEmbeddedFile(embeddedFile);

    Okular::Movie *movie = new Okular::Movie(embeddedFile->name(), embeddedFile->data());
    movie->setPlayMode(playbackLoops ? Okular::Movie::PlayRepeat : Okular::Movie::PlayLimited);

    if (popplerRichMedia && popplerRichMedia->settings() && popplerRichMedia->settings()->activation()) {
        if (popplerRichMedia->settings()->activation()->condition() == Poppler::RichMediaAnnotation::Activation::PageOpened ||
            popplerRichMedia->settings()->activation()->condition() == Poppler::RichMediaAnnotation::Activation::PageVisible) {
            movie->setAutoPlay(true);
        } else {
            movie->setAutoPlay(false);
        }

    } else {
        movie->setAutoPlay(false);
    }

    return qMakePair(movie, pdfEmbeddedFile);
}

static std::optional<Okular::DocumentAction::DocumentActionType> popplerToOkular(Poppler::LinkAction::ActionType pat)
{
    switch (pat) {
    case Poppler::LinkAction::PageFirst:
        return Okular::DocumentAction::PageFirst;
    case Poppler::LinkAction::PagePrev:
        return Okular::DocumentAction::PagePrev;
    case Poppler::LinkAction::PageNext:
        return Okular::DocumentAction::PageNext;
    case Poppler::LinkAction::PageLast:
        return Okular::DocumentAction::PageLast;
    case Poppler::LinkAction::HistoryBack:
        return Okular::DocumentAction::HistoryBack;
    case Poppler::LinkAction::HistoryForward:
        return Okular::DocumentAction::HistoryForward;
    case Poppler::LinkAction::Quit:
        return Okular::DocumentAction::Quit;
    case Poppler::LinkAction::Presentation:
        return Okular::DocumentAction::Presentation;
    case Poppler::LinkAction::EndPresentation:
        return Okular::DocumentAction::EndPresentation;
    case Poppler::LinkAction::Find:
        return Okular::DocumentAction::Find;
    case Poppler::LinkAction::GoToPage:
        return Okular::DocumentAction::GoToPage;
    case Poppler::LinkAction::Close:
        return Okular::DocumentAction::Close;
    case Poppler::LinkAction::Print:
        return Okular::DocumentAction::Print;
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(22, 04, 0)
    case Poppler::LinkAction::SaveAs:
        return Okular::DocumentAction::SaveAs;
#endif
    }

    qWarning() << "Unsupported Poppler::LinkAction::ActionType" << pat;
    return {};
}

// helper for using std::visit to get a dependent false for static_asserts
// to help get compile errors if one ever extends variants
template<class> inline constexpr bool always_false_v = false;

const Poppler::Link *rawPtr(std::variant<const Poppler::Link *, std::unique_ptr<Poppler::Link>> &link)
{
    return std::visit(
        [](auto &&item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, const Poppler::Link *>) {
                return item;
            } else if constexpr (std::is_same_v<T, std::unique_ptr<Poppler::Link>>) {
                return const_cast<const Poppler::Link *>(item.get());
            } else {
                static_assert(always_false_v<T>, "non-exhaustive visitor");
            }
        },
        link);
}

template<typename T> auto toSharedPointer(std::unique_ptr<Poppler::Link> link)
{
    std::shared_ptr<Poppler::Link> sharedLink = std::move(link);
    return std::static_pointer_cast<T>(sharedLink);
}

/**
 * Note: the function will take ownership of the popplerLink if the popplerlink object is in a unique ptr
 */
Okular::Action *createLinkFromPopplerLink(std::variant<const Poppler::Link *, std::unique_ptr<Poppler::Link>> popplerLink)
{
    auto rawPopplerLink = rawPtr(popplerLink);
    if (!rawPopplerLink) {
        return nullptr;
    }

    Okular::Action *link = nullptr;
    Okular::DocumentViewport viewport;

    switch (rawPopplerLink->linkType()) {
    case Poppler::Link::None:
        break;

    case Poppler::Link::Goto: {
        auto popplerLinkGoto = static_cast<const Poppler::LinkGoto *>(rawPopplerLink);
        const Poppler::LinkDestination dest = popplerLinkGoto->destination();
        const QString destName = dest.destinationName();
        if (destName.isEmpty()) {
            fillViewportFromLinkDestination(viewport, dest);
            link = new Okular::GotoAction(popplerLinkGoto->fileName(), viewport);
        } else {
            link = new Okular::GotoAction(popplerLinkGoto->fileName(), destName);
        }
    } break;

    case Poppler::Link::Execute: {
        auto popplerLinkExecute = static_cast<const Poppler::LinkExecute *>(rawPopplerLink);
        link = new Okular::ExecuteAction(popplerLinkExecute->fileName(), popplerLinkExecute->parameters());
        break;
    }

    case Poppler::Link::Browse: {
        auto popplerLinkBrowse = static_cast<const Poppler::LinkBrowse *>(rawPopplerLink);
        link = new Okular::BrowseAction(QUrl(popplerLinkBrowse->url()));
        break;
    }

    case Poppler::Link::Action: {
        auto popplerLinkAction = static_cast<const Poppler::LinkAction *>(rawPopplerLink);
        const auto okularLinkActionType = popplerToOkular(popplerLinkAction->actionType());
        if (okularLinkActionType) {
            link = new Okular::DocumentAction(okularLinkActionType.value());
        }
        break;
    }

    case Poppler::Link::Sound: {
        auto popplerLinkSound = static_cast<const Poppler::LinkSound *>(rawPopplerLink);
        Poppler::SoundObject *popplerSound = popplerLinkSound->sound();
        Okular::Sound *sound = createSoundFromPopplerSound(popplerSound);
        link = new Okular::SoundAction(popplerLinkSound->volume(), popplerLinkSound->synchronous(), popplerLinkSound->repeat(), popplerLinkSound->mix(), sound);
    } break;

    case Poppler::Link::JavaScript: {
        auto popplerLinkJS = static_cast<const Poppler::LinkJavaScript *>(rawPopplerLink);
        link = new Okular::ScriptAction(Okular::JavaScript, popplerLinkJS->script());
    } break;

    case Poppler::Link::Rendition: {
        /* This gets weird. Depending on which parts of Poppler gives us a
         * rendition link, it might be owned by us; it might be owned by poppler
         * Luckily we can count on the return types being correct from poppler.
         * If it is owned by poppler, we get a raw pointer
         * if ownership is transferred, we get a unique ptr.
         *
         * So for that reason, put the owned one in a normal shared_ptr for later usage
         * and cleanup
         *
         * and put the non-owned in a special shared_ptr with a nondeleter as deleter
         */
        std::shared_ptr<const Poppler::LinkRendition> popplerLinkRendition;
        if (std::holds_alternative<std::unique_ptr<Poppler::Link>>(popplerLink)) {
            auto uniquePopplerLink = std::get<std::unique_ptr<Poppler::Link>>(std::move(popplerLink));
            popplerLinkRendition = toSharedPointer<const Poppler::LinkRendition>(std::move(uniquePopplerLink));
        } else {
            popplerLinkRendition = std::shared_ptr<const Poppler::LinkRendition>(static_cast<const Poppler::LinkRendition *>(rawPopplerLink), [](auto *) { /*don't delete this*/ });
        }

        Okular::RenditionAction::OperationType operation = Okular::RenditionAction::None;
        switch (popplerLinkRendition->action()) {
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

        Okular::Movie *movie = nullptr;
        if (popplerLinkRendition->rendition()) {
            movie = createMovieFromPopplerScreen(popplerLinkRendition.get());
        }

        Okular::RenditionAction *renditionAction = new Okular::RenditionAction(operation, movie, Okular::JavaScript, popplerLinkRendition->script());
        renditionAction->setNativeHandle(popplerLinkRendition);
        link = renditionAction;
    } break;

    case Poppler::Link::Movie: {
        if (!std::holds_alternative<std::unique_ptr<Poppler::Link>>(popplerLink)) {
            // See comment above in Link::Rendition
            qCDebug(OkularPdfDebug) << "parsing movie link without taking ownership is not supported. Action chain might be broken.";
            break;
        }

        auto uniquePopplerLink = std::get<std::unique_ptr<Poppler::Link>>(std::move(popplerLink));
        auto popplerLinkMovie = toSharedPointer<const Poppler::LinkMovie>(std::move(uniquePopplerLink));

        Okular::MovieAction::OperationType operation = Okular::MovieAction::Play;
        switch (popplerLinkMovie->operation()) {
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

        Okular::MovieAction *movieAction = new Okular::MovieAction(operation);
        movieAction->setNativeHandle(popplerLinkMovie);
        link = movieAction;
    } break;

    case Poppler::Link::Hide: {
        const Poppler::LinkHide *l = static_cast<const Poppler::LinkHide *>(rawPopplerLink);
        QStringList scripts;
        const QVector<QString> targets = l->targets();
        for (const QString &target : targets) {
            scripts << QStringLiteral("getField(\"%1\").hidden = %2;").arg(target).arg(l->isShowAction() ? QLatin1String("false") : QLatin1String("true"));
        }
        link = new Okular::ScriptAction(Okular::JavaScript, scripts.join(QLatin1Char('\n')));
    } break;

    case Poppler::Link::OCGState: {
        if (!std::holds_alternative<std::unique_ptr<Poppler::Link>>(popplerLink)) {
            // See comment above in Link::Rendition
            qCDebug(OkularPdfDebug) << "ocg link without taking ownership is not supported. Action chain might be broken.";
            break;
        }
        link = new Okular::BackendOpaqueAction();
        auto uniquePopplerLink = std::get<std::unique_ptr<Poppler::Link>>(std::move(popplerLink));
        auto popplerLinkOCG = toSharedPointer<const Poppler::LinkOCGState>(std::move(uniquePopplerLink));
        link->setNativeHandle(popplerLinkOCG);
        break;
    }
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(24, 07, 0)
    case Poppler::Link::ResetForm: {
        if (!std::holds_alternative<std::unique_ptr<Poppler::Link>>(popplerLink)) {
            // See comment above in Link::Rendition
            qCDebug(OkularPdfDebug) << "ResetForm link without taking ownership is not supported. Action chain might be broken.";
            break;
        }
        link = new Okular::BackendOpaqueAction();
        auto uniquePopplerLink = std::get<std::unique_ptr<Poppler::Link>>(std::move(popplerLink));
        auto popplerLinkResetForm = toSharedPointer<const Poppler::LinkResetForm>(std::move(uniquePopplerLink));
        link->setNativeHandle(popplerLinkResetForm);
    } break;
#endif
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(24, 10, 0)
    case Poppler::Link::SubmitForm: {
        // TODO
    } break;
#endif
    }

    if (link) {
        QVector<Okular::Action *> nextActions;
        const QVector<Poppler::Link *> nextLinks = rawPopplerLink->nextLinks();
        for (const Poppler::Link *nl : nextLinks) {
            nextActions << createLinkFromPopplerLink(nl);
        }
        link->setNextActions(nextActions);
    }

    return link;
}

/**
 * Note: the function will take ownership of the popplerLink objects.
 */
static QList<Okular::ObjectRect *> generateLinks(std::vector<std::unique_ptr<Poppler::Link>> &&popplerLinks)
{
    QList<Okular::ObjectRect *> links;
    for (auto &popplerLink : popplerLinks) {
        QRectF linkArea = popplerLink->linkArea();
        double nl = linkArea.left(), nt = linkArea.top(), nr = linkArea.right(), nb = linkArea.bottom();
        // create the rect using normalized coords and attach the Okular::Link to it
        Okular::ObjectRect *rect = new Okular::ObjectRect(nl, nt, nr, nb, false, Okular::ObjectRect::Action, createLinkFromPopplerLink(std::move(popplerLink)));
        // add the ObjectRect to the container
        links.push_front(rect);
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

PDFGenerator::PDFGenerator(QObject *parent, const QVariantList &args)
    : Generator(parent, args)
    , pdfdoc(nullptr)
    , docSynopsisDirty(true)
    , xrefReconstructed(false)
    , docEmbeddedFilesDirty(true)
    , nextFontPage(0)
    , annotProxy(nullptr)
    , certStore(nullptr)
{
    setFeature(Threaded);
    setFeature(TextExtraction);
    setFeature(FontInfo);
#ifdef Q_OS_WIN32
    setFeature(PrintNative);
#else
    setFeature(PrintPostscript);
#endif
    if (Okular::FilePrinter::ps2pdfAvailable()) {
        setFeature(PrintToFile);
    }
    setFeature(ReadRawData);
    setFeature(TiledRendering);
    setFeature(SwapBackingFile);
    setFeature(SupportsCancelling);

    // You only need to do it once not for each of the documents but it is cheap enough
    // so doing it all the time won't hurt either
    Poppler::setDebugErrorFunction(PDFGeneratorPopplerDebugFunction, QVariant());
    if (!PDFSettings::useDefaultCertDB()) {
        Poppler::setNSSDir(QUrl(PDFSettings::dBCertificatePath()).toLocalFile());
    }
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 06, 0)
    auto activeBackend = PDFSettingsWidget::settingStringToPopplerEnum(PDFSettings::signatureBackend());
    if (activeBackend) {
        Poppler::setActiveCryptoSignBackend(activeBackend.value());
    }
#endif
}

PDFGenerator::~PDFGenerator()
{
    delete pdfOptionsPage;
    delete certStore;
    for (auto it = m_additionalDocumentActions.begin(); it != m_additionalDocumentActions.end(); it++) {
        delete it.value();
    }
}

// BEGIN Generator inherited functions
Okular::Document::OpenResult PDFGenerator::loadDocumentWithPassword(const QString &filePath, QVector<Okular::Page *> &pagesVector, const QString &password)
{
#ifndef NDEBUG
    if (pdfdoc) {
        qCDebug(OkularPdfDebug) << "PDFGenerator: multiple calls to loadDocument. Check it.";
        return Okular::Document::OpenError;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::load(filePath, nullptr, nullptr);
    documentFilePath = filePath;
    return init(pagesVector, password);
}

Okular::Document::OpenResult PDFGenerator::loadDocumentFromDataWithPassword(const QByteArray &fileData, QVector<Okular::Page *> &pagesVector, const QString &password)
{
#ifndef NDEBUG
    if (pdfdoc) {
        qCDebug(OkularPdfDebug) << "PDFGenerator: multiple calls to loadDocument. Check it.";
        return Okular::Document::OpenError;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = Poppler::Document::loadFromData(fileData, nullptr, nullptr);
    documentFilePath = QString();
    return init(pagesVector, password);
}

Okular::Document::OpenResult PDFGenerator::init(QVector<Okular::Page *> &pagesVector, const QString &password)
{
    if (!pdfdoc) {
        return Okular::Document::OpenError;
    }

    if (pdfdoc->isLocked()) {
        pdfdoc->unlock(password.toLatin1(), password.toLatin1());
        documentHasPassword = !password.isEmpty();

        if (pdfdoc->isLocked()) {
            pdfdoc->unlock(password.toUtf8(), password.toUtf8());

            if (pdfdoc->isLocked()) {
                pdfdoc.reset();
                return Okular::Document::OpenNeedsPassword;
            }
        }
    } else {
        documentHasPassword = false;
    }

    xrefReconstructed = false;
    if (pdfdoc->xrefWasReconstructed()) {
        xrefReconstructionHandler();
    } else {
        std::function<void()> cb = std::bind(&PDFGenerator::xrefReconstructionHandler, this);
        pdfdoc->setXRefReconstructedCallback(cb);
    }

    // build Pages (currentPage was set -1 by deletePages)
    int pageCount = pdfdoc->numPages();
    if (pageCount < 0) {
        pdfdoc.reset();
        return Okular::Document::OpenError;
    }
    pagesVector.resize(pageCount);
    rectsGenerated.fill(false, pageCount);

    annotationsOnOpenHash.clear();

    loadPages(pagesVector, 0, false);

    // update the configuration
    reparseConfig();

    // create annotation proxy
    annotProxy = new PopplerAnnotationProxy(pdfdoc.get(), userMutex(), &annotationsOnOpenHash);

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(24, 07, 0)
    setAdditionalDocumentAction(Okular::Document::CloseDocument, createLinkFromPopplerLink(pdfdoc->additionalAction(Poppler::Document::CloseDocument)));
    setAdditionalDocumentAction(Okular::Document::SaveDocumentStart, createLinkFromPopplerLink(pdfdoc->additionalAction(Poppler::Document::SaveDocumentStart)));
    setAdditionalDocumentAction(Okular::Document::SaveDocumentFinish, createLinkFromPopplerLink(pdfdoc->additionalAction(Poppler::Document::SaveDocumentFinish)));
    setAdditionalDocumentAction(Okular::Document::PrintDocumentStart, createLinkFromPopplerLink(pdfdoc->additionalAction(Poppler::Document::PrintDocumentStart)));
    setAdditionalDocumentAction(Okular::Document::PrintDocumentFinish, createLinkFromPopplerLink(pdfdoc->additionalAction(Poppler::Document::PrintDocumentFinish)));
#endif
    // the file has been loaded correctly
    return Okular::Document::OpenSuccess;
}

void PDFGenerator::setAdditionalDocumentAction(Okular::Document::DocumentAdditionalActionType type, Okular::Action *action)
{
    if (m_additionalDocumentActions.contains(type)) {
        delete m_additionalDocumentActions.value(type);
    }
    m_additionalDocumentActions.insert(type, action);
}

Okular::Action *PDFGenerator::additionalDocumentAction(Okular::Document::DocumentAdditionalActionType type)
{
    if (m_additionalDocumentActions.contains(type)) {
        return m_additionalDocumentActions.value(type);
    }
    return nullptr;
}

PDFGenerator::SwapBackingFileResult PDFGenerator::swapBackingFile(QString const &newFileName, QVector<Okular::Page *> &newPagesVector)
{
    const QBitArray oldRectsGenerated = rectsGenerated;

    doCloseDocument();
    auto openResult = loadDocumentWithPassword(newFileName, newPagesVector, QString());
    if (openResult != Okular::Document::OpenSuccess) {
        return SwapBackingFileError;
    }

    // Recreate links if needed since they are done on image() and image() is not called when swapping the file
    // since the page is already rendered
    if (oldRectsGenerated.count() == rectsGenerated.count()) {
        for (int i = 0; i < oldRectsGenerated.count(); ++i) {
            if (oldRectsGenerated[i]) {
                Okular::Page *page = newPagesVector[i];

                std::unique_ptr<Poppler::Page> pp = pdfdoc->page(i);
                if (pp) {
                    page->setObjectRects(generateLinks(pp->links()));
                    rectsGenerated[i] = true;
                    resolveMediaLinkReferences(page);
                }
            }
        }
    }

    return SwapBackingFileReloadInternalData;
}

bool PDFGenerator::doCloseDocument()
{
    // remove internal objects
    userMutex()->lock();
    delete annotProxy;
    annotProxy = nullptr;
    pdfdoc = nullptr;
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

void PDFGenerator::loadPages(QVector<Okular::Page *> &pagesVector, int rotation, bool clear)
{
    // TODO XPDF 3.01 check
    const int count = pagesVector.count();
    double w = 0, h = 0;
    for (int i = 0; i < count; i++) {
        // get xpdf page
        std::unique_ptr<Poppler::Page> p = pdfdoc->page(i);
        Okular::Page *page;
        if (p) {
            const QSizeF pSize = p->pageSizeF();
            w = pSize.width() / 72.0 * dpi().width();
            h = pSize.height() / 72.0 * dpi().height();
            Okular::Rotation orientation = Okular::Rotation0;
            switch (p->orientation()) {
            case Poppler::Page::Landscape:
                orientation = Okular::Rotation90;
                break;
            case Poppler::Page::UpsideDown:
                orientation = Okular::Rotation180;
                break;
            case Poppler::Page::Seascape:
                orientation = Okular::Rotation270;
                break;
            case Poppler::Page::Portrait:
                orientation = Okular::Rotation0;
                break;
            }
            if (rotation % 2 == 1) {
                std::swap(w, h);
            }
            // init a Okular::page, add transition and annotation information
            page = new Okular::Page(i, w, h, orientation);
            addTransition(p.get(), page);
            if (true) { // TODO real check
                addAnnotations(p.get(), page);
            }
            std::unique_ptr<Poppler::Link> tmplink = p->action(Poppler::Page::Opening);
            if (tmplink) {
                page->setPageAction(Okular::Page::Opening, createLinkFromPopplerLink(std::move(tmplink)));
            }
            tmplink = p->action(Poppler::Page::Closing);
            if (tmplink) {
                page->setPageAction(Okular::Page::Closing, createLinkFromPopplerLink(tmplink.get()));
            }
            page->setDuration(p->duration());
            page->setLabel(p->label());

            QList<Okular::FormField *> okularFormFields;
            if (i > 0) { // for page 0 we handle the form fields at the end
                okularFormFields = getFormFields(p.get());
            }
            if (!okularFormFields.isEmpty()) {
                page->setFormFields(okularFormFields);
            }
            // qWarning(PDFDebug).nospace() << page->width() << "x" << page->height();

#ifdef PDFGENERATOR_DEBUG
            qCDebug(OkularPdfDebug) << "load page" << i << "with rotation" << rotation << "and orientation" << orientation;
#endif
            if (clear && pagesVector[i]) {
                delete pagesVector[i];
            }
        } else {
            page = new Okular::Page(i, defaultPageWidth, defaultPageHeight, Okular::Rotation0);
        }
        // set the Okular::page at the right position in document's pages vector
        pagesVector[i] = page;
    }

    // Once we've added the signatures to all pages except page 0, we add all the missing signatures there
    // we do that because there's signatures that don't belong to any page, but okular needs a page<->signature mapping
    if (count > 0) {
        std::vector<std::unique_ptr<Poppler::FormFieldSignature>> allSignatures = pdfdoc->signatures();
        std::unique_ptr<Poppler::Page> page0(pdfdoc->page(0));
        QList<Okular::FormField *> page0FormFields = getFormFields(page0.get());

        for (auto &s : allSignatures) {
            bool createSignature = true;
            const QString fullyQualifiedName = s->fullyQualifiedName();
            auto compareSignatureByFullyQualifiedName = [&fullyQualifiedName](const Okular::FormField *off) { return off->fullyQualifiedName() == fullyQualifiedName; };

            // See if the signature is in one of the already loaded page (i.e. 1 to end)
            for (Okular::Page *p : std::as_const(pagesVector)) {
                const QList<Okular::FormField *> pageFormFields = p->formFields();
                if (std::find_if(pageFormFields.begin(), pageFormFields.end(), compareSignatureByFullyQualifiedName) != pageFormFields.end()) {
                    createSignature = false;
                    break;
                }
            }
            // See if the signature is in page 0
            if (createSignature && std::find_if(page0FormFields.constBegin(), page0FormFields.constEnd(), compareSignatureByFullyQualifiedName) != page0FormFields.constEnd()) {
                createSignature = false;
            }
            // Otherwise it's a page-less signature, add it to page 0
            if (createSignature) {
                Okular::FormField *of = new PopplerFormFieldSignature(std::move(s));
                page0FormFields.append(of);
            }
        }

        if (!page0FormFields.isEmpty()) {
            pagesVector[0]->setFormFields(page0FormFields);
        }
    }
}

Okular::DocumentInfo PDFGenerator::generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const
{
    Okular::DocumentInfo docInfo;
    docInfo.set(Okular::DocumentInfo::MimeType, QStringLiteral("application/pdf"));

    userMutex()->lock();

    if (pdfdoc) {
        // compile internal structure reading properties from PDFDoc
        if (keys.contains(Okular::DocumentInfo::Title)) {
            docInfo.set(Okular::DocumentInfo::Title, pdfdoc->info(QStringLiteral("Title")));
        }
        if (keys.contains(Okular::DocumentInfo::Subject)) {
            docInfo.set(Okular::DocumentInfo::Subject, pdfdoc->info(QStringLiteral("Subject")));
        }
        if (keys.contains(Okular::DocumentInfo::Author)) {
            docInfo.set(Okular::DocumentInfo::Author, pdfdoc->info(QStringLiteral("Author")));
        }
        if (keys.contains(Okular::DocumentInfo::Keywords)) {
            docInfo.set(Okular::DocumentInfo::Keywords, pdfdoc->info(QStringLiteral("Keywords")));
        }
        if (keys.contains(Okular::DocumentInfo::Creator)) {
            docInfo.set(Okular::DocumentInfo::Creator, pdfdoc->info(QStringLiteral("Creator")));
        }
        if (keys.contains(Okular::DocumentInfo::Producer)) {
            docInfo.set(Okular::DocumentInfo::Producer, pdfdoc->info(QStringLiteral("Producer")));
        }
        if (keys.contains(Okular::DocumentInfo::CreationDate)) {
            docInfo.set(Okular::DocumentInfo::CreationDate, QLocale().toString(pdfdoc->date(QStringLiteral("CreationDate")), QLocale::LongFormat));
        }
        if (keys.contains(Okular::DocumentInfo::ModificationDate)) {
            docInfo.set(Okular::DocumentInfo::ModificationDate, QLocale().toString(pdfdoc->date(QStringLiteral("ModDate")), QLocale::LongFormat));
        }
        if (keys.contains(Okular::DocumentInfo::CustomKeys)) {
            int major, minor;
            auto version = pdfdoc->getPdfVersion();
            major = version.major;
            minor = version.minor;
            docInfo.set(QStringLiteral("format"), i18nc("PDF v. <version>", "PDF v. %1.%2", major, minor), i18n("Format"));
            docInfo.set(QStringLiteral("encryption"), pdfdoc->isEncrypted() ? i18n("Encrypted") : i18n("Unencrypted"), i18n("Security"));
            docInfo.set(QStringLiteral("optimization"), pdfdoc->isLinearized() ? i18n("Yes") : i18n("No"), i18n("Optimized"));
        }

        docInfo.set(Okular::DocumentInfo::Pages, QString::number(pdfdoc->numPages()));
    }
    userMutex()->unlock();

    return docInfo;
}

const Okular::DocumentSynopsis *PDFGenerator::generateDocumentSynopsis()
{
    if (!docSynopsisDirty) {
        return &docSyn;
    }

    if (!pdfdoc) {
        return nullptr;
    }

    userMutex()->lock();
    const QVector<Poppler::OutlineItem> outline = pdfdoc->outline();
    userMutex()->unlock();

    if (outline.isEmpty()) {
        return nullptr;
    }

    addSynopsisChildren(outline, &docSyn);

    docSynopsisDirty = false;
    return &docSyn;
}

static Okular::FontInfo::FontType convertPopplerFontInfoTypeToOkularFontInfoType(Poppler::FontInfo::Type type)
{
    switch (type) {
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
    default:;
    }
    return Okular::FontInfo::Unknown;
}

static Okular::FontInfo::EmbedType embedTypeForPopplerFontInfo(const Poppler::FontInfo &fi)
{
    Okular::FontInfo::EmbedType ret = Okular::FontInfo::NotEmbedded;
    if (fi.isEmbedded()) {
        if (fi.isSubset()) {
            ret = Okular::FontInfo::EmbeddedSubset;
        } else {
            ret = Okular::FontInfo::FullyEmbedded;
        }
    }
    return ret;
}

Okular::FontInfo::List PDFGenerator::fontsForPage(int page)
{
    Okular::FontInfo::List list;

    if (page != nextFontPage) {
        return list;
    }

    QList<Poppler::FontInfo> fonts;
    userMutex()->lock();

    {
        std::unique_ptr<Poppler::FontIterator> it = pdfdoc->newFontIterator(page);
        if (it->hasNext()) {
            fonts = it->next();
        }
    }
    userMutex()->unlock();

    for (const Poppler::FontInfo &font : std::as_const(fonts)) {
        Okular::FontInfo of;
        of.setName(font.name());
        of.setSubstituteName(font.substituteName());
        of.setType(convertPopplerFontInfoTypeToOkularFontInfoType(font.type()));
        of.setEmbedType(embedTypeForPopplerFontInfo(font));
        of.setFile(font.file());
        of.setCanBeExtracted(of.embedType() != Okular::FontInfo::NotEmbedded);

        QVariant nativeId;
        nativeId.setValue(font);
        of.setNativeId(nativeId);

        list.append(of);
    }

    ++nextFontPage;

    return list;
}

const QList<Okular::EmbeddedFile *> *PDFGenerator::embeddedFiles() const
{
    if (docEmbeddedFilesDirty) {
        userMutex()->lock();
        const QList<Poppler::EmbeddedFile *> &popplerFiles = pdfdoc->embeddedFiles();
        for (Poppler::EmbeddedFile *pef : popplerFiles) {
            docEmbeddedFiles.append(new PDFEmbeddedFile(pef));
        }
        userMutex()->unlock();

        docEmbeddedFilesDirty = false;
    }

    return &docEmbeddedFiles;
}

PDFGenerator::PageLayout PDFGenerator::defaultPageLayout() const
{
    Poppler::Document::PageLayout defaultValue = pdfdoc->pageLayout();

    switch (defaultValue) {
    case Poppler::Document::OneColumn:
    case Poppler::Document::SinglePage:
        return PDFGenerator::SinglePage;
    case Poppler::Document::TwoColumnLeft:
    case Poppler::Document::TwoPageLeft:
        return PDFGenerator::TwoPageLeft;
    case Poppler::Document::TwoPageRight:
    case Poppler::Document::TwoColumnRight:
        return PDFGenerator::TwoPageRight;
    case Poppler::Document::NoLayout:
        return PDFGenerator::NoLayout;
    }
    return PDFGenerator::NoLayout;
}

bool PDFGenerator::defaultPageContinuous() const
{
    Poppler::Document::PageLayout defaultValue = pdfdoc->pageLayout();

    if ((defaultValue == Poppler::Document::OneColumn) || (defaultValue == Poppler::Document::TwoColumnLeft) || (defaultValue == Poppler::Document::TwoColumnRight)) {
        return true;
    } else {
        return false;
    }
}

QAbstractItemModel *PDFGenerator::layersModel() const
{
    return pdfdoc->hasOptionalContent() ? pdfdoc->optionalContentModel() : nullptr;
}

Okular::BackendOpaqueAction::OpaqueActionResult PDFGenerator::opaqueAction(const Okular::BackendOpaqueAction *action)
{
    const Poppler::Link *popplerLink = static_cast<const Poppler::Link *>(action->nativeHandle());
    if (const Poppler::LinkOCGState *ocgLink = dynamic_cast<const Poppler::LinkOCGState *>(popplerLink)) {
        pdfdoc->optionalContentModel()->applyLink(const_cast<Poppler::LinkOCGState *>(ocgLink));
        return Okular::BackendOpaqueAction::DoNothing;
    }
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(24, 07, 0)
    else if (const Poppler::LinkResetForm *resetFormLink = dynamic_cast<const Poppler::LinkResetForm *>(popplerLink)) {
        pdfdoc->applyResetFormsLink(*resetFormLink);
        return Okular::BackendOpaqueAction::RefreshForms;
    }
#endif
    return Okular::BackendOpaqueAction::DoNothing;
}

bool PDFGenerator::isAllowed(Okular::Permission permission) const
{
    bool b = true;
    switch (permission) {
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
    default:;
    }
    return b;
}

struct RenderImagePayload {
    RenderImagePayload(PDFGenerator *g, Okular::PixmapRequest *r)
        : generator(g)
        , request(r)
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
Q_DECLARE_METATYPE(RenderImagePayload *)

static bool shouldDoPartialUpdateCallback(const QVariant &vPayload)
{
    auto payload = vPayload.value<RenderImagePayload *>();

    // Since the timer lives in a thread without an event loop we need to stop it ourselves
    // when the remaining time has reached 0
    if (payload->timer.isActive() && payload->timer.remainingTime() == 0) {
        payload->timer.stop();
    }

    return !payload->timer.isActive();
}

static void partialUpdateCallback(const QImage &image, const QVariant &vPayload)
{
    auto payload = vPayload.value<RenderImagePayload *>();
    // clang-format off
    // Otherwise the Okular::PixmapRequest* gets turned into Okular::PixmapRequest * that is not normalized and is slightly slower
    QMetaObject::invokeMethod(payload->generator, "signalPartialPixmapRequest", Qt::QueuedConnection, Q_ARG(Okular::PixmapRequest*, payload->request), Q_ARG(QImage, image));
    // clang-format on
}

static bool shouldAbortRenderCallback(const QVariant &vPayload)
{
    auto payload = vPayload.value<RenderImagePayload *>();
    return payload->request->shouldAbortRender();
}

QImage PDFGenerator::image(Okular::PixmapRequest *request)
{
    // debug requests to this (xpdf) generator
    // qCDebug(OkularPdfDebug) << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "].";

    // compute dpi used to get an image with desired width and height
    Okular::Page *page = request->page();

    double pageWidth = page->width(), pageHeight = page->height();

    if (page->rotation() % 2) {
        std::swap(pageWidth, pageHeight);
    }

    qreal fakeDpiX = request->width() / pageWidth * dpi().width();
    qreal fakeDpiY = request->height() / pageHeight * dpi().height();

    // generate links rects only the first time
    bool genObjectRects = !rectsGenerated.at(page->number());

    // 0. LOCK [waits for the thread end]
    userMutex()->lock();

    if (request->shouldAbortRender()) {
        userMutex()->unlock();
        return QImage();
    }

    // 1. Set OutputDev parameters and Generate contents
    // note: thread safety is set on 'false' for the GUI (this) thread
    std::unique_ptr<Poppler::Page> p = pdfdoc->page(page->number());

    // 2. Take data from outputdev and attach it to the Page
    QImage img;
    if (p) {
        if (request->isTile()) {
            const QRect rect = request->normalizedRect().geometry(request->width(), request->height());
            if (request->partialUpdatesWanted()) {
                RenderImagePayload payload(this, request);
                img = p->renderToImage(
                    fakeDpiX, fakeDpiY, rect.x(), rect.y(), rect.width(), rect.height(), Poppler::Page::Rotate0, partialUpdateCallback, shouldDoPartialUpdateCallback, shouldAbortRenderCallback, QVariant::fromValue(&payload));
            } else {
                RenderImagePayload payload(this, request);
                img = p->renderToImage(fakeDpiX, fakeDpiY, rect.x(), rect.y(), rect.width(), rect.height(), Poppler::Page::Rotate0, nullptr, nullptr, shouldAbortRenderCallback, QVariant::fromValue(&payload));
            }
        } else {
            if (request->partialUpdatesWanted()) {
                RenderImagePayload payload(this, request);
                img = p->renderToImage(fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0, partialUpdateCallback, shouldDoPartialUpdateCallback, shouldAbortRenderCallback, QVariant::fromValue(&payload));
            } else {
                RenderImagePayload payload(this, request);
                img = p->renderToImage(fakeDpiX, fakeDpiY, -1, -1, -1, -1, Poppler::Page::Rotate0, nullptr, nullptr, shouldAbortRenderCallback, QVariant::fromValue(&payload));
            }
        }
    } else {
        img = QImage(request->width(), request->height(), QImage::Format_Mono);
        img.fill(Qt::white);
    }

    if (p && genObjectRects) {
        // TODO previously we extracted Image type rects too, but that needed porting to poppler
        // and as we are not doing anything with Image type rects i did not port it, have a look at
        // dead gp_outputdev.cpp on image extraction
        page->setObjectRects(generateLinks(p->links()));
        rectsGenerated[request->page()->number()] = true;

        resolveMediaLinkReferences(page);
    }

    // 3. UNLOCK [re-enables shared access]
    userMutex()->unlock();

    return img;
}

template<typename PopplerLinkType, typename OkularLinkType, typename PopplerAnnotationType, typename OkularAnnotationType>
void resolveMediaLinks(Okular::Action *action, enum Okular::Annotation::SubType subType, QHash<Okular::Annotation *, Poppler::Annotation *> &annotationsHash)
{
    OkularLinkType *okularAction = static_cast<OkularLinkType *>(action);

    const PopplerLinkType *popplerLink = static_cast<const PopplerLinkType *>(action->nativeHandle());

    QHashIterator<Okular::Annotation *, Poppler::Annotation *> it(annotationsHash);
    while (it.hasNext()) {
        it.next();

        if (it.key()->subType() == subType) {
            const PopplerAnnotationType *popplerAnnotation = static_cast<const PopplerAnnotationType *>(it.value());

            if (popplerLink->isReferencedAnnotation(popplerAnnotation)) {
                okularAction->setAnnotation(static_cast<OkularAnnotationType *>(it.key()));
                okularAction->setNativeHandle({});
                break;
            }
        }
    }
}

void PDFGenerator::resolveMediaLinkReference(Okular::Action *action)
{
    if (!action) {
        return;
    }

    if ((action->actionType() != Okular::Action::Movie) && (action->actionType() != Okular::Action::Rendition)) {
        return;
    }

    resolveMediaLinks<Poppler::LinkMovie, Okular::MovieAction, Poppler::MovieAnnotation, Okular::MovieAnnotation>(action, Okular::Annotation::AMovie, annotationsOnOpenHash);
    resolveMediaLinks<Poppler::LinkRendition, Okular::RenditionAction, Poppler::ScreenAnnotation, Okular::ScreenAnnotation>(action, Okular::Annotation::AScreen, annotationsOnOpenHash);
}

void PDFGenerator::resolveMediaLinkReferences(Okular::Page *page)
{
    resolveMediaLinkReference(const_cast<Okular::Action *>(page->pageAction(Okular::Page::Opening)));
    resolveMediaLinkReference(const_cast<Okular::Action *>(page->pageAction(Okular::Page::Closing)));

    const QList<Okular::Annotation *> annotations = page->annotations();
    for (Okular::Annotation *annotation : annotations) {
        if (annotation->subType() == Okular::Annotation::AScreen) {
            Okular::ScreenAnnotation *screenAnnotation = static_cast<Okular::ScreenAnnotation *>(annotation);
            resolveMediaLinkReference(screenAnnotation->additionalAction(Okular::Annotation::PageOpening));
            resolveMediaLinkReference(screenAnnotation->additionalAction(Okular::Annotation::PageClosing));
        }

        if (annotation->subType() == Okular::Annotation::AWidget) {
            Okular::WidgetAnnotation *widgetAnnotation = static_cast<Okular::WidgetAnnotation *>(annotation);
            resolveMediaLinkReference(widgetAnnotation->additionalAction(Okular::Annotation::PageOpening));
            resolveMediaLinkReference(widgetAnnotation->additionalAction(Okular::Annotation::PageClosing));
        }
    }

    const QList<Okular::FormField *> fields = page->formFields();
    for (Okular::FormField *field : fields) {
        resolveMediaLinkReference(field->activationAction());
    }
}

struct TextExtractionPayload {
    explicit TextExtractionPayload(Okular::TextRequest *r)
        : request(r)
    {
    }

    Okular::TextRequest *request;
};
Q_DECLARE_METATYPE(TextExtractionPayload *)

static bool shouldAbortTextExtractionCallback(const QVariant &vPayload)
{
    auto payload = vPayload.value<TextExtractionPayload *>();
    return payload->request->shouldAbortExtraction();
}

Okular::TextPage *PDFGenerator::textPage(Okular::TextRequest *request)
{
    const Okular::Page *page = request->page();
#ifdef PDFGENERATOR_DEBUG
    qCDebug(OkularPdfDebug) << "page" << page->number();
#endif
    // build a TextList...
    std::vector<std::unique_ptr<Poppler::TextBox>> textList;
    double pageWidth, pageHeight;
    userMutex()->lock();
    if (request->shouldAbortExtraction()) {
        userMutex()->unlock();
        return nullptr;
    }
    std::unique_ptr<Poppler::Page> pp = pdfdoc->page(page->number());
    if (pp) {
        TextExtractionPayload payload(request);
        textList = pp->textList(Poppler::Page::Rotate0, shouldAbortTextExtractionCallback, QVariant::fromValue(&payload));
        const QSizeF s = pp->pageSizeF();
        pageWidth = s.width();
        pageHeight = s.height();
    } else {
        pageWidth = defaultPageWidth;
        pageHeight = defaultPageHeight;
    }
    userMutex()->unlock();

    if (textList.empty() && request->shouldAbortExtraction()) {
        return nullptr;
    }

    Okular::TextPage *tp = abstractTextPage(textList, pageHeight, pageWidth, (Poppler::Page::Rotation)page->orientation());
    return tp;
}

QByteArray PDFGenerator::requestFontData(const Okular::FontInfo &font)
{
    Poppler::FontInfo fi = font.nativeId().value<Poppler::FontInfo>();
    return pdfdoc->fontData(fi);
}

void PDFGenerator::okularToPoppler(const Okular::NewSignatureData &oData, Poppler::PDFConverter::NewSignatureData *pData)
{
    pData->setCertNickname(oData.certNickname());
    pData->setPassword(oData.password());
    pData->setPage(oData.page());
    const QString datetime = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss t"));
    pData->setSignatureText(i18n("Signed by: %1\n\nDate: %2", oData.certSubjectCommonName(), datetime));
    pData->setSignatureLeftText(oData.certSubjectCommonName());
    pData->setFontSize(oData.fontSize());
    pData->setLeftFontSize(oData.leftFontSize());
    const Okular::NormalizedRect bRect = oData.boundingRectangle();
    pData->setBoundingRectangle({bRect.left, bRect.top, bRect.width(), bRect.height()});
    pData->setFontColor(Qt::black);
    pData->setBorderColor(Qt::black);
    pData->setReason(oData.reason());
    pData->setLocation(oData.location());
    pData->setDocumentOwnerPassword(oData.documentPassword().toLatin1());
    pData->setDocumentUserPassword(oData.documentPassword().toLatin1());
}

#define DUMMY_QPRINTER_COPY
Okular::Document::PrintError PDFGenerator::print(QPrinter &printer)
{
    bool printAnnots = true;
    bool forceRasterize = false;
    PDFOptionsPage::ScaleMode scaleMode = PDFOptionsPage::FitToPrintableArea;

    if (pdfOptionsPage) {
        printAnnots = pdfOptionsPage->printAnnots();
        forceRasterize = pdfOptionsPage->printForceRaster();
        scaleMode = pdfOptionsPage->scaleMode();
    }

    const auto overprintPreviewEnabled = PDFSettings::overprintPreviewEnabled();

#ifdef Q_OS_WIN
    // Windows can only print by rasterization, because that is
    // currently the only way Okular implements printing without using UNIX-specific
    // tools like 'lpr'.
    forceRasterize = true;
#endif

    if (forceRasterize) {
        pdfdoc->setRenderHint(Poppler::Document::HideAnnotations, !printAnnots);
        pdfdoc->setRenderHint(Poppler::Document::OverprintPreview, overprintPreviewEnabled);

        if (pdfOptionsPage) {
            // If requested, scale to full page instead of the printable area
            printer.setFullPage(pdfOptionsPage->ignorePrintMargins());
        }

        QPainter painter;
        painter.begin(&printer);

        QList<int> pageList = Okular::FilePrinter::pageList(printer, pdfdoc->numPages(), document()->currentPage() + 1, document()->bookmarkedPageList());
        for (int i = 0; i < pageList.count(); ++i) {
            if (i != 0) {
                printer.newPage();
            }

            const int page = pageList.at(i) - 1;
            userMutex()->lock();
            std::unique_ptr<Poppler::Page> pp(pdfdoc->page(page));
            if (pp) {
                QSizeF pageSize = pp->pageSizeF();      // Unit is 'points' (i.e., 1/72th of an inch)
                QRect painterWindow = painter.window(); // Unit is 'QPrinter::DevicePixel'

                // Default: no scaling at all, but we need to go from DevicePixel units to 'points'
                // Warning: We compute the horizontal scaling, and later assume that the vertical scaling will be the same.
                double scaling = printer.paperRect(QPrinter::DevicePixel).width() / printer.paperRect(QPrinter::Point).width();

                if (scaleMode != PDFOptionsPage::None) {
                    // Get the two scaling factors needed to fit the page onto paper horizontally or vertically
                    auto horizontalScaling = painterWindow.width() / pageSize.width();
                    auto verticalScaling = painterWindow.height() / pageSize.height();

                    // We use the smaller of the two for both directions, to keep the aspect ratio
                    scaling = std::min(horizontalScaling, verticalScaling);
                }

#ifdef Q_OS_WIN
                QImage img = pp->renderToImage(printer.physicalDpiX(), printer.physicalDpiY());
#else
                // UNIX: Same resolution as the postscript rasterizer; see discussion at https://git.reviewboard.kde.org/r/130218/
                QImage img = pp->renderToImage(300, 300);
#endif
                painter.drawImage(QRectF(QPointF(0, 0), scaling * pp->pageSizeF()), img);
            }
            userMutex()->unlock();
        }
        painter.end();
        return Okular::Document::NoPrintError;
    }

    // Check if we can send the PDF directly to the printer or need to convert to PS first
    // We need PS if:
    // - we are printing with annotations
    // - we are printing to a file (otherwise page ranges are not respected)
    // - our document isn't backed by a file
    if (!printAnnots && printer.outputFileName().isEmpty() && !documentFilePath.isEmpty()) {
        const Okular::FilePrinter::ScaleMode filePrinterScaleMode = (scaleMode == PDFOptionsPage::None) ? Okular::FilePrinter::ScaleMode::NoScaling : Okular::FilePrinter::ScaleMode::FitToPrintArea;

        return Okular::FilePrinter::printFile(
            printer, documentFilePath, document()->orientation(), Okular::FilePrinter::ApplicationDeletesFiles, Okular::FilePrinter::SystemSelectsPages, document()->bookmarkedPageRange(), filePrinterScaleMode);
    }

#ifdef DUMMY_QPRINTER_COPY
    // Get the real page size to pass to the ps generator
    QPrinter dummy(QPrinter::PrinterResolution);
    dummy.setFullPage(true);
    dummy.setPageOrientation(printer.pageLayout().orientation());
    dummy.setPageSize(printer.pageLayout().pageSize());
    int width = dummy.width();
    int height = dummy.height();
#else
    int width = printer.width();
    int height = printer.height();
#endif

    if (width <= 0 || height <= 0) {
        return Okular::Document::InvalidPageSizePrintError;
    }

    // Create the tempfile to send to FilePrinter, which will manage the deletion
    QTemporaryFile tf(QDir::tempPath() + QLatin1String("/okular_XXXXXX.ps"));
    if (!tf.open()) {
        return Okular::Document::TemporaryFileOpenPrintError;
    }
    QString tempfilename = tf.fileName();

    // Generate the list of pages to be printed as selected in the print dialog
    QList<int> pageList = Okular::FilePrinter::pageList(printer, pdfdoc->numPages(), document()->currentPage() + 1, document()->bookmarkedPageList());

    // TODO rotation

    tf.setAutoRemove(false);

    QString pstitle = metaData(QStringLiteral("Title"), QVariant()).toString();
    if (pstitle.trimmed().isEmpty()) {
        pstitle = document()->currentDocument().fileName();
    }

    std::unique_ptr<Poppler::PSConverter> psConverter = pdfdoc->psConverter();

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

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 9, 0)
    const auto isPdfOutput = printer.outputFormat() == QPrinter::PdfFormat;
    psConverter->setForceOverprintPreview(!isPdfOutput && overprintPreviewEnabled);
#endif

    if (!printAnnots) {
        psConverter->setPSOptions(psConverter->psOptions() | Poppler::PSConverter::HideAnnotations);
    }

    userMutex()->lock();
    if (psConverter->convert()) {
        userMutex()->unlock();
        tf.close();

        const Okular::FilePrinter::ScaleMode filePrinterScaleMode = (scaleMode == PDFOptionsPage::None) ? Okular::FilePrinter::ScaleMode::NoScaling : Okular::FilePrinter::ScaleMode::FitToPrintArea;

        return Okular::FilePrinter::printFile(printer, tempfilename, document()->orientation(), Okular::FilePrinter::SystemDeletesFiles, Okular::FilePrinter::ApplicationSelectsPages, document()->bookmarkedPageRange(), filePrinterScaleMode);
    } else {
        userMutex()->unlock();

        tf.close();

        return Okular::Document::FileConversionPrintError;
    }
}

QVariant PDFGenerator::metaData(const QString &key, const QVariant &option) const
{
    if (key == QLatin1String("StartFullScreen")) {
        QMutexLocker ml(userMutex());
        // asking for the 'start in fullscreen mode' (pdf property)
        if (pdfdoc->pageMode() == Poppler::Document::FullScreen) {
            return true;
        }
    } else if (key == QLatin1String("NamedViewport") && !option.toString().isEmpty()) {
        Okular::DocumentViewport viewport;
        QString optionString = option.toString();

        // asking for the page related to a 'named link destination'. the
        // option is the link name. @see addSynopsisChildren.
        userMutex()->lock();
        std::unique_ptr<Poppler::LinkDestination> ld = pdfdoc->linkDestination(optionString);
        userMutex()->unlock();
        if (ld) {
            fillViewportFromLinkDestination(viewport, *ld);
        }
        if (viewport.pageNumber >= 0) {
            return viewport.toString();
        }
    } else if (key == QLatin1String("DocumentTitle")) {
        userMutex()->lock();
        QString title = pdfdoc->info(QStringLiteral("Title"));
        userMutex()->unlock();
        return title;
    } else if (key == QLatin1String("OpenTOC")) {
        QMutexLocker ml(userMutex());
        if (pdfdoc->pageMode() == Poppler::Document::UseOutlines) {
            return true;
        }
    } else if (key == QLatin1String("DocumentScripts") && option.toString() == QLatin1String("JavaScript")) {
        QMutexLocker ml(userMutex());
        return pdfdoc->scripts();
    } else if (key == QLatin1String("HasUnsupportedXfaForm")) {
        QMutexLocker ml(userMutex());
        return pdfdoc->formType() == Poppler::Document::XfaForm;
    } else if (key == QLatin1String("FormCalculateOrder")) {
        QMutexLocker ml(userMutex());
        return QVariant::fromValue<QVector<int>>(pdfdoc->formCalculateOrder());
    } else if (key == QLatin1String("GeneratorExtraDescription")) {
        if (Poppler::Version::string() == QStringLiteral(POPPLER_VERSION)) {
            return i18n("Using Poppler %1", Poppler::Version::string());
        } else {
            return i18n("Using Poppler %1\n\nBuilt against Poppler %2", Poppler::Version::string(), QStringLiteral(POPPLER_VERSION));
        }
    } else if (key == QLatin1String("DocumentHasPassword")) {
        return documentHasPassword ? QStringLiteral("yes") : QStringLiteral("no");
    }
    return QVariant();
}

bool PDFGenerator::reparseConfig()
{
    if (!pdfdoc) {
        return false;
    }

    bool somethingchanged = false;
    // load paper color
    QColor color = documentMetaData(PaperColorMetaData, true).value<QColor>();
    // if paper color is changed we have to rebuild every visible pixmap in addition
    // to the outputDevice. it's the 'heaviest' case, other effect are just recoloring
    // over the page rendered on 'standard' white background.
    if (color != pdfdoc->paperColor()) {
        userMutex()->lock();
        pdfdoc->setPaperColor(color);
        userMutex()->unlock();
        somethingchanged = true;
    }
    bool aaChanged = setDocumentRenderHints();
    somethingchanged = somethingchanged || aaChanged;
    return somethingchanged;
}

void PDFGenerator::addPages(KConfigDialog *dlg)
{
    PDFSettingsWidget *w = new PDFSettingsWidget(dlg);

    dlg->addPage(w, PDFSettings::self(), i18n("PDF"), QStringLiteral("application-pdf"), i18n("PDF Backend Configuration"));
}

bool PDFGenerator::setDocumentRenderHints()
{
    bool changed = false;
    const Poppler::Document::RenderHints oldhints = pdfdoc->renderHints();
#define SET_HINT(hintname, hintdefvalue, hintflag)                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                          \
        bool newhint = documentMetaData(hintname, hintdefvalue).toBool();                                                                                                                                                                      \
        if (newhint != oldhints.testFlag(hintflag)) {                                                                                                                                                                                          \
            pdfdoc->setRenderHint(hintflag, newhint);                                                                                                                                                                                          \
            changed = true;                                                                                                                                                                                                                    \
        }                                                                                                                                                                                                                                      \
    }
    SET_HINT(GraphicsAntialiasMetaData, true, Poppler::Document::Antialiasing)
    SET_HINT(TextAntialiasMetaData, true, Poppler::Document::TextAntialiasing)
    SET_HINT(TextHintingMetaData, false, Poppler::Document::TextHinting)
#undef SET_HINT
    // load thin line mode
    const int thinLineMode = PDFSettings::enhanceThinLines();
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 07, 0)
    const bool enableOverprintPreview = PDFSettings::overprintPreviewEnabled();
#endif
    const bool enableThinLineSolid = thinLineMode == PDFSettings::EnumEnhanceThinLines::Solid;
    const bool enableShapeLineSolid = thinLineMode == PDFSettings::EnumEnhanceThinLines::Shape;

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 07, 0)
    const bool overprintPreviewWasEnabled = (oldhints & Poppler::Document::OverprintPreview) == Poppler::Document::OverprintPreview;
#endif
    const bool thinLineSolidWasEnabled = (oldhints & Poppler::Document::ThinLineSolid) == Poppler::Document::ThinLineSolid;
    const bool thinLineShapeWasEnabled = (oldhints & Poppler::Document::ThinLineShape) == Poppler::Document::ThinLineShape;

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 07, 0)
    if (enableOverprintPreview != overprintPreviewWasEnabled) {
        pdfdoc->setRenderHint(Poppler::Document::OverprintPreview, enableOverprintPreview);
        changed = true;
    }
#endif
    if (enableThinLineSolid != thinLineSolidWasEnabled) {
        pdfdoc->setRenderHint(Poppler::Document::ThinLineSolid, enableThinLineSolid);
        changed = true;
    }
    if (enableShapeLineSolid != thinLineShapeWasEnabled) {
        pdfdoc->setRenderHint(Poppler::Document::ThinLineShape, enableShapeLineSolid);
        changed = true;
    }
    return changed;
}

Okular::ExportFormat::List PDFGenerator::exportFormats() const
{
    static Okular::ExportFormat::List formats;
    if (formats.isEmpty()) {
        formats.append(Okular::ExportFormat::standardFormat(Okular::ExportFormat::PlainText));
    }

    return formats;
}

bool PDFGenerator::exportTo(const QString &fileName, const Okular::ExportFormat &format)
{
    if (format.mimeType().inherits(QStringLiteral("text/plain"))) {
        QFile f(fileName);
        if (!f.open(QIODevice::WriteOnly)) {
            return false;
        }

        QTextStream ts(&f);
        int num = document()->pages();
        for (int i = 0; i < num; ++i) {
            QString text;
            userMutex()->lock();
            std::unique_ptr<Poppler::Page> pp = pdfdoc->page(i);
            if (pp) {
                text = pp->text(QRect()).normalized(QString::NormalizationForm_C);
            }
            userMutex()->unlock();
            ts << text;
        }
        f.close();

        return true;
    }

    return false;
}

// END Generator inherited functions

inline void append(Okular::TextPage *ktp, const QString &s, double l, double b, double r, double t)
{
    //    qWarning(PDFDebug).nospace() << "text: " << s << " at (" << l << "," << t << ")x(" << r <<","<<b<<")";
    ktp->append(s, Okular::NormalizedRect(l, t, r, b));
}

Okular::TextPage *PDFGenerator::abstractTextPage(const std::vector<std::unique_ptr<Poppler::TextBox>> &text, double height, double width, int rot)
{
    Q_UNUSED(rot);
    Okular::TextPage *ktp = new Okular::TextPage;
    Poppler::TextBox *next;
#ifdef PDFGENERATOR_DEBUG
    qCDebug(OkularPdfDebug) << "getting text page in generator pdf - rotation:" << rot;
#endif
    QString s;
    bool addChar;
    for (const auto &word : text) {
        const int qstringCharCount = word->text().length();
        next = word->nextWord();
        int textBoxChar = 0;
        for (int j = 0; j < qstringCharCount; j++) {
            const QChar c = word->text().at(j);
            if (c.isHighSurrogate()) {
                s = c;
                addChar = false;
            } else if (c.isLowSurrogate()) {
                s += c;
                addChar = true;
            } else {
                s = c;
                addChar = true;
            }

            if (addChar) {
                QRectF charBBox = word->charBoundingBox(textBoxChar);
                append(ktp, (j == qstringCharCount - 1 && !next) ? (s + QLatin1Char('\n')) : s, charBBox.left() / width, charBBox.bottom() / height, charBBox.right() / width, charBBox.top() / height);
                textBoxChar++;
            }
        }

        if (word->hasSpaceAfter() && next) {
            // TODO Check with a document with vertical text
            // probably won't work and we will need to do comparisons
            // between wordBBox and nextWordBBox to see if they are
            // vertically or horizontally aligned
            QRectF wordBBox = word->boundingBox();
            QRectF nextWordBBox = next->boundingBox();
            append(ktp, QStringLiteral(" "), wordBBox.right() / width, wordBBox.bottom() / height, nextWordBBox.left() / width, wordBBox.top() / height);
        }
    }
    return ktp;
}

void PDFGenerator::addSynopsisChildren(const QVector<Poppler::OutlineItem> &outlineItems, QDomNode *parentDestination)
{
    for (const Poppler::OutlineItem &outlineItem : outlineItems) {
        QDomElement item = docSyn.createElement(outlineItem.name());
        parentDestination->appendChild(item);

        item.setAttribute(QStringLiteral("ExternalFileName"), outlineItem.externalFileName());
        const QSharedPointer<const Poppler::LinkDestination> outlineDestination = outlineItem.destination();
        if (outlineDestination) {
            const QString destinationName = outlineDestination->destinationName();
            if (!destinationName.isEmpty()) {
                item.setAttribute(QStringLiteral("ViewportName"), destinationName);
            } else {
                Okular::DocumentViewport vp;
                fillViewportFromLinkDestination(vp, *outlineDestination);
                item.setAttribute(QStringLiteral("Viewport"), vp.toString());
            }
        }
        item.setAttribute(QStringLiteral("Open"), outlineItem.isOpen());
        item.setAttribute(QStringLiteral("URL"), outlineItem.uri());

        if (outlineItem.hasChildren()) {
            addSynopsisChildren(outlineItem.children(), &item);
        }
    }
}

void PDFGenerator::addAnnotations(Poppler::Page *popplerPage, Okular::Page *page)
{
    QSet<Poppler::Annotation::SubType> subtypes;
    subtypes << Poppler::Annotation::AFileAttachment << Poppler::Annotation::ASound << Poppler::Annotation::AMovie << Poppler::Annotation::AWidget << Poppler::Annotation::AScreen << Poppler::Annotation::AText << Poppler::Annotation::ALine
             << Poppler::Annotation::AGeom << Poppler::Annotation::AHighlight << Poppler::Annotation::AInk << Poppler::Annotation::AStamp << Poppler::Annotation::ACaret;

    std::vector<std::unique_ptr<Poppler::Annotation>> popplerAnnotations = popplerPage->annotations(subtypes);

    for (auto &a : popplerAnnotations) {
        bool doDelete = true;
        Okular::Annotation *newann = createAnnotationFromPopplerAnnotation(a.get(), *popplerPage, &doDelete);
        if (newann) {
            page->addAnnotation(newann);

            if (a->subType() == Poppler::Annotation::AScreen) {
                Poppler::ScreenAnnotation *annotScreen = static_cast<Poppler::ScreenAnnotation *>(a.get());
                Okular::ScreenAnnotation *screenAnnotation = static_cast<Okular::ScreenAnnotation *>(newann);

                // The activation action
                Poppler::Link *actionLink = annotScreen->action();
                if (actionLink) {
                    screenAnnotation->setAction(createLinkFromPopplerLink(actionLink));
                }

                // The additional actions
                std::unique_ptr<Poppler::Link> pageOpeningLink = annotScreen->additionalAction(Poppler::Annotation::PageOpeningAction);
                if (pageOpeningLink) {
                    screenAnnotation->setAdditionalAction(Okular::Annotation::PageOpening, createLinkFromPopplerLink(std::move(pageOpeningLink)));
                }

                std::unique_ptr<Poppler::Link> pageClosingLink = annotScreen->additionalAction(Poppler::Annotation::PageClosingAction);
                if (pageClosingLink) {
                    screenAnnotation->setAdditionalAction(Okular::Annotation::PageClosing, createLinkFromPopplerLink(std::move(pageClosingLink)));
                }
            }

            if (a->subType() == Poppler::Annotation::AWidget) {
                Poppler::WidgetAnnotation *annotWidget = static_cast<Poppler::WidgetAnnotation *>(a.get());
                Okular::WidgetAnnotation *widgetAnnotation = static_cast<Okular::WidgetAnnotation *>(newann);

                // The additional actions
                std::unique_ptr<Poppler::Link> pageOpeningLink = annotWidget->additionalAction(Poppler::Annotation::PageOpeningAction);
                if (pageOpeningLink) {
                    widgetAnnotation->setAdditionalAction(Okular::Annotation::PageOpening, createLinkFromPopplerLink(std::move(pageOpeningLink)));
                }

                std::unique_ptr<Poppler::Link> pageClosingLink = annotWidget->additionalAction(Poppler::Annotation::PageClosingAction);
                if (pageClosingLink) {
                    widgetAnnotation->setAdditionalAction(Okular::Annotation::PageClosing, createLinkFromPopplerLink(std::move(pageClosingLink)));
                }
            }

            if (!doDelete) {
                annotationsOnOpenHash.insert(newann, a.release()); // investigate
            }
        }
    }
}

void PDFGenerator::addTransition(Poppler::Page *pdfPage, Okular::Page *page)
// called on opening when MUTEX is not used
{
    Poppler::PageTransition *pdfTransition = pdfPage->transition();
    if (!pdfTransition || pdfTransition->type() == Poppler::PageTransition::Replace) {
        return;
    }

    Okular::PageTransition *transition = new Okular::PageTransition();
    switch (pdfTransition->type()) {
    case Poppler::PageTransition::Replace:
        // won't get here, added to avoid warning
        break;
    case Poppler::PageTransition::Split:
        transition->setType(Okular::PageTransition::Split);
        break;
    case Poppler::PageTransition::Blinds:
        transition->setType(Okular::PageTransition::Blinds);
        break;
    case Poppler::PageTransition::Box:
        transition->setType(Okular::PageTransition::Box);
        break;
    case Poppler::PageTransition::Wipe:
        transition->setType(Okular::PageTransition::Wipe);
        break;
    case Poppler::PageTransition::Dissolve:
        transition->setType(Okular::PageTransition::Dissolve);
        break;
    case Poppler::PageTransition::Glitter:
        transition->setType(Okular::PageTransition::Glitter);
        break;
    case Poppler::PageTransition::Fly:
        transition->setType(Okular::PageTransition::Fly);
        break;
    case Poppler::PageTransition::Push:
        transition->setType(Okular::PageTransition::Push);
        break;
    case Poppler::PageTransition::Cover:
        transition->setType(Okular::PageTransition::Cover);
        break;
    case Poppler::PageTransition::Uncover:
        transition->setType(Okular::PageTransition::Uncover);
        break;
    case Poppler::PageTransition::Fade:
        transition->setType(Okular::PageTransition::Fade);
        break;
    }

    transition->setDuration(pdfTransition->durationReal());

    switch (pdfTransition->alignment()) {
    case Poppler::PageTransition::Horizontal:
        transition->setAlignment(Okular::PageTransition::Horizontal);
        break;
    case Poppler::PageTransition::Vertical:
        transition->setAlignment(Okular::PageTransition::Vertical);
        break;
    }

    switch (pdfTransition->direction()) {
    case Poppler::PageTransition::Inward:
        transition->setDirection(Okular::PageTransition::Inward);
        break;
    case Poppler::PageTransition::Outward:
        transition->setDirection(Okular::PageTransition::Outward);
        break;
    }

    transition->setAngle(pdfTransition->angle());
    transition->setScale(pdfTransition->scale());
    transition->setIsRectangular(pdfTransition->isRectangular());

    page->setTransition(transition);
}

QList<Okular::FormField *> PDFGenerator::getFormFields(Poppler::Page *popplerPage)
{
    if (!popplerPage) {
        return {};
    }

    std::vector<std::unique_ptr<Poppler::FormField>> popplerFormFields = popplerPage->formFields();
    QList<Okular::FormField *> okularFormFields;
    for (auto &f : popplerFormFields) {
        Okular::FormField *of = nullptr;
        switch (f->type()) {
        case Poppler::FormField::FormButton:
            of = new PopplerFormFieldButton(std::unique_ptr<Poppler::FormFieldButton>(static_cast<Poppler::FormFieldButton *>(f.release())));
            break;
        case Poppler::FormField::FormText:
            of = new PopplerFormFieldText(std::unique_ptr<Poppler::FormFieldText>(static_cast<Poppler::FormFieldText *>(f.release())));
            break;
        case Poppler::FormField::FormChoice:
            of = new PopplerFormFieldChoice(std::unique_ptr<Poppler::FormFieldChoice>(static_cast<Poppler::FormFieldChoice *>(f.release())));
            break;
        case Poppler::FormField::FormSignature: {
            of = new PopplerFormFieldSignature(std::unique_ptr<Poppler::FormFieldSignature>(static_cast<Poppler::FormFieldSignature *>(f.release())));
            break;
        }
        default:;
        }
        if (of) {
            // form field created, good - it will take care of the Poppler::FormField
            okularFormFields.append(of);
        }
    }

    return okularFormFields;
}

Okular::PrintOptionsWidget *PDFGenerator::printConfigurationWidget() const
{
    if (!pdfOptionsPage) {
        const_cast<PDFGenerator *>(this)->pdfOptionsPage = new PDFOptionsPage();
    }
    return pdfOptionsPage;
}

bool PDFGenerator::supportsOption(SaveOption option) const
{
    switch (option) {
    case SaveChanges: {
        return true;
    }
    default:;
    }
    return false;
}

bool PDFGenerator::save(const QString &fileName, SaveOptions options, QString *errorText)
{
    Q_UNUSED(errorText);
    std::unique_ptr<Poppler::PDFConverter> pdfConv = pdfdoc->pdfConverter();

    pdfConv->setOutputFileName(fileName);
    if (options & SaveChanges) {
        pdfConv->setPDFOptions(pdfConv->pdfOptions() | Poppler::PDFConverter::WithChanges);
    }

    QMutexLocker locker(userMutex());

    QHashIterator<Okular::Annotation *, Poppler::Annotation *> it(annotationsOnOpenHash);
    while (it.hasNext()) {
        it.next();

        if (it.value()->uniqueName().isEmpty()) {
            it.value()->setUniqueName(it.key()->uniqueName());
        }
    }

    bool success = pdfConv->convert();
    if (!success) {
        switch (pdfConv->lastError()) {
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
    return success;
}

Okular::AnnotationProxy *PDFGenerator::annotationProxy() const
{
    return annotProxy;
}

bool PDFGenerator::canSign() const
{
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 06, 0)
    return !Poppler::availableCryptoSignBackends().empty();
#else
    return Poppler::hasNSSSupport();
#endif
}

bool PDFGenerator::sign(const Okular::NewSignatureData &oData, const QString &rFilename)
{
    // We need a temporary file to pass a prepared image to poppler
    QTemporaryFile timg(QFileInfo(rFilename).absolutePath() + QLatin1String("/okular_XXXXXX.png"));
    timg.setAutoRemove(true);
    if (!timg.open()) {
        return false;
    }

    // save to tmp file - poppler doesn't like overwriting in-place
    QTemporaryFile tf(QFileInfo(rFilename).absolutePath() + QLatin1String("/okular_XXXXXX.pdf"));
    tf.setAutoRemove(false);
    if (!tf.open()) {
        return false;
    }
    std::unique_ptr<Poppler::PDFConverter> converter(pdfdoc->pdfConverter());
    converter->setOutputFileName(tf.fileName());
    converter->setPDFOptions(converter->pdfOptions() | Poppler::PDFConverter::WithChanges);

    Poppler::PDFConverter::NewSignatureData pData;
    okularToPoppler(oData, &pData);
    if (!oData.backgroundImagePath().isEmpty() && QFile::exists(oData.backgroundImagePath())) {
        // width and height for target image
        const Okular::NormalizedRect bRect = oData.boundingRectangle();
        // 2 is an experimental decided upon fudge factor to compensate for the fact that pageSize is in points
        // but most of this ends up working in pixels anyway
        double width = pdfdoc->page(oData.page())->pageSizeF().width() * bRect.width() * 2;
        double height = pdfdoc->page(oData.page())->pageSizeF().height() * bRect.height() * 2;

        QImageReader reader(oData.backgroundImagePath());
        QSize imageSize = reader.size();
        if (!reader.size().isNull()) {
            reader.setScaledSize(imageSize.scaled(width, height, Qt::KeepAspectRatio));
        }
        auto input = reader.read();
        if (!input.isNull()) {
            auto scaled = imagescaling::scaleAndFitCanvas(input, QSize(width, height));
            bool success = scaled.save(timg.fileName(), "png");
            if (success) {
                pData.setImagePath(timg.fileName());
                pData.setBackgroundColor(Qt::white);
            }
        }
    }
    if (!converter->sign(pData)) {
        tf.remove();
        return false;
    }

    // now copy over old file
    QFile::remove(rFilename);
    if (!tf.rename(rFilename)) {
        return false;
    }

    return true;
}

Okular::CertificateStore *PDFGenerator::certificateStore() const
{
    if (!certStore) {
        certStore = new PopplerCertificateStore();
    }

    return certStore;
}

void PDFGenerator::xrefReconstructionHandler()
{
    if (!xrefReconstructed) {
        qCDebug(OkularPdfDebug) << "XRef Table of the document has been reconstructed";
        xrefReconstructed = true;
        Q_EMIT warning(i18n("Some errors were found in the document, Okular might not be able to show the content correctly"), 5000);
    }
}

#include "generator_pdf.moc"

Q_LOGGING_CATEGORY(OkularPdfDebug, "org.kde.okular.generators.pdf", QtWarningMsg)

/* kate: replace-tabs on; indent-width 4; */
