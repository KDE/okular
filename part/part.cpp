/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2002 Chris Cheney <ccheney@cheney.cx>
    SPDX-FileCopyrightText: 2002 Malcolm Hunter <malcolm.hunter@gmx.co.uk>
    SPDX-FileCopyrightText: 2003-2004 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Daniel Molkentin <molkentin@kde.org>
    SPDX-FileCopyrightText: 2003 Andy Goossens <andygoossens@telenet.be>
    SPDX-FileCopyrightText: 2003 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2004 Dominique Devriese <devriese@kde.org>
    SPDX-FileCopyrightText: 2004 Christoph Cullmann <crossfire@babylon2k.de>
    SPDX-FileCopyrightText: 2004 Henrique Pinto <stampede@coltec.ufmg.br>
    SPDX-FileCopyrightText: 2004 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2004-2008 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2004 Antti Markus <antti.markus@starman.ee>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "part.h"

#include "config-okular.h"

// qt/kde includes
#include <QApplication>
#include <QContextMenuEvent>
#include <QDBusConnection>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMimeDatabase>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>
#include <QWidgetAction>

#include <KAboutPluginDialog>
#include <KActionCollection>
#include <KActionMenu>
#include <KBookmarkAction>
#include <KColorSchemeManager>
#include <KDirWatch>
#include <KFilterBase>
#include <KFilterDev>
#include <KHamburgerMenu>
#include <KIO/OpenFileManagerWindowJob>
#include <KJobWidgets>
#include <KMainWindow>
#include <KMessageBox>
#include <KParts/GUIActivateEvent>
#include <KPasswordDialog>
#include <KPluginMetaData>
#include <KSharedDataCache>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KToggleFullScreenAction>
#include <KToolBar>
#include <Kdelibs4ConfigMigrator>
#include <Kdelibs4Migration>
#if HAVE_KWALLET
#include <KWallet>
#endif
#include <KXMLGUIClient>
#include <KXMLGUIFactory>

#if HAVE_PURPOSE
#include <Purpose/AlternativesModel>
#include <purpose_version.h>
#if PURPOSE_VERSION >= QT_VERSION_CHECK(5, 104, 0)
#include <Purpose/Menu>
#else
#include <PurposeWidgets/Menu>
#endif
#endif

// local includes
#include "aboutdata.h"
#include "bookmarklist.h"
#include "core/action.h"
#include "core/annotations.h"
#include "core/bookmarkmanager.h"
#include "core/document_p.h"
#include "core/fileprinter.h"
#include "core/form.h"
#include "core/generator.h"
#include "core/page.h"
#include "core/printoptionswidget.h"
#include "drawingtoolactions.h"
#include "embeddedfilesdialog.h"
#include "extensions.h"
#include "fileprinterpreview.h"
#include "findbar.h"
#include "gui/debug_ui.h"
#include "gui/guiutils.h"
#include "gui/signatureguiutils.h"
#include "layers.h"
#include "minibar.h"
#include "okmenutitle.h"
#include "pagesizelabel.h"
#include "pageview.h"
#include "preferencesdialog.h"
#include "presentationwidget.h"
#include "propertiesdialog.h"
#include "searchwidget.h"
#include "settings.h"
#include "side_reviews.h"
#include "sidebar.h"
#include "signaturepanel.h"
#include "thumbnaillist.h"
#include "toc.h"

#include <memory>
#include <type_traits>

#ifdef OKULAR_KEEP_FILE_OPEN
class FileKeeper
{
public:
    FileKeeper()
        : m_handle(nullptr)
    {
    }

    ~FileKeeper()
    {
    }

    void open(const QString &path)
    {
        if (!m_handle)
            m_handle = std::fopen(QFile::encodeName(path).constData(), "r");
    }

    void close()
    {
        if (m_handle) {
            int ret = std::fclose(m_handle);
            Q_UNUSED(ret)
            m_handle = nullptr;
        }
    }

    QTemporaryFile *copyToTemporary() const
    {
        if (!m_handle)
            return nullptr;

        QTemporaryFile *retFile = new QTemporaryFile;
        retFile->open();

        std::rewind(m_handle);
        int c = -1;
        do {
            c = std::fgetc(m_handle);
            if (c == EOF)
                break;
            if (!retFile->putChar((char)c))
                break;
        } while (!feof(m_handle));

        retFile->flush();

        return retFile;
    }

private:
    std::FILE *m_handle;
};
#endif

K_PLUGIN_FACTORY(OkularPartFactory, registerPlugin<Okular::Part>();)

static QAction *actionForExportFormat(const Okular::ExportFormat &format, QObject *parent = Q_NULLPTR)
{
    QAction *act = new QAction(format.description(), parent);
    if (!format.icon().isNull()) {
        act->setIcon(format.icon());
    }
    return act;
}

static QString KStandardActionName(KStandardAction::StandardAction id)
{
    return QString::fromLatin1(KStandardAction::name(id));
}

static KFilterDev::CompressionType compressionTypeFor(const QString &mime_to_check)
{
    // The compressedMimeMap is here in case you have a very old shared mime database
    // that doesn't have inheritance info for things like gzeps, etc
    // Otherwise the "is()" calls below are just good enough
    static QHash<QString, KFilterDev::CompressionType> compressedMimeMap;
    static bool supportBzip = false;
    static bool supportXz = false;
    const QString app_gzip(QStringLiteral("application/x-gzip"));
    const QString app_bzip(QStringLiteral("application/x-bzip"));
    const QString app_xz(QStringLiteral("application/x-xz"));
    if (compressedMimeMap.isEmpty()) {
        std::unique_ptr<KFilterBase> f;
        compressedMimeMap[QStringLiteral("image/x-gzeps")] = KFilterDev::GZip;
        // check we can read bzip2-compressed files
        f.reset(KCompressionDevice::filterForCompressionType(KCompressionDevice::BZip2));
        if (f.get()) {
            supportBzip = true;
            compressedMimeMap[QStringLiteral("application/x-bzpdf")] = KFilterDev::BZip2;
            compressedMimeMap[QStringLiteral("application/x-bzpostscript")] = KFilterDev::BZip2;
            compressedMimeMap[QStringLiteral("application/x-bzdvi")] = KFilterDev::BZip2;
            compressedMimeMap[QStringLiteral("image/x-bzeps")] = KFilterDev::BZip2;
        }
        // check if we can read XZ-compressed files
        f.reset(KCompressionDevice::filterForCompressionType(KCompressionDevice::Xz));
        if (f.get()) {
            supportXz = true;
        }
    }
    QHash<QString, KFilterDev::CompressionType>::const_iterator it = compressedMimeMap.constFind(mime_to_check);
    if (it != compressedMimeMap.constEnd()) {
        return it.value();
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mime_to_check);
    if (mime.isValid()) {
        if (mime.inherits(app_gzip)) {
            return KFilterDev::GZip;
        } else if (supportBzip && mime.inherits(app_bzip)) {
            return KFilterDev::BZip2;
        } else if (supportXz && mime.inherits(app_xz)) {
            return KFilterDev::Xz;
        }
    }

    return KFilterDev::None;
}

static Okular::EmbedMode detectEmbedMode(QWidget *parentWidget, QObject *parent, const QVariantList &args)
{
    Q_UNUSED(parentWidget);

    if (parent && (parent->objectName().startsWith(QLatin1String("okular::Shell")) || parent->objectName().startsWith(QLatin1String("okular/okular__Shell")))) {
        return Okular::NativeShellMode;
    }

    if (parent && (QByteArray("KHTMLPart") == parent->metaObject()->className())) {
        return Okular::KHTMLPartMode;
    }

    for (const QVariant &arg : args) {
        if (arg.type() == QVariant::String) {
            if (arg.toString() == QLatin1String("Print/Preview")) {
                return Okular::PrintPreviewMode;
            } else if (arg.toString() == QLatin1String("ViewerWidget")) {
                return Okular::ViewerWidgetMode;
            }
        }
    }

    return Okular::UnknownEmbedMode;
}

static QString detectConfigFileName(const QVariantList &args)
{
    for (const QVariant &arg : args) {
        if (arg.type() == QVariant::String) {
            QString argString = arg.toString();
            int separatorIndex = argString.indexOf(QStringLiteral("="));
            if (separatorIndex >= 0 && argString.left(separatorIndex) == QLatin1String("ConfigFileName")) {
                return argString.mid(separatorIndex + 1);
            }
        }
    }

    return QString();
}

#undef OKULAR_KEEP_FILE_OPEN

#ifdef OKULAR_KEEP_FILE_OPEN
static bool keepFileOpen()
{
    static bool keep_file_open = !qgetenv("OKULAR_NO_KEEP_FILE_OPEN").toInt();
    return keep_file_open;
}
#endif

int Okular::Part::numberOfParts = 0;

namespace Okular
{
Part::Part(QWidget *parentWidget, QObject *parent, const QVariantList &args)
    : KParts::ReadWritePart(parent)
    , m_tempfile(nullptr)
    , m_documentOpenWithPassword(false)
    , m_swapInsteadOfOpening(false)
    , m_tocEnabled(false)
    , m_isReloading(false)
    , m_fileWasRemoved(false)
    , m_showMenuBarAction(nullptr)
    , m_showFullScreenAction(nullptr)
    , m_cliPresentation(false)
    , m_cliPrint(false)
    , m_cliPrintAndExit(false)
    , m_embedMode(detectEmbedMode(parentWidget, parent, args))
    , m_generatorGuiClient(nullptr)
    , m_keeper(nullptr)
{
    // make sure that the component name is okular otherwise the XMLGUI .rc files are not found
    // when this part is used in an application other than okular (e.g. unit tests)
    setComponentName(QStringLiteral("okular"), QString());

    setupConfigSkeleton(args, componentName());

    numberOfParts++;
    if (numberOfParts == 1) {
        m_registerDbusName = QStringLiteral("/okular");
    } else {
        m_registerDbusName = QStringLiteral("/okular%1").arg(numberOfParts);
    }
    QDBusConnection::sessionBus().registerObject(m_registerDbusName, this, QDBusConnection::ExportScriptableSlots);

    // connect the started signal to tell the job the mimetypes we like,
    // and get some more information from it
    connect(this, &KParts::ReadOnlyPart::started, this, &Part::slotJobStarted);

    // connect the completed signal so we can put the window caption when loading remote files
    connect(this, QOverload<>::of(&Part::completed), this, &Part::setWindowTitleFromDocument);
    connect(this, &KParts::ReadOnlyPart::canceled, this, &Part::loadCancelled);

    // create browser extension (for printing when embedded into browser)
    m_bExtension = new BrowserExtension(this);
    // create live connect extension (for integrating with browser scripting)
    new OkularLiveConnectExtension(this);

    const QStringList iconDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("okular/pics"), QStandardPaths::LocateDirectory);
    QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << iconDirs);

    m_sidebar = new Sidebar(parentWidget);
    setWidget(m_sidebar);
    connect(m_sidebar, &Sidebar::urlsDropped, this, &Part::handleDroppedUrls);

    // build the document
    m_document = new Okular::Document(widget());
    connect(m_document, &Document::linkFind, this, &Part::slotFind);
    connect(m_document, &Document::linkGoToPage, this, &Part::slotGoToPage);
    connect(m_document, &Document::linkPresentation, this, &Part::slotShowPresentation);
    connect(m_document, &Document::linkEndPresentation, this, &Part::slotHidePresentation);
    connect(m_document, &Document::openUrl, this, &Part::openUrlFromDocument);
    connect(m_document->bookmarkManager(), &BookmarkManager::openUrl, this, &Part::openUrlFromBookmarks);
    connect(m_document, &Document::close, this, &Part::close);
    connect(m_document, &Document::requestPrint, this, &Part::slotPrint);
    connect(m_document, &Document::requestSaveAs, this, [this] { slotSaveFileAs(); });
    connect(m_document, &Document::undoHistoryCleanChanged, this, [this](bool clean) {
        setModified(!clean);
        setWindowTitleFromDocument();
    });

    if (parent && parent->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("slotQuit()").constData()) != -1) {
        connect(m_document, SIGNAL(quit()), parent, SLOT(slotQuit())); // clazy:exclude=old-style-connect
    } else {
        connect(m_document, &Document::quit, this, &Part::cannotQuit);
    }
    // widgets: ^searchbar (toolbar containing label and SearchWidget)
    //      m_searchToolBar = new KToolBar( parentWidget, "searchBar" );
    //      m_searchToolBar->boxLayout()->setSpacing( KDialog::spacingHint() );
    //      QLabel * sLabel = new QLabel( i18n( "&Search:" ), m_searchToolBar, "kde toolbar widget" );
    //      m_searchWidget = new SearchWidget( m_searchToolBar, m_document );
    //      sLabel->setBuddy( m_searchWidget );
    //      m_searchToolBar->setStretchableWidget( m_searchWidget );

    // [left toolbox optional item: Table of Contents] | []
    m_toc = new TOC(nullptr, m_document);
    connect(m_toc.data(), &TOC::hasTOC, this, &Part::enableTOC);
    connect(m_toc.data(), &TOC::rightClick, this, &Part::slotShowTOCMenu);

    // [left toolbox optional item: Layers] | []
    m_layers = new Layers(nullptr, m_document);
    connect(m_layers.data(), &Layers::hasLayers, this, &Part::enableLayers);

    // [left toolbox: Thumbnails and Bookmarks] | []
    QWidget *thumbsBox = new ThumbnailsBox(nullptr);
    thumbsBox->layout()->setSpacing(6);
    m_searchWidget = new SearchWidget(thumbsBox, m_document);
    thumbsBox->layout()->addWidget(m_searchWidget);
    m_thumbnailList = new ThumbnailList(thumbsBox, m_document);
    thumbsBox->layout()->addWidget(m_thumbnailList);
    //	ThumbnailController * m_tc = new ThumbnailController( thumbsBox, m_thumbnailList );
    connect(m_thumbnailList.data(), &ThumbnailList::rightClick, this, &Part::slotShowMenu);
    m_sidebar->addItem(thumbsBox, QIcon::fromTheme(QStringLiteral("view-preview")), i18n("Thumbnails"));

    m_sidebar->setCurrentItem(thumbsBox);

    // [left toolbox: Reviews] | []
    m_reviewsWidget = new Reviews(nullptr, m_document);
    m_sidebar->addItem(m_reviewsWidget, QIcon::fromTheme(QStringLiteral("draw-freehand")), i18n("Annotations"));

    // [left toolbox: Bookmarks] | []
    m_bookmarkList = new BookmarkList(m_document, nullptr);
    m_sidebar->addItem(m_bookmarkList, QIcon::fromTheme(QStringLiteral("bookmarks")), i18n("Bookmarks"));

    // [left toolbox optional item: Signature Panel] | []
    m_signaturePanel = new SignaturePanel(m_document, nullptr);
    connect(m_signaturePanel.data(), &SignaturePanel::documentHasSignatures, this, &Part::enableSidebarSignaturesItem);

    // widgets: [../miniBarContainer] | []
#ifdef OKULAR_ENABLE_MINIBAR
    QWidget *miniBarContainer = new QWidget(0);
    m_sidebar->setBottomWidget(miniBarContainer);
    QVBoxLayout *miniBarLayout = new QVBoxLayout(miniBarContainer);
    miniBarLayout->setContentsMargins(0, 0, 0, 0);
    // widgets: [../[spacer/..]] | []
    miniBarLayout->addItem(new QSpacerItem(6, 6, QSizePolicy::Fixed, QSizePolicy::Fixed));
    // widgets: [../[../MiniBar]] | []
    QFrame *bevelContainer = new QFrame(miniBarContainer);
    bevelContainer->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QVBoxLayout *bevelContainerLayout = new QVBoxLayout(bevelContainer);
    bevelContainerLayout->setContentsMargins(4, 4, 4, 4);
    m_progressWidget = new ProgressWidget(bevelContainer, m_document);
    bevelContainerLayout->addWidget(m_progressWidget);
    miniBarLayout->addWidget(bevelContainer);
    miniBarLayout->addItem(new QSpacerItem(6, 6, QSizePolicy::Fixed, QSizePolicy::Fixed));
#endif

    // widgets: [] | [right 'pageView']
    QWidget *rightContainer = new QWidget(nullptr);
    m_sidebar->setMainWidget(rightContainer);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    //	KToolBar * rtb = new KToolBar( rightContainer, "mainToolBarSS" );
    //	rightLayout->addWidget( rtb );
    m_migrationMessage = new KMessageWidget(rightContainer);
    m_migrationMessage->setVisible(false);
    m_migrationMessage->setWordWrap(true);
    m_migrationMessage->setMessageType(KMessageWidget::Warning);
    m_migrationMessage->setText(
        i18n("This document contains annotations or form data that were saved internally by a previous Okular version. Internal storage is <b>no longer supported</b>.<br/>Please save to a file in order to move them if you want to continue "
             "to edit the document."));
    rightLayout->addWidget(m_migrationMessage);
    m_topMessage = new KMessageWidget(rightContainer);
    m_topMessage->setVisible(false);
    m_topMessage->setWordWrap(true);
    m_topMessage->setMessageType(KMessageWidget::Information);
    m_topMessage->setText(i18n("This document has embedded files. <a href=\"okular:/embeddedfiles\">Click here to see them</a> or go to File -> Embedded Files."));
    m_topMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-attachment")));
    connect(m_topMessage, &KMessageWidget::linkActivated, this, &Part::slotShowEmbeddedFiles);
    rightLayout->addWidget(m_topMessage);
    m_formsMessage = new KMessageWidget(rightContainer);
    m_formsMessage->setVisible(false);
    m_formsMessage->setWordWrap(true);
    m_formsMessage->setMessageType(KMessageWidget::Information);
    rightLayout->addWidget(m_formsMessage);
    m_infoMessage = new KMessageWidget(rightContainer);
    m_infoMessage->setVisible(false);
    m_infoMessage->setWordWrap(true);
    m_infoMessage->setMessageType(KMessageWidget::Information);
    rightLayout->addWidget(m_infoMessage);
    m_infoTimer = new QTimer();
    m_infoTimer->setSingleShot(true);
    connect(m_infoTimer, &QTimer::timeout, m_infoMessage, &KMessageWidget::animatedHide);
    m_signatureMessage = new KMessageWidget(rightContainer);
    m_signatureMessage->setVisible(false);
    m_signatureMessage->setWordWrap(true);
    rightLayout->addWidget(m_signatureMessage);
    m_pageView = new PageView(rightContainer, m_document);
    rightContainer->setFocusProxy(m_pageView);
    QMetaObject::invokeMethod(m_pageView, "setFocus", Qt::QueuedConnection); // usability setting
    //    m_splitter->setFocusProxy(m_pageView);
    connect(m_pageView.data(), &PageView::rightClick, this, &Part::slotShowMenu);
    connect(m_pageView, &PageView::triggerSearch, this, [this](const QString &searchText) {
        m_findBar->startSearch(searchText);
        slotShowFindBar();
    });
    connect(m_document, &Document::error, this, &Part::errorMessage);
    connect(m_document, &Document::warning, this, &Part::warningMessage);
    connect(m_document, &Document::notice, this, &Part::noticeMessage);
    connect(m_document, &Document::sourceReferenceActivated, this, &Part::slotHandleActivatedSourceReference);
    connect(m_pageView.data(), &PageView::fitWindowToPage, this, &Part::fitWindowToPage);
    rightLayout->addWidget(m_pageView);
    m_layers->setPageView(m_pageView);
    m_signaturePanel->setPageView(m_pageView);
    m_findBar = new FindBar(m_document, rightContainer);
    rightLayout->addWidget(m_findBar);
    m_bottomBar = new QWidget(rightContainer);
    QHBoxLayout *bottomBarLayout = new QHBoxLayout(m_bottomBar);
    m_pageSizeLabel = new PageSizeLabel(m_bottomBar, m_document);
    bottomBarLayout->setContentsMargins(0, 0, 0, 0);
    bottomBarLayout->setSpacing(0);
    bottomBarLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Expanding, QSizePolicy::Minimum));
    m_miniBarLogic = new MiniBarLogic(this, m_document);
    m_miniBar = new MiniBar(m_bottomBar, m_miniBarLogic);
    bottomBarLayout->addWidget(m_miniBar);
    bottomBarLayout->addWidget(m_pageSizeLabel);
    rightLayout->addWidget(m_bottomBar);

    m_pageNumberTool = new MiniBar(nullptr, m_miniBarLogic);

    connect(m_findBar, &FindBar::forwardKeyPressEvent, m_pageView, &PageView::externalKeyPressEvent);
    connect(m_findBar, &FindBar::onCloseButtonPressed, m_pageView, QOverload<>::of(&PageView::setFocus));
    connect(m_miniBar, &MiniBar::forwardKeyPressEvent, m_pageView, &PageView::externalKeyPressEvent);
    connect(m_pageView.data(), &PageView::escPressed, m_findBar, &FindBar::resetSearch);
    connect(m_pageNumberTool, &MiniBar::forwardKeyPressEvent, m_pageView, &PageView::externalKeyPressEvent);
    connect(m_pageView.data(), &PageView::requestOpenFile, this, [this](const QString &filePath, int pageNumber) {
        // We cheat a bit here reusing the urlsDropped signal, but at the end the output is the same, we want to open some files
        // urlsDropped should have just had a different name
        QUrl u = QUrl::fromLocalFile(filePath);
        u.setFragment(QStringLiteral("page=%1").arg(pageNumber));
        Q_EMIT urlsDropped({u});
    });

    connect(m_reviewsWidget.data(), &Reviews::openAnnotationWindow, m_pageView.data(), &PageView::openAnnotationWindow);

    // add document observers
    m_document->addObserver(this);
    m_document->addObserver(m_thumbnailList);
    m_document->addObserver(m_pageView);
    m_document->registerView(m_pageView);
    m_document->addObserver(m_toc);
    m_document->addObserver(m_miniBarLogic);
#ifdef OKULAR_ENABLE_MINIBAR
    m_document->addObserver(m_progressWidget);
#endif
    m_document->addObserver(m_reviewsWidget);
    m_document->addObserver(m_pageSizeLabel);
    m_document->addObserver(m_bookmarkList);
    m_document->addObserver(m_signaturePanel);

    connect(m_document->bookmarkManager(), &BookmarkManager::saved, this, &Part::slotRebuildBookmarkMenu);

    setupViewerActions();

    if (m_embedMode != ViewerWidgetMode) {
        setupActions();
    } else {
        setViewerShortcuts();
    }

    // document watcher and reloader
    m_watcher = new KDirWatch(this);
    connect(m_watcher, &KDirWatch::dirty, this, &Part::slotFileDirty);
    connect(m_watcher, &KDirWatch::created, this, &Part::slotFileDirty);
    connect(m_watcher, &KDirWatch::deleted, this, &Part::slotFileDirty);
    m_dirtyHandler = new QTimer(this);
    m_dirtyHandler->setSingleShot(true);
    connect(m_dirtyHandler, &QTimer::timeout, this, [this] { slotAttemptReload(); });

    slotNewConfig();

    // keep us informed when the user changes settings
    connect(Okular::Settings::self(), &KCoreConfigSkeleton::configChanged, this, &Part::slotNewConfig);

#if HAVE_SPEECH
    // [SPEECH] check for TTS presence and usability
    Okular::Settings::setUseTTS(true);
    Okular::Settings::self()->save();
#endif

    rebuildBookmarkMenu(false);

    if (m_embedMode == ViewerWidgetMode) {
        // set the XML-UI resource file for the viewer mode
        setXMLFile(QStringLiteral("part-viewermode.rc"));
    } else {
        // set our main XML-UI resource file
        setXMLFile(QStringLiteral("part.rc"));
    }

    m_pageView->setupBaseActions(actionCollection());

    m_sidebar->setSidebarVisibility(false);
    if (m_embedMode != PrintPreviewMode) {
        // now set up actions that are required for all remaining modes
        m_pageView->setupViewerActions(actionCollection());
        // and if we are not in viewer mode, we want the full GUI
        if (m_embedMode != ViewerWidgetMode) {
            unsetDummyMode();
        }
    }

    // ensure history actions are in the correct state
    updateViewActions();

    // also update the state of the actions in the page view
    m_pageView->updateActionState(false, false);

    if (m_embedMode == NativeShellMode) {
        m_sidebar->setAutoFillBackground(false);
    }

#ifdef OKULAR_KEEP_FILE_OPEN
    m_keeper = new FileKeeper();
#endif
}

void Part::setupConfigSkeleton(const QVariantList &args, const QString &componentName)
{
    const QLatin1String configFileName("okularpartrc");

    // first, we check if a config file name has been specified
    QString configFilePath = detectConfigFileName(args);

    if (configFilePath.isEmpty()) {
        configFilePath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1Char('/') + configFileName;
    }

    // Migrate old config
    if (!QFile::exists(configFilePath)) {
        qCDebug(OkularUiDebug) << "Did not find a config file, attempting to look for old config";

        // Migrate old config + UI
        Kdelibs4ConfigMigrator configMigrator(componentName);
        // UI file is handled automatically, we only need to specify config name because we're a part
        configMigrator.setConfigFiles(QStringList(configFileName));

        // If there's no old okular config to migrate, look for kpdf
        if (!configMigrator.migrate()) {
            qCDebug(OkularUiDebug) << "Did not find an old okular config file, attempting to look for kpdf config";

            // First try the automatic detection, using $KDEHOME etc.
            Kdelibs4Migration migration;
            QString kpdfConfig = migration.locateLocal("config", QStringLiteral("kpdfpartrc"));

            // Fallback just in case it tried e. g. ~/.kde4
            if (kpdfConfig.isEmpty()) {
                kpdfConfig = QDir::homePath() + QStringLiteral("/.kde/share/config/kpdfpartrc");
            }

            if (QFile::exists(kpdfConfig)) {
                qCDebug(OkularUiDebug) << "Found old kpdf config" << kpdfConfig << "copying to" << configFilePath;
                QFile::copy(kpdfConfig, configFilePath);
            } else {
                qCDebug(OkularUiDebug) << "Did not find an old kpdf config file";
            }
        } else {
            qCDebug(OkularUiDebug) << "Migrated old okular config";
        }
    }

    KSharedConfigPtr config = KSharedConfig::openConfig(configFilePath);

    // Configuration update: SlidesTransitionsEnabled -> SlidesTransition = NoTransitions.
    // See https://invent.kde.org/graphics/okular/-/merge_requests/357
    KConfigGroup slidesConfigGroup = config.data()->group("Dlg Presentation");
    if (slidesConfigGroup.readEntry<bool>("SlidesTransitionsEnabled", true) == false) {
        slidesConfigGroup.writeEntry("SlidesTransition", "NoTransitions");
    }
    slidesConfigGroup.deleteEntry("SlidesTransitionsEnabled");
    config.data()->sync();

    Okular::Settings::instance(config);
}

void Part::setupViewerActions()
{
    // ACTIONS
    KActionCollection *ac = actionCollection();

    // Page Traversal actions
    m_gotoPage = KStandardAction::gotoPage(this, SLOT(slotGoToPage()), ac);
    ac->setDefaultShortcuts(m_gotoPage, KStandardShortcut::gotoLine());
    // dirty way to activate gotopage when pressing miniBar's button
    connect(m_miniBar.data(), &MiniBar::gotoPage, m_gotoPage, &QAction::trigger);
    connect(m_pageNumberTool.data(), &MiniBar::gotoPage, m_gotoPage, &QAction::trigger);

    m_prevPage = KStandardAction::prior(this, SLOT(slotPreviousPage()), ac);
    m_prevPage->setIconText(i18nc("Previous page", "Previous"));
    m_prevPage->setToolTip(i18n("Go back to the Previous Page"));
    m_prevPage->setWhatsThis(i18n("Moves to the previous page of the document"));
    ac->setDefaultShortcut(m_prevPage, QKeySequence());
    // dirty way to activate prev page when pressing miniBar's button
    connect(m_miniBar.data(), &MiniBar::prevPage, m_prevPage, &QAction::trigger);
    connect(m_pageNumberTool.data(), &MiniBar::prevPage, m_prevPage, &QAction::trigger);
#ifdef OKULAR_ENABLE_MINIBAR
    connect(m_progressWidget, SIGNAL(prevPage()), m_prevPage, SLOT(trigger()));
#endif

    m_nextPage = KStandardAction::next(this, SLOT(slotNextPage()), ac);
    m_nextPage->setIconText(i18nc("Next page", "Next"));
    m_nextPage->setToolTip(i18n("Advance to the Next Page"));
    m_nextPage->setWhatsThis(i18n("Moves to the next page of the document"));
    ac->setDefaultShortcut(m_nextPage, QKeySequence());
    // dirty way to activate next page when pressing miniBar's button
    connect(m_miniBar.data(), &MiniBar::nextPage, m_nextPage, &QAction::trigger);
    connect(m_pageNumberTool.data(), &MiniBar::nextPage, m_nextPage, &QAction::trigger);
#ifdef OKULAR_ENABLE_MINIBAR
    connect(m_progressWidget, SIGNAL(nextPage()), m_nextPage, SLOT(trigger()));
#endif

    m_beginningOfDocument = KStandardAction::firstPage(this, SLOT(slotGotoFirst()), ac);
    ac->addAction(QStringLiteral("first_page"), m_beginningOfDocument);
    m_beginningOfDocument->setText(i18n("Beginning of the document"));
    m_beginningOfDocument->setWhatsThis(i18n("Moves to the beginning of the document"));

    m_endOfDocument = KStandardAction::lastPage(this, SLOT(slotGotoLast()), ac);
    ac->addAction(QStringLiteral("last_page"), m_endOfDocument);
    m_endOfDocument->setText(i18n("End of the document"));
    m_endOfDocument->setWhatsThis(i18n("Moves to the end of the document"));

    // we do not want back and next in history in the dummy mode
    m_historyBack = nullptr;
    m_historyNext = nullptr;

    m_addBookmark = KStandardAction::addBookmark(this, SLOT(slotAddBookmark()), ac);
    m_addBookmarkText = m_addBookmark->text();
    m_addBookmarkIcon = m_addBookmark->icon();
    m_bookmarkList->setAddBookmarkAction(m_addBookmark);

    m_renameBookmark = ac->addAction(QStringLiteral("rename_bookmark"));
    m_renameBookmark->setText(i18n("Rename Bookmark"));
    m_renameBookmark->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    m_renameBookmark->setWhatsThis(i18n("Rename the current bookmark"));
    connect(m_renameBookmark, &QAction::triggered, this, &Part::slotRenameCurrentViewportBookmark);

    m_prevBookmark = ac->addAction(QStringLiteral("previous_bookmark"));
    m_prevBookmark->setText(i18n("Previous Bookmark"));
    m_prevBookmark->setIcon(QIcon::fromTheme(QStringLiteral("go-up-search")));
    m_prevBookmark->setWhatsThis(i18n("Go to the previous bookmark"));
    connect(m_prevBookmark, &QAction::triggered, this, &Part::slotPreviousBookmark);

    m_nextBookmark = ac->addAction(QStringLiteral("next_bookmark"));
    m_nextBookmark->setText(i18n("Next Bookmark"));
    m_nextBookmark->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
    m_nextBookmark->setWhatsThis(i18n("Go to the next bookmark"));
    connect(m_nextBookmark, &QAction::triggered, this, &Part::slotNextBookmark);

    m_copy = nullptr;

    m_selectAll = nullptr;
    m_selectCurrentPage = nullptr;

    // Find and other actions
    m_find = KStandardAction::find(this, SLOT(slotShowFindBar()), ac);
    QList<QKeySequence> s = m_find->shortcuts();
    s.append(QKeySequence(Qt::Key_Slash));
    ac->setDefaultShortcuts(m_find, s);
    m_find->setEnabled(false);

    m_findNext = KStandardAction::findNext(this, SLOT(slotFindNext()), ac);
    m_findNext->setEnabled(false);

    m_findPrev = KStandardAction::findPrev(this, SLOT(slotFindPrev()), ac);
    m_findPrev->setEnabled(false);

    m_save = nullptr;
    m_saveAs = nullptr;
    m_openContainingFolder = nullptr;

    m_hamburgerMenuAction = nullptr;

    QAction *prefs = KStandardAction::preferences(this, SLOT(slotPreferences()), ac);
    if (m_embedMode == NativeShellMode) {
        prefs->setText(i18n("Configure Okular..."));
    } else {
        // TODO: improve this message
        prefs->setText(i18n("Configure Viewer..."));
    }

    QAction *genPrefs = new QAction(ac);
    ac->addAction(QStringLiteral("options_configure_generators"), genPrefs);
    if (m_embedMode == ViewerWidgetMode) {
        genPrefs->setText(i18n("Configure Viewer Backends..."));
    } else {
        genPrefs->setText(i18n("Configure Backends..."));
    }
    genPrefs->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    genPrefs->setEnabled(m_document->configurableGenerators() > 0);
    connect(genPrefs, &QAction::triggered, this, &Part::slotGeneratorPreferences);

    m_printPreview = KStandardAction::printPreview(this, SLOT(slotPrintPreview()), ac);
    m_printPreview->setEnabled(false);

    m_showLeftPanel = nullptr;
    m_showBottomBar = nullptr;
    m_showSignaturePanel = nullptr;

    m_showProperties = ac->addAction(QStringLiteral("properties"));
    m_showProperties->setText(i18n("&Properties"));
    m_showProperties->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    ac->setDefaultShortcuts(m_showProperties, {QKeySequence(Qt::ALT | Qt::Key_Return)});
    connect(m_showProperties, &QAction::triggered, this, &Part::slotShowProperties);
    m_showProperties->setEnabled(false);

    m_showEmbeddedFiles = nullptr;
    m_showPresentation = nullptr;

    m_exportAs = nullptr;
    m_exportAsMenu = nullptr;
    m_exportAsText = nullptr;
    m_exportAsDocArchive = nullptr;

#if HAVE_PURPOSE
    m_share = nullptr;
    m_shareMenu = nullptr;
#endif

    m_presentationDrawingActions = nullptr;

    m_aboutBackend = ac->addAction(QStringLiteral("help_about_backend"));
    m_aboutBackend->setText(i18n("About Backend"));
    m_aboutBackend->setEnabled(false);
    connect(m_aboutBackend, &QAction::triggered, this, &Part::slotAboutBackend);

    QAction *reload = ac->add<QAction>(QStringLiteral("file_reload"));
    reload->setText(i18n("Reloa&d"));
    reload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    reload->setWhatsThis(i18n("Reload the current document from disk."));
    connect(reload, &QAction::triggered, this, &Part::slotReload);
    ac->setDefaultShortcuts(reload, KStandardShortcut::reload());
    m_reload = reload;

    m_closeFindBar = ac->addAction(QStringLiteral("close_find_bar"), this, SLOT(slotHideFindBar()));
    m_closeFindBar->setText(i18n("Close &Find Bar"));
    ac->setDefaultShortcut(m_closeFindBar, QKeySequence(Qt::Key_Escape));
    m_closeFindBar->setEnabled(false);

    QWidgetAction *pageno = new QWidgetAction(ac);
    pageno->setText(i18n("Page Number"));
    pageno->setDefaultWidget(m_pageNumberTool);
    ac->addAction(QStringLiteral("page_number"), pageno);

    QAction *configureColorModes = new QAction(i18nc("@action", "Configure Color Modes..."), ac);
    ac->addAction(QStringLiteral("options_configure_color_modes"), configureColorModes);
    configureColorModes->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    connect(configureColorModes, &QAction::triggered, this, &Part::slotAccessibilityPreferences);
}

void Part::setViewerShortcuts()
{
    KActionCollection *ac = actionCollection();

    ac->setDefaultShortcut(m_gotoPage, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G));
    ac->setDefaultShortcut(m_find, QKeySequence());

    ac->setDefaultShortcut(m_findNext, QKeySequence());
    ac->setDefaultShortcut(m_findPrev, QKeySequence());

    ac->setDefaultShortcut(m_addBookmark, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_B));

    ac->setDefaultShortcut(m_beginningOfDocument, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Home));
    ac->setDefaultShortcut(m_endOfDocument, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_End));

    QAction *action = static_cast<QAction *>(ac->action(QStringLiteral("file_reload")));
    if (action) {
        ac->setDefaultShortcut(action, QKeySequence(Qt::ALT | Qt::Key_F5));
    }
}

void Part::setupActions()
{
    KActionCollection *ac = actionCollection();

    auto manager = new KColorSchemeManager(this);
    KActionMenu *schemeMenu = manager->createSchemeSelectionMenu(this);
    ac->addAction(QStringLiteral("colorscheme_menu"), schemeMenu->menu()->menuAction());

    m_copy = KStandardAction::create(KStandardAction::Copy, m_pageView, SLOT(copyTextSelection()), ac);

    m_selectAll = KStandardAction::selectAll(m_pageView, SLOT(selectAll()), ac);

    // Setup select all action for the current page
    m_selectCurrentPage = ac->addAction(QStringLiteral("edit_select_all_current_page"));
    m_selectCurrentPage->setText(i18n("Select All Text on Current Page"));
    connect(m_selectCurrentPage, &QAction::triggered, m_pageView, &PageView::slotSelectPage);
    m_selectCurrentPage->setEnabled(false);

    m_save = KStandardAction::save(
        this, [this] { saveFile(); }, ac);
    m_save->setEnabled(false);

    m_saveAs = KStandardAction::saveAs(this, SLOT(slotSaveFileAs()), ac);
    m_saveAs->setEnabled(false);
    m_migrationMessage->addAction(m_saveAs);

    m_showLeftPanel = ac->add<KToggleAction>(QStringLiteral("show_leftpanel"));
    m_showLeftPanel->setText(i18n("Show S&idebar"));
    const QString preferredSidebarIcon = m_sidebar->layoutDirection() == Qt::LeftToRight ? QStringLiteral("sidebar-expand-left") : QStringLiteral("sidebar-expand-right");
    m_showLeftPanel->setIcon(QIcon::fromTheme(preferredSidebarIcon, QIcon::fromTheme(QStringLiteral("view-sidetree"))));
    connect(m_showLeftPanel, &QAction::toggled, this, &Part::slotShowLeftPanel);
    ac->setDefaultShortcut(m_showLeftPanel, QKeySequence(Qt::Key_F7));
    m_showLeftPanel->setChecked(Okular::Settings::showLeftPanel());
    slotShowLeftPanel();

    m_showBottomBar = ac->add<KToggleAction>(QStringLiteral("show_bottombar"));
    m_showBottomBar->setText(i18n("Show &Page Bar"));
    connect(m_showBottomBar, &QAction::toggled, this, &Part::slotShowBottomBar);
    m_showBottomBar->setChecked(Okular::Settings::showBottomBar());
    slotShowBottomBar();

    m_showSignaturePanel = ac->add<QAction>(QStringLiteral("show_signatures"));
    m_showSignaturePanel->setText(i18n("Show &Signatures Panel"));
    connect(m_showSignaturePanel, &QAction::triggered, this, [this] {
        if (m_sidebar->currentItem() != m_signaturePanel) {
            m_sidebar->setCurrentItem(m_signaturePanel);
        }
        m_showLeftPanel->setChecked(true);
        slotShowLeftPanel();
    });

    m_showEmbeddedFiles = ac->addAction(QStringLiteral("embedded_files"));
    m_showEmbeddedFiles->setText(i18n("&Embedded Files"));
    m_showEmbeddedFiles->setIcon(QIcon::fromTheme(QStringLiteral("mail-attachment")));
    connect(m_showEmbeddedFiles, &QAction::triggered, this, &Part::slotShowEmbeddedFiles);
    m_showEmbeddedFiles->setEnabled(false);

    m_exportAs = ac->addAction(QStringLiteral("file_export_as"));
    m_exportAs->setText(i18n("E&xport As"));
    m_exportAs->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
    m_exportAsMenu = new QMenu();
    connect(m_exportAsMenu, &QMenu::triggered, this, &Part::slotExportAs);
    m_exportAs->setMenu(m_exportAsMenu);
    m_exportAsText = actionForExportFormat(Okular::ExportFormat::standardFormat(Okular::ExportFormat::PlainText), m_exportAsMenu);
    m_exportAsMenu->addAction(m_exportAsText);
    m_exportAs->setEnabled(false);
    m_exportAsText->setEnabled(false);

#if HAVE_PURPOSE
    m_share = ac->addAction(QStringLiteral("file_share"));
    m_share->setText(i18n("S&hare"));
    m_share->setIcon(QIcon::fromTheme(QStringLiteral("document-share")));
    m_share->setEnabled(false);
    m_shareMenu = new Purpose::Menu();
    connect(m_shareMenu, &Purpose::Menu::finished, this, &Part::slotShareActionFinished);
    m_share->setMenu(m_shareMenu);
#endif

    m_showPresentation = ac->addAction(QStringLiteral("presentation"));
    m_showPresentation->setText(i18n("P&resentation"));
    m_showPresentation->setIcon(QIcon::fromTheme(QStringLiteral("view-presentation")));
    connect(m_showPresentation, &QAction::triggered, this, &Part::slotShowPresentation);
    ac->setDefaultShortcut(m_showPresentation, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    m_showPresentation->setEnabled(false);

    m_openContainingFolder = ac->addAction(QStringLiteral("open_containing_folder"));
    m_openContainingFolder->setText(i18n("Open Con&taining Folder"));
    m_openContainingFolder->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    connect(m_openContainingFolder, &QAction::triggered, this, &Part::slotOpenContainingFolder);
    m_openContainingFolder->setEnabled(false);

    if (m_embedMode == Okular::NativeShellMode) { // This hamburger menu is designed to be quite Okular-specific.
        m_hamburgerMenuAction = KStandardAction::hamburgerMenu(nullptr, nullptr, ac);
        if (auto *mainWindow = findMainWindow()) {
            m_hamburgerMenuAction->setMenuBar(mainWindow->menuBar());
        }
        connect(m_hamburgerMenuAction, &KHamburgerMenu::aboutToShowMenu, this, &Part::slotUpdateHamburgerMenu);
    }

    QAction *importPS = ac->addAction(QStringLiteral("import_ps"));
    importPS->setText(i18n("&Import PostScript as PDF..."));
    importPS->setIcon(QIcon::fromTheme(QStringLiteral("document-import")));
    connect(importPS, &QAction::triggered, this, &Part::slotImportPSFile);

    KToggleAction *blackscreenAction = new KToggleAction(i18n("Switch Blackscreen Mode"), ac);
    ac->addAction(QStringLiteral("switch_blackscreen_mode"), blackscreenAction);
    ac->setDefaultShortcut(blackscreenAction, QKeySequence(Qt::Key_B));
    blackscreenAction->setIcon(QIcon::fromTheme(QStringLiteral("view-presentation")));
    blackscreenAction->setEnabled(false);

    m_presentationDrawingActions = new DrawingToolActions(ac);

    QAction *eraseDrawingAction = new QAction(i18n("Erase Drawing"), ac);
    ac->addAction(QStringLiteral("presentation_erase_drawings"), eraseDrawingAction);
    eraseDrawingAction->setIcon(QIcon::fromTheme(QStringLiteral("draw-eraser-delete-objects")));
    eraseDrawingAction->setEnabled(false);

    QAction *configureAnnotations = new QAction(i18n("Configure Annotations..."), ac);
    ac->addAction(QStringLiteral("options_configure_annotations"), configureAnnotations);
    configureAnnotations->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    connect(configureAnnotations, &QAction::triggered, this, &Part::slotAnnotationPreferences);

    QAction *playPauseAction = new QAction(i18n("Play/Pause Presentation"), ac);
    ac->addAction(QStringLiteral("presentation_play_pause"), playPauseAction);
    playPauseAction->setEnabled(false);
}

Part::~Part()
{
    QDBusConnection::sessionBus().unregisterObject(m_registerDbusName);

    m_document->removeObserver(this);

    if (m_document->isOpened()) {
        Part::closeUrl(false);
    }

    delete m_toc;
    delete m_layers;
    delete m_pageView;
    delete m_thumbnailList;
    delete m_miniBar;
    delete m_pageNumberTool;
    delete m_miniBarLogic;
    delete m_bottomBar;
#ifdef OKULAR_ENABLE_MINIBAR
    delete m_progressWidget;
#endif
    delete m_pageSizeLabel;
    delete m_reviewsWidget;
    delete m_bookmarkList;
    delete m_infoTimer;
    delete m_signaturePanel;

    delete m_document;

    delete m_tempfile;

    qDeleteAll(m_bookmarkActions);

    delete m_exportAsMenu;
#if HAVE_PURPOSE
    delete m_shareMenu;
#endif

#ifdef OKULAR_KEEP_FILE_OPEN
    delete m_keeper;
#endif
}

bool Part::openDocument(const QUrl &url, uint page)
{
    Okular::DocumentViewport vp(page - 1);
    vp.rePos.enabled = true;
    vp.rePos.normalizedX = 0;
    vp.rePos.normalizedY = 0;
    vp.rePos.pos = Okular::DocumentViewport::TopLeft;
    if (vp.isValid()) {
        m_document->setNextDocumentViewport(vp);
    }
    return openUrl(url);
}

void Part::startPresentation()
{
    m_cliPresentation = true;
}

QStringList Part::supportedMimeTypes() const
{
    return m_document->supportedMimeTypes();
}

QUrl Part::realUrl() const
{
    if (!m_realUrl.isEmpty()) {
        return m_realUrl;
    }

    return url();
}

// ViewerInterface

void Part::showSourceLocation(const QString &fileName, int line, int column, bool showGraphically)
{
    Q_UNUSED(column);

    const QString u = QStringLiteral("src:%1 %2").arg(line + 1).arg(fileName);
    GotoAction action(QString(), u);
    m_document->processAction(&action);
    if (showGraphically) {
        m_pageView->setLastSourceLocationViewport(m_document->viewport());
    }
}

void Part::clearLastShownSourceLocation()
{
    m_pageView->clearLastSourceLocationViewport();
}

bool Part::isWatchFileModeEnabled() const
{
    return !m_watcher->signalsBlocked();
}

void Part::setWatchFileModeEnabled(bool enabled)
{
    // Don't call 'KDirWatch::stopScan()' in here (as of KDE Frameworks 5.51.0, see bug 400541)!
    // 'KDirWatch::stopScan' has a bug that may affect other code paths that make use of KDirWatch
    // (other loaded KParts, for example).
    if (isWatchFileModeEnabled() == enabled) {
        return;
    }

    m_watcher->blockSignals(!enabled);

    if (!enabled) {
        m_dirtyHandler->stop();
    }
}

bool Part::areSourceLocationsShownGraphically() const
{
    return m_pageView->areSourceLocationsShownGraphically();
}

void Part::setShowSourceLocationsGraphically(bool show)
{
    m_pageView->setShowSourceLocationsGraphically(show);
}

bool Part::openNewFilesInTabs() const
{
    return Okular::Settings::self()->shellOpenFileInTabs();
}

QWidget *Part::getSideContainer() const
{
    return m_sidebar->getSideContainer();
}

bool Part::activateTabIfAlreadyOpenFile() const
{
    return Okular::Settings::self()->switchToTabIfOpen();
}

void Part::setModified(bool modified)
{
    KParts::ReadWritePart::setModified(modified);

    if (modified && !m_save->isEnabled()) {
        if (!m_warnedAboutModifyingUnsaveableDocument) {
            m_warnedAboutModifyingUnsaveableDocument = true;
            KMessageBox::information(widget(),
                                     i18n("You have just modified the open document, but this kind of document can not be saved.\nAny modification will be lost once Okular is closed."),
                                     i18n("Document can't be saved"),
                                     QStringLiteral("warnAboutUnsaveableDocuments"));
        }
    }
}

void Part::slotHandleActivatedSourceReference(const QString &absFileName, int line, int col, bool *handled)
{
    Q_EMIT openSourceReference(absFileName, line, col);
    if (m_embedMode == Okular::ViewerWidgetMode) {
        *handled = true;
    }
}

void Part::openUrlFromDocument(const QUrl &url)
{
    if (m_embedMode == PrintPreviewMode) {
        return;
    }

    if (url.isLocalFile()) {
        if (!QFile::exists(url.toLocalFile())) {
            KMessageBox::error(widget(), i18n("Could not open '%1'. File does not exist", url.toDisplayString()));
            return;
        }
    } else {
        KIO::StatJob *statJob = KIO::stat(url, KIO::StatJob::SourceSide, 0);
        KJobWidgets::setWindow(statJob, widget());
        if (!statJob->exec() || statJob->error()) {
            KMessageBox::error(widget(), i18n("Could not open '%1' (%2) ", url.toDisplayString(), statJob->errorString()));
            return;
        }
    }

    Q_EMIT m_bExtension->openUrlNotify();
    Q_EMIT m_bExtension->setLocationBarUrl(url.toDisplayString());
    openUrl(url);
}

void Part::openUrlFromBookmarks(const QUrl &_url)
{
    QUrl url = _url;
    Okular::DocumentViewport vp(_url.fragment(QUrl::FullyDecoded));
    if (vp.isValid()) {
        m_document->setNextDocumentViewport(vp);
    }
    url.setFragment(QString());
    if (m_document->currentDocument() == url) {
        if (vp.isValid()) {
            m_document->setViewport(vp);
        }
    } else {
        openUrl(url);
    }
}

void Part::handleDroppedUrls(const QList<QUrl> &urls)
{
    if (urls.isEmpty()) {
        return;
    }

    if (m_embedMode != NativeShellMode || !openNewFilesInTabs()) {
        openUrlFromDocument(urls.first());
        return;
    }

    Q_EMIT urlsDropped(urls);
}

void Part::slotJobStarted(KIO::Job *job)
{
    if (job) {
        QStringList supportedMimeTypes = m_document->supportedMimeTypes();
        job->addMetaData(QStringLiteral("accept"), supportedMimeTypes.join(QStringLiteral(", ")) + QStringLiteral(", */*;q=0.5"));

        connect(job, &KJob::result, this, &Part::slotJobFinished);
    }
}

void Part::slotJobFinished(KJob *job)
{
    if (job->error() == KIO::ERR_USER_CANCELED) {
        m_pageView->displayMessage(i18n("The loading of %1 has been canceled.", realUrl().toDisplayString(QUrl::PreferLocalFile)));
    }
}

void Part::loadCancelled(const QString &reason)
{
    Q_EMIT setWindowCaption(QString());
    resetStartArguments();

    // when m_viewportDirty.pageNumber != -1 we come from slotAttemptReload
    // so we don't want to show an ugly messagebox just because the document is
    // taking more than usual to be recreated
    if (m_viewportDirty.pageNumber == -1) {
        if (m_urlWithFragment.isValid() && !m_urlWithFragment.isLocalFile()) {
            tryOpeningUrlWithFragmentAsName();
        } else if (!reason.isEmpty()) {
            KMessageBox::error(widget(), i18n("Could not open %1. Reason: %2", url().toDisplayString(), reason));
        }
    }
}

void Part::setWindowTitleFromDocument()
{
    // If 'DocumentTitle' should be used, check if the document has one. If
    // either case is false, use the file name.
    QString title = Okular::Settings::displayDocumentNameOrPath() == Okular::Settings::EnumDisplayDocumentNameOrPath::Path ? realUrl().toDisplayString(QUrl::PreferLocalFile) : realUrl().fileName();

    if (Okular::Settings::displayDocumentTitle()) {
        const QString docTitle = m_document->metaData(QStringLiteral("DocumentTitle")).toString();
        if (!docTitle.isEmpty() && !docTitle.trimmed().isEmpty()) {
            title = docTitle;
        }
    }

    Q_EMIT setWindowCaption(title);
}

KConfigDialog *Part::slotGeneratorPreferences()
{
    // Create dialog
    KConfigDialog *dialog = new Okular::BackendConfigDialog(m_pageView, QStringLiteral("generator_prefs"), Okular::Settings::self());
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    if (m_embedMode == ViewerWidgetMode) {
        dialog->setWindowTitle(i18n("Configure Viewer Backends"));
    } else {
        dialog->setWindowTitle(i18n("Configure Backends"));
    }

    m_document->fillConfigDialog(dialog);

    // Show it
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->show();

    return dialog;
}

void Part::notifySetup(const QVector<Okular::Page *> & /*pages*/, int setupFlags)
{
    // Hide the migration message if the user has just migrated. Otherwise,
    // if m_migrationMessage is already hidden, this does nothing.
    if (!m_document->isDocdataMigrationNeeded()) {
        m_migrationMessage->animatedHide();
    }

    if (!(setupFlags & Okular::DocumentObserver::DocumentChanged)) {
        return;
    }

    rebuildBookmarkMenu();
    updateAboutBackendAction();
    m_findBar->resetSearch();
    m_searchWidget->setEnabled(m_document->supportsSearching());
}

void Part::notifyViewportChanged(bool /*smoothMove*/)
{
    updateViewActions();
}

void Part::notifyPageChanged(int page, int flags)
{
    if (!(flags & Okular::DocumentObserver::Bookmark)) {
        return;
    }

    rebuildBookmarkMenu();
    if (page == m_document->viewport().pageNumber) {
        updateBookmarksActions();
    }
}

void Part::goToPage(uint page)
{
    if (page <= m_document->pages()) {
        m_document->setViewportPage(page - 1);
    }
}

void Part::openDocument(const QString &doc)
{
    openUrl(QUrl::fromUserInput(doc));
}

uint Part::pages()
{
    return m_document->pages();
}

uint Part::currentPage()
{
    return m_document->pages() ? m_document->currentPage() + 1 : 0;
}

QString Part::currentDocument()
{
    return m_document->currentDocument().toDisplayString(QUrl::PreferLocalFile);
}

QString Part::documentMetaData(const QString &metaData) const
{
    const Okular::DocumentInfo info = m_document->documentInfo();
    return info.get(metaData);
}

bool Part::slotImportPSFile()
{
    QString app = QStandardPaths::findExecutable(QStringLiteral("ps2pdf"));
    if (app.isEmpty()) {
        // TODO point the user to their distro packages?
        KMessageBox::error(widget(), i18n("The program \"ps2pdf\" was not found, so Okular can not import PS files using it."), i18n("ps2pdf not found"));
        return false;
    }

    QMimeDatabase mimeDatabase;
    QString filter = i18n("PostScript files (%1)", mimeDatabase.mimeTypeForName(QStringLiteral("application/postscript")).globPatterns().join(QLatin1Char(' ')));

    QUrl url = QFileDialog::getOpenFileUrl(widget(), QString(), QUrl(), filter);
    if (url.isLocalFile()) {
        QTemporaryFile tf(QDir::tempPath() + QLatin1String("/okular_XXXXXX.pdf"));
        tf.setAutoRemove(false);
        if (!tf.open()) {
            return false;
        }
        m_temporaryLocalFile = tf.fileName();
        tf.close();

        setLocalFilePath(url.toLocalFile());
        QStringList args;
        QProcess *p = new QProcess();
        args << url.toLocalFile() << m_temporaryLocalFile;
        m_pageView->displayMessage(i18n("Importing PS file as PDF (this may take a while)..."));
        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Part::psTransformEnded);
        p->start(app, args);
        return true;
    }

    m_temporaryLocalFile.clear();
    return false;
}

void Part::setFileToWatch(const QString &filePath)
{
    if (!m_watchedFilePath.isEmpty()) {
        unsetFileToWatch();
    }

    const QFileInfo fi(filePath);

    m_watchedFilePath = filePath;
    m_watcher->addFile(m_watchedFilePath);

    if (fi.isSymLink()) {
        m_watchedFileSymlinkTarget = fi.symLinkTarget();
        m_watcher->addFile(m_watchedFileSymlinkTarget);
    } else {
        m_watchedFileSymlinkTarget.clear();
    }
}

void Part::unsetFileToWatch()
{
    if (m_watchedFilePath.isEmpty()) {
        return;
    }

    m_watcher->removeFile(m_watchedFilePath);

    if (!m_watchedFileSymlinkTarget.isEmpty()) {
        m_watcher->removeFile(m_watchedFileSymlinkTarget);
    }

    m_watchedFilePath.clear();
    m_watchedFileSymlinkTarget.clear();
}

Document::OpenResult Part::doOpenFile(const QMimeType &mimeA, const QString &fileNameToOpenA, bool *isCompressedFile)
{
    QMimeDatabase db;
    Document::OpenResult openResult = Document::OpenError;
    bool uncompressOk = true;
    QMimeType mime = mimeA;
    QString fileNameToOpen = fileNameToOpenA;
    KFilterDev::CompressionType compressionType = compressionTypeFor(mime.name());
    if (compressionType != KFilterDev::None) {
        *isCompressedFile = true;
        uncompressOk = handleCompressed(fileNameToOpen, localFilePath(), compressionType);
        mime = db.mimeTypeForFile(fileNameToOpen);
    } else {
        *isCompressedFile = false;
    }

    if (m_swapInsteadOfOpening) {
        m_swapInsteadOfOpening = false;

        if (!uncompressOk) {
            return Document::OpenError;
        }

        if (mime.inherits(QStringLiteral("application/vnd.kde.okular-archive"))) {
            isDocumentArchive = true;
            if (!m_document->swapBackingFileArchive(fileNameToOpen, url())) {
                return Document::OpenError;
            }
        } else {
            isDocumentArchive = false;
            if (!m_document->swapBackingFile(fileNameToOpen, url())) {
                return Document::OpenError;
            }
        }

        m_fileLastModified = QFileInfo(localFilePath()).lastModified();
        return Document::OpenSuccess;
    }

    isDocumentArchive = false;
    if (uncompressOk) {
        if (mime.inherits(QStringLiteral("application/vnd.kde.okular-archive"))) {
            openResult = m_document->openDocumentArchive(fileNameToOpen, url());
            isDocumentArchive = true;
        } else {
            openResult = m_document->openDocument(fileNameToOpen, url(), mime);
        }
        m_documentOpenWithPassword = false;

#if HAVE_KWALLET
        // if the file didn't open correctly it might be encrypted, so ask for a pass
        QString walletName, walletFolder, walletKey;
        m_document->walletDataForFile(fileNameToOpen, &walletName, &walletFolder, &walletKey);
        bool firstInput = true;
        bool triedWallet = false;
        KWallet::Wallet *wallet = nullptr;
        bool keep = true;
        while (openResult == Document::OpenNeedsPassword) {
            QString password;

            // 1.A. try to retrieve the first password from the kde wallet system
            if (!triedWallet && !walletKey.isNull()) {
                const WId parentwid = widget()->effectiveWinId();
                wallet = KWallet::Wallet::openWallet(walletName, parentwid);
                if (wallet) {
                    // use the KPdf folder (and create if missing)
                    if (!wallet->hasFolder(walletFolder)) {
                        wallet->createFolder(walletFolder);
                    }
                    wallet->setFolder(walletFolder);

                    // look for the pass in that folder
                    QString retrievedPass;
                    if (!wallet->readPassword(walletKey, retrievedPass)) {
                        password = retrievedPass;
                    }
                }
                triedWallet = true;
            }

            // 1.B. if not retrieved, ask the password using the kde password dialog
            if (password.isNull()) {
                QString prompt;
                if (firstInput) {
                    prompt = i18n("Please enter the password to read the document:");
                } else {
                    prompt = i18n("Incorrect password. Try again:");
                }
                firstInput = false;

                // if the user presses cancel, abort opening
                KPasswordDialog dlg(widget(), wallet ? KPasswordDialog::ShowKeepPassword : KPasswordDialog::KPasswordDialogFlags());
                dlg.setWindowTitle(i18n("Document Password"));
                dlg.setPrompt(prompt);
                if (!dlg.exec()) {
                    break;
                }
                password = dlg.password();
                if (wallet) {
                    keep = dlg.keepPassword();
                }
            }

            // 2. reopen the document using the password
            if (mime.inherits(QStringLiteral("application/vnd.kde.okular-archive"))) {
                openResult = m_document->openDocumentArchive(fileNameToOpen, url(), password);
                isDocumentArchive = true;
            } else {
                openResult = m_document->openDocument(fileNameToOpen, url(), mime, password);
            }

            if (openResult == Document::OpenSuccess) {
                m_documentOpenWithPassword = true;

                // 3. if the password is correct and the user chose to remember it, store it to the wallet
                if (wallet && /*safety check*/ wallet->isOpen() && keep) {
                    wallet->writePassword(walletKey, password);
                }
            }
        }
#endif
    }

    if (openResult == Document::OpenSuccess) {
        m_fileLastModified = QFileInfo(localFilePath()).lastModified();
        m_warnedAboutModifyingUnsaveableDocument = false;
    }
    return openResult;
}

bool Part::openFile()
{
    QList<QMimeType> mimes;
    QString fileNameToOpen = localFilePath();
    const bool isstdin = url().isLocalFile() && url().fileName() == QLatin1String("-");
    const QFileInfo fileInfo(fileNameToOpen);
    if ((!isstdin) && (!fileInfo.exists())) {
        return false;
    }
    QMimeDatabase db;
    QMimeType pathMime = db.mimeTypeForFile(fileNameToOpen);
    if (!arguments().mimeType().isEmpty()) {
        QMimeType argMime = db.mimeTypeForName(arguments().mimeType());

        // Select the "childmost" mimetype, if none of them
        // inherits the other trust more what pathMime says
        // but still do a second try if that one fails
        if (argMime.inherits(pathMime.name())) {
            mimes << argMime;
        } else if (pathMime.inherits(argMime.name())) {
            mimes << pathMime;
        } else {
            mimes << pathMime << argMime;
        }

        // text is super annoying because it always succeeds when opening so try to make sure
        // that we don't set it as first mime unless we're really sure it is that.
        // If it could be something else based on the content we try that first but only if that content itself
        // is not text or if it's a supported text child like markdown
        if (mimes[0].inherits(QStringLiteral("text/plain"))) {
            const QMimeType contentMime = db.mimeTypeForFile(fileNameToOpen, QMimeDatabase::MatchContent);
            if (!contentMime.inherits(QStringLiteral("text/plain"))) {
                mimes.prepend(contentMime);
            } else if (contentMime.name() != QLatin1String("text/plain")) {
                const QStringList supportedMimes = m_document->supportedMimeTypes();
                if (supportedMimes.contains(contentMime.name())) {
                    mimes.prepend(contentMime);
                }
            }
        }
    } else {
        mimes << pathMime;
    }

    QMimeType mime;
    Document::OpenResult openResult = Document::OpenError;
    bool isCompressedFile = false;
    while (!mimes.isEmpty() && openResult == Document::OpenError) {
        mime = mimes.takeFirst();
        openResult = doOpenFile(mime, fileNameToOpen, &isCompressedFile);
    }

    bool canSearch = m_document->supportsSearching();
    Q_EMIT mimeTypeChanged(mime);

    // update one-time actions
    const bool ok = openResult == Document::OpenSuccess;
    Q_EMIT enableCloseAction(ok);
    m_find->setEnabled(ok && canSearch);
    m_findNext->setEnabled(ok && canSearch);
    m_findPrev->setEnabled(ok && canSearch);
    if (m_save) {
        m_save->setEnabled(ok && !(isstdin || mime.inherits(QStringLiteral("inode/directory"))));
    }
    if (m_saveAs) {
        m_saveAs->setEnabled(ok && !(isstdin || mime.inherits(QStringLiteral("inode/directory"))));
    }
    Q_EMIT enablePrintAction(ok && m_document->printingSupport() != Okular::Document::NoPrinting);
    m_printPreview->setEnabled(ok && m_document->printingSupport() != Okular::Document::NoPrinting);
    m_showProperties->setEnabled(ok);
    if (m_openContainingFolder) {
        m_openContainingFolder->setEnabled(ok);
    }
    bool hasEmbeddedFiles = ok && m_document->embeddedFiles() && m_document->embeddedFiles()->count() > 0;
    if (m_showEmbeddedFiles) {
        m_showEmbeddedFiles->setEnabled(hasEmbeddedFiles);
    }
    m_topMessage->setVisible(hasEmbeddedFiles && Okular::Settings::showEmbeddedContentMessages());
    m_migrationMessage->setVisible(m_document->isDocdataMigrationNeeded());

    // Warn the user that XFA forms are not supported yet (NOTE: poppler generator only)
    if (ok && Okular::Settings::showEmbeddedContentMessages() && m_document->metaData(QStringLiteral("HasUnsupportedXfaForm")).toBool() == true) {
        m_formsMessage->setText(i18n("This document has XFA forms, which are currently <b>unsupported</b>."));
        m_formsMessage->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
        m_formsMessage->setMessageType(KMessageWidget::Warning);
        m_formsMessage->setVisible(true);
    }
    // m_pageView->toggleFormsAction() may be null on dummy mode
    else if (ok && Okular::Settings::showEmbeddedContentMessages() && m_pageView->toggleFormsAction() && m_pageView->toggleFormsAction()->isEnabled()) {
        m_formsMessage->setText(i18n("This document has forms. Click on the button to interact with them, or use View -> Show Forms."));
        m_formsMessage->setMessageType(KMessageWidget::Information);
        m_formsMessage->setVisible(true);
    } else {
        m_formsMessage->setVisible(false);
    }

    if (ok) {
        KMessageWidget::MessageType messageType;
        QString message;

        std::tie(messageType, message) = SignatureGuiUtils::documentSignatureMessageWidgetText(m_document);

        if (!message.isEmpty()) {
            if (m_embedMode == PrintPreviewMode) {
                if (Okular::Settings::showEmbeddedContentMessages()) {
                    m_signatureMessage->setText(i18n("All editing and interactive features for this document are disabled. Please save a copy and reopen to edit this document."));
                    m_signatureMessage->setVisible(true);
                }
            } else {
                if (Okular::Settings::showEmbeddedContentMessages() || messageType > KMessageWidget::Information) {
                    m_signatureMessage->setMessageType(messageType);
                    m_signatureMessage->setText(message);
                    m_signatureMessage->setVisible(true);
                }
            }
        }
    }

    if (m_showPresentation) {
        m_showPresentation->setEnabled(ok);
    }
    if (ok) {
        if (m_exportAs) {
            m_exportFormats = m_document->exportFormats();
            QList<Okular::ExportFormat>::ConstIterator it = m_exportFormats.constBegin();
            QList<Okular::ExportFormat>::ConstIterator itEnd = m_exportFormats.constEnd();
            QMenu *menu = m_exportAs->menu();
            for (; it != itEnd; ++it) {
                menu->addAction(actionForExportFormat(*it));
            }
        }
#if HAVE_PURPOSE
        if (m_share) {
            m_shareMenu->model()->setInputData(QJsonObject {{QStringLiteral("mimeType"), mime.name()}, {QStringLiteral("urls"), QJsonArray {url().toString()}}});
            m_shareMenu->model()->setPluginType(QStringLiteral("Export"));
            m_shareMenu->reload();
        }
#endif
        if (isCompressedFile) {
            m_realUrl = url();
        }
#ifdef OKULAR_KEEP_FILE_OPEN
        if (keepFileOpen())
            m_keeper->open(fileNameToOpen);
#endif

        // Tries to find the text passed from terminal after the file is open
        if (!m_textToFindOnOpen.isEmpty()) {
            m_findBar->startSearch(m_textToFindOnOpen);
            m_textToFindOnOpen = QString();
        }
    }
    if (m_exportAsText) {
        m_exportAsText->setEnabled(ok && m_document->canExportToText());
    }
    if (m_exportAs) {
        m_exportAs->setEnabled(ok);
    }
#if HAVE_PURPOSE
    if (m_share) {
        m_share->setEnabled(ok);
    }
#endif

    // update viewing actions
    updateViewActions();

    m_fileWasRemoved = false;

    if (!ok) {
        // if can't open document, update windows so they display blank contents
        m_pageView->viewport()->update();
        m_thumbnailList->update();
        setUrl(QUrl());
        return false;
    }

    // set the file to the fileWatcher
    if (url().isLocalFile()) {
        setFileToWatch(localFilePath());
    }

    // if the 'OpenTOC' flag is set, open the TOC
    if (m_document->metaData(QStringLiteral("OpenTOC")).toBool() && m_tocEnabled && m_sidebar->currentItem() != m_toc) {
        m_sidebar->setCurrentItem(m_toc);
    }
    // if the 'StartFullScreen' flag is set and we're not in viewer widget mode, or the command line flag was
    // specified, start presentation
    const bool presentationBecauseOfDocumentMetadata = (m_embedMode != ViewerWidgetMode) && m_document->metaData(QStringLiteral("StartFullScreen")).toBool();
    if ((presentationBecauseOfDocumentMetadata || m_cliPresentation) && !m_isReloading) {
        bool goAheadWithPresentationMode = true;
        if (!m_cliPresentation) {
            const QString text = i18n(
                "This document wants to be shown full screen.\n"
                "Leave normal mode and enter presentation mode?");
            const QString caption = i18n("Request to Change Viewing Mode");
            const KGuiItem yesItem = KGuiItem(i18n("Enter Presentation Mode"), QStringLiteral("dialog-ok"));
            const KGuiItem noItem = KGuiItem(i18n("Deny Request"), QStringLiteral("dialog-cancel"));
            const int result = KMessageBox::questionYesNo(widget(), text, caption, yesItem, noItem);
            if (result == KMessageBox::No) {
                goAheadWithPresentationMode = false;
            }
        }
        m_cliPresentation = false;
        if (goAheadWithPresentationMode) {
            QMetaObject::invokeMethod(this, "slotShowPresentation", Qt::QueuedConnection);
        }
    }
    m_generatorGuiClient = factory() ? m_document->guiClient() : nullptr;
    if (m_generatorGuiClient) {
        factory()->addClient(m_generatorGuiClient);
    }
    if (m_cliPrint) {
        m_cliPrint = false;
        slotPrint();
    } else if (m_cliPrintAndExit) {
        slotPrint();
    }
    return true;
}

bool Part::openUrl(const QUrl &url)
{
    return openUrl(url, false /* swapInsteadOfOpening */);
}

bool Part::openUrl(const QUrl &_url, bool swapInsteadOfOpening)
{
    /* Store swapInsteadOfOpening, so that closeUrl and openFile will be able
     * to read it */
    m_swapInsteadOfOpening = swapInsteadOfOpening;

    // The subsequent call to closeUrl clears the arguments.
    // We want to save them and restore them later.
    const KParts::OpenUrlArguments args = arguments();

    // Close current document if any
    if (!closeUrl()) {
        return false;
    }

    setArguments(args);

    QUrl url(_url);
    if (url.hasFragment()) {
        m_urlWithFragment = _url;
        const QString dest = url.fragment(QUrl::FullyDecoded);
        bool ok = true;
        int page = dest.toInt(&ok);

        if (!ok) {
            const QStringList parameters = dest.split(QLatin1Char('&'));
            for (const QString &parameter : parameters) {
                if (parameter.startsWith(QStringLiteral("page="), Qt::CaseInsensitive)) {
                    page = dest.midRef(5).toInt(&ok);
                }
            }
        }

        if (ok) {
            Okular::DocumentViewport vp(page - 1);
            vp.rePos.enabled = true;
            vp.rePos.normalizedX = 0;
            vp.rePos.normalizedY = 0;
            vp.rePos.pos = Okular::DocumentViewport::TopLeft;
            m_document->setNextDocumentViewport(vp);
        } else {
            m_document->setNextDocumentDestination(dest);
        }
        url.setFragment(QString());
    } else {
        m_urlWithFragment.clear();
    }

    // this calls in sequence the 'closeUrl' and 'openFile' methods
    bool openOk = KParts::ReadWritePart::openUrl(url);

    if (openOk) {
        m_viewportDirty.pageNumber = -1;

        setWindowTitleFromDocument();
    } else {
        if (m_urlWithFragment.isValid() && m_urlWithFragment.isLocalFile()) {
            openOk = tryOpeningUrlWithFragmentAsName();
        } else {
            resetStartArguments();
            /* TRANSLATORS: Adding the reason (%2) why the opening failed (if any). */
            QString errorMessage = i18n("Could not open %1. %2", url.toDisplayString(), QStringLiteral("\n%1").arg(m_document->openError()));
            KMessageBox::error(widget(), errorMessage);
        }
    }

    return openOk;
}

bool Part::tryOpeningUrlWithFragmentAsName()
{
    QUrl url = m_urlWithFragment;
    url.setPath(url.path() + QLatin1Char('#') + url.fragment());
    url.setFragment(QString());
    return openUrl(url);
}

bool Part::queryClose()
{
    if (!isReadWrite() || !isModified()) {
        return true;
    }

    // TODO When we get different saving backends we need to query the backend
    // as to if it can save changes even if the open file has been modified,
    // since we only have poppler as saving backend for now we're skipping that check
    if (m_fileLastModified != QFileInfo(localFilePath()).lastModified()) {
        int res;
        if (m_isReloading) {
            res = KMessageBox::warningYesNo(widget(),
                                            xi18nc("@info",
                                                   "The file <filename>%1</filename> has unsaved changes but has been modified by another program. Reloading it "
                                                   "will replace the unsaved changes with the changes made in the other "
                                                   "program.<nl/><nl/>Do you want to continue reloading the file?",
                                                   url().fileName()),
                                            i18n("File Changed"),
                                            KGuiItem(i18n("Continue Reloading")), // <- KMessageBox::Yes
                                            KGuiItem(i18n("Abort Reloading")));
        } else {
            res = KMessageBox::warningYesNo(widget(),
                                            xi18nc("@info",
                                                   "The file <filename>%1</filename> has unsaved changes but has been modified by another program. Closing it "
                                                   "will replace the unsaved changes with the changes made in the other "
                                                   "program.<nl/><nl/>Do you want to continue closing the file?",
                                                   url().fileName()),
                                            i18n("File Changed"),
                                            KGuiItem(i18n("Continue Closing")), // <- KMessageBox::Yes
                                            KGuiItem(i18n("Abort Closing")));
        }
        return res == KMessageBox::Yes;
    }

    // Not all things are saveable (e.g. files opened from stdin, folders)
    if (m_save->isEnabled()) {
        const int res = KMessageBox::warningYesNoCancel(widget(), i18n("Do you want to save your changes to \"%1\" or discard them?", url().fileName()), i18n("Close Document"), KStandardGuiItem::save(), KStandardGuiItem::discard());

        switch (res) {
        case KMessageBox::Yes: // Save
            saveFile();
            return !isModified(); // Only allow closing if file was really saved
        case KMessageBox::No:     // Discard
            return true;
        default: // Cancel
            return false;
        }
    } else {
        return true;
    }
}

bool Part::closeUrl(bool promptToSave)
{
    if (promptToSave && !queryClose()) {
        return false;
    }

    if (m_swapInsteadOfOpening) {
        // If we're swapping the backing file, we don't want to close the
        // current one when openUrl() calls us internally
        return true; // pretend it worked
    }

    m_document->setHistoryClean(true);

    if (!m_temporaryLocalFile.isNull() && m_temporaryLocalFile != localFilePath()) {
        QFile::remove(m_temporaryLocalFile);
        m_temporaryLocalFile.clear();
    }

    slotHidePresentation();
    Q_EMIT enableCloseAction(false);
    m_find->setEnabled(false);
    m_findNext->setEnabled(false);
    m_findPrev->setEnabled(false);
    if (m_save) {
        m_save->setEnabled(false);
    }
    if (m_saveAs) {
        m_saveAs->setEnabled(false);
    }
    m_printPreview->setEnabled(false);
    m_showProperties->setEnabled(false);
    if (m_showEmbeddedFiles) {
        m_showEmbeddedFiles->setEnabled(false);
    }
    if (m_exportAs) {
        m_exportAs->setEnabled(false);
    }
    if (m_exportAsText) {
        m_exportAsText->setEnabled(false);
    }
    m_exportFormats.clear();
    if (m_exportAs) {
        QMenu *menu = m_exportAs->menu();
        QList<QAction *> acts = menu->actions();
        int num = acts.count();
        for (int i = 1; i < num; ++i) {
            menu->removeAction(acts.at(i));
            delete acts.at(i);
        }
    }
#if HAVE_PURPOSE
    if (m_share) {
        m_share->setEnabled(false);
        m_shareMenu->clear();
    }
#endif
    if (m_showPresentation) {
        m_showPresentation->setEnabled(false);
    }
    Q_EMIT setWindowCaption(QLatin1String(""));
    Q_EMIT enablePrintAction(false);
    m_realUrl = QUrl();
    if (url().isLocalFile()) {
        unsetFileToWatch();
    }
    m_fileWasRemoved = false;
    if (m_generatorGuiClient) {
        factory()->removeClient(m_generatorGuiClient);
    }
    m_generatorGuiClient = nullptr;
    m_document->closeDocument();
    m_fileLastModified = QDateTime();
    updateViewActions();
    delete m_tempfile;
    m_tempfile = nullptr;
    if (widget()) {
        m_searchWidget->clearText();
        m_migrationMessage->setVisible(false);
        m_topMessage->setVisible(false);
        m_formsMessage->setVisible(false);
        m_signatureMessage->setVisible(false);
    }
#ifdef OKULAR_KEEP_FILE_OPEN
    m_keeper->close();
#endif
    bool r = KParts::ReadWritePart::closeUrl();
    setUrl(QUrl());

    return r;
}

bool Part::closeUrl()
{
    return closeUrl(true);
}

void Part::guiActivateEvent(KParts::GUIActivateEvent *event)
{
    updateViewActions();

    KParts::ReadWritePart::guiActivateEvent(event);

    setWindowTitleFromDocument();

    if (event->activated()) {
        m_pageView->setupActionsPostGUIActivated();
        rebuildBookmarkMenu();
    }
}

void Part::close()
{
    if (m_embedMode == NativeShellMode) {
        closeUrl();
    } else {
        KMessageBox::information(widget(), i18n("This link points to a close document action that does not work when using the embedded viewer."), QString(), QStringLiteral("warnNoCloseIfNotInOkular"));
    }
}

void Part::cannotQuit()
{
    KMessageBox::information(widget(), i18n("This link points to a quit application action that does not work when using the embedded viewer."), QString(), QStringLiteral("warnNoQuitIfNotInOkular"));
}

void Part::slotShowLeftPanel()
{
    bool showLeft = m_showLeftPanel->isChecked();
    Okular::Settings::setShowLeftPanel(showLeft);
    Okular::Settings::self()->save();
    // show/hide left panel
    m_sidebar->setSidebarVisibility(showLeft);
}

void Part::slotShowBottomBar()
{
    const bool showBottom = m_showBottomBar->isChecked();
    Okular::Settings::setShowBottomBar(showBottom);
    Okular::Settings::self()->save();
    // show/hide bottom bar
    m_bottomBar->setVisible(showBottom);
}

void Part::slotFileDirty(const QString &path)
{
    // The beauty of this is that each start cancels the previous one.
    // This means that timeout() is only fired when there have
    // no changes to the file for the last 750 millisecs.
    // This ensures that we don't update on every other byte that gets
    // written to the file.
    if (path == localFilePath()) {
        // Only start watching the file in case if it wasn't removed
        if (QFile::exists(localFilePath())) {
            m_dirtyHandler->start(750);
        } else {
            m_fileWasRemoved = true;
        }
    } else {
        const QFileInfo fi(localFilePath());
        if (fi.absolutePath() == path) {
            // Our parent has been dirtified
            if (!QFile::exists(localFilePath())) {
                m_fileWasRemoved = true;
            } else if (m_fileWasRemoved && QFile::exists(localFilePath())) {
                // we need to watch the new file
                unsetFileToWatch();
                setFileToWatch(localFilePath());
                m_dirtyHandler->start(750);
            }
        } else if (fi.isSymLink() && fi.symLinkTarget() == path) {
            if (QFile::exists(fi.symLinkTarget())) {
                m_dirtyHandler->start(750);
            } else {
                m_fileWasRemoved = true;
            }
        }
    }
}

// Attempt to reload the document, one or more times, optionally from a different URL
bool Part::slotAttemptReload(bool oneShot, const QUrl &newUrl)
{
    // Skip reload when another reload is already in progress
    if (m_isReloading) {
        return false;
    }
    QScopedValueRollback<bool> rollback(m_isReloading, true);

    bool tocReloadPrepared = false;

    // do the following the first time the file is reloaded
    if (m_viewportDirty.pageNumber == -1) {
        // store the url of the current document
        m_oldUrl = newUrl.isEmpty() ? url() : newUrl;

        // store the current viewport
        m_viewportDirty = m_document->viewport();

        // store the current toolbox pane
        m_dirtyToolboxItem = m_sidebar->currentItem();
        m_wasSidebarVisible = m_sidebar->isSidebarVisible();

        // store if presentation view was open
        m_wasPresentationOpen = ((PresentationWidget *)m_presentationWidget != nullptr);

        // preserves the toc state after reload
        m_toc->prepareForReload();
        tocReloadPrepared = true;

        // store the page rotation
        m_dirtyPageRotation = m_document->rotation();

        // inform the user about the operation in progress
        // TODO: Remove this line and integrate reload info in queryClose
        m_pageView->displayMessage(i18n("Reloading the document..."));
    }

    // close and (try to) reopen the document
    if (!closeUrl()) {
        m_viewportDirty.pageNumber = -1;

        if (tocReloadPrepared) {
            m_toc->rollbackReload();
        }
        return false;
    }

    if (tocReloadPrepared) {
        m_toc->finishReload();
    }

    // inform the user about the operation in progress
    m_pageView->displayMessage(i18n("Reloading the document..."));

    bool reloadSucceeded = false;

    if (KParts::ReadWritePart::openUrl(m_oldUrl)) {
        // on successful opening, restore the previous viewport
        if (m_viewportDirty.pageNumber >= (int)m_document->pages()) {
            m_viewportDirty.pageNumber = (int)m_document->pages() - 1;
        }
        m_document->setViewport(m_viewportDirty);
        m_oldUrl = QUrl();
        m_viewportDirty.pageNumber = -1;
        m_document->setRotation(m_dirtyPageRotation);
        if (m_sidebar->currentItem() != m_dirtyToolboxItem) {
            m_sidebar->setCurrentItem(m_dirtyToolboxItem);
        }
        if (m_sidebar->isSidebarVisible() != m_wasSidebarVisible) {
            m_sidebar->setSidebarVisibility(m_wasSidebarVisible);
        }
        if (m_wasPresentationOpen) {
            slotShowPresentation();
        }
        Q_EMIT enablePrintAction(true && m_document->printingSupport() != Okular::Document::NoPrinting);

        reloadSucceeded = true;
    } else if (!oneShot) {
        // start watching the file again (since we dropped it on close)
        setFileToWatch(localFilePath());
        m_dirtyHandler->start(750);
    }

    return reloadSucceeded;
}

void Part::updateViewActions()
{
    bool opened = m_document->pages() > 0;
    if (opened) {
        m_gotoPage->setEnabled(m_document->pages() > 1);

        // Check if you are at the beginning or not
        if (m_document->currentPage() != 0) {
            m_beginningOfDocument->setEnabled(true);
            m_prevPage->setEnabled(true);
        } else {
            if (m_pageView->verticalScrollBar()->value() != 0) {
                // The page isn't at the very beginning
                m_beginningOfDocument->setEnabled(true);
            } else {
                // The page is at the very beginning of the document
                m_beginningOfDocument->setEnabled(false);
            }
            // The document is at the first page, you can go to a page before
            m_prevPage->setEnabled(false);
        }

        if (m_document->pages() == m_document->currentPage() + 1) {
            // If you are at the end, disable go to next page
            m_nextPage->setEnabled(false);
            if (m_pageView->verticalScrollBar()->value() == m_pageView->verticalScrollBar()->maximum()) {
                // If you are the end of the page of the last document, you can't go to the last page
                m_endOfDocument->setEnabled(false);
            } else {
                // Otherwise you can move to the endif
                m_endOfDocument->setEnabled(true);
            }
        } else {
            // If you are not at the end, enable go to next page
            m_nextPage->setEnabled(true);
            m_endOfDocument->setEnabled(true);
        }

        if (m_historyBack) {
            m_historyBack->setEnabled(!m_document->historyAtBegin());
        }
        if (m_historyNext) {
            m_historyNext->setEnabled(!m_document->historyAtEnd());
        }
        m_reload->setEnabled(true);
        if (m_copy) {
            m_copy->setEnabled(true);
        }
        if (m_selectAll) {
            m_selectAll->setEnabled(true);
        }
        if (m_selectCurrentPage) {
            m_selectCurrentPage->setEnabled(true);
        }
    } else {
        m_gotoPage->setEnabled(false);
        m_beginningOfDocument->setEnabled(false);
        m_endOfDocument->setEnabled(false);
        m_prevPage->setEnabled(false);
        m_nextPage->setEnabled(false);
        if (m_historyBack) {
            m_historyBack->setEnabled(false);
        }
        if (m_historyNext) {
            m_historyNext->setEnabled(false);
        }
        m_reload->setEnabled(false);
        if (m_copy) {
            m_copy->setEnabled(false);
        }
        if (m_selectAll) {
            m_selectAll->setEnabled(false);
        }
        if (m_selectCurrentPage) {
            m_selectCurrentPage->setEnabled(false);
        }
    }

    if (factory()) {
        QWidget *menu = factory()->container(QStringLiteral("menu_okular_part_viewer"), this);
        if (menu) {
            menu->setEnabled(opened);
        }

        menu = factory()->container(QStringLiteral("view_orientation"), this);
        if (menu) {
            menu->setEnabled(opened);
        }
    }
    Q_EMIT viewerMenuStateChange(opened);

    updateBookmarksActions();
}

void Part::updateBookmarksActions()
{
    bool opened = m_document->pages() > 0;
    if (opened) {
        m_addBookmark->setEnabled(true);
        if (m_document->bookmarkManager()->isBookmarked(m_document->viewport())) {
            m_addBookmark->setText(i18n("Remove Bookmark"));
            m_addBookmark->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-remove"), QIcon::fromTheme(QStringLiteral("edit-delete-bookmark"))));
            m_renameBookmark->setEnabled(true);
        } else {
            m_addBookmark->setText(m_addBookmarkText);
            m_addBookmark->setIcon(m_addBookmarkIcon);
            m_renameBookmark->setEnabled(false);
        }
    } else {
        m_addBookmark->setEnabled(false);
        m_addBookmark->setText(m_addBookmarkText);
        m_addBookmark->setIcon(m_addBookmarkIcon);
        m_renameBookmark->setEnabled(false);
    }
}

void Part::enableTOC(bool enable)
{
    if (!enable) {
        m_tocEnabled = false;
        return;
    }

    m_sidebar->addItem(m_toc, QIcon::fromTheme(QApplication::isLeftToRight() ? QStringLiteral("format-justify-left") : QStringLiteral("format-justify-right")), i18n("Contents"));
    m_tocEnabled = true;

    // If present, show the TOC when a document is opened
    if (m_sidebar->currentItem() != m_toc) {
        m_sidebar->setCurrentItem(m_toc);
    }
}

void Part::slotRebuildBookmarkMenu()
{
    rebuildBookmarkMenu();
}

void Part::enableLayers(bool enable)
{
    if (!enable) {
        return;
    }

    m_sidebar->addItem(m_layers, QIcon::fromTheme(QStringLiteral("format-list-unordered")), i18n("Layers"));
}

void Part::enableSidebarSignaturesItem(bool enable)
{
    if (!enable) {
        return;
    }

    m_sidebar->addItem(m_signaturePanel, QIcon::fromTheme(QStringLiteral("application-pkcs7-signature")), i18n("Signatures"));
}

void Part::slotShowFindBar()
{
    m_findBar->show();
    m_findBar->focusAndSetCursor();
    m_closeFindBar->setEnabled(true);
}

void Part::slotHideFindBar()
{
    if (m_findBar->maybeHide()) {
        m_pageView->setFocus();
        m_closeFindBar->setEnabled(false);
    }
}

// BEGIN go to page dialog
class GotoPageDialog : public QDialog
{
    Q_OBJECT

public:
    GotoPageDialog(QWidget *p, int current, int max)
        : QDialog(p)
    {
        setWindowTitle(i18n("Go to Page"));
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *topLayout = new QVBoxLayout(this);
        topLayout->setContentsMargins(6, 6, 6, 6);
        QHBoxLayout *midLayout = new QHBoxLayout();
        spinbox = new QSpinBox(this);
        spinbox->setRange(1, max);
        spinbox->setValue(current);
        spinbox->setFocus();

        slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(1, max);
        slider->setValue(current);
        slider->setSingleStep(1);
        slider->setTickPosition(QSlider::TicksBelow);
        slider->setTickInterval(max / 10);

        connect(slider, &QSlider::valueChanged, spinbox, &QSpinBox::setValue);
        connect(spinbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), slider, &QSlider::setValue);

        QLabel *label = new QLabel(i18n("&Page:"), this);
        label->setBuddy(spinbox);
        topLayout->addWidget(label);
        topLayout->addLayout(midLayout);
        midLayout->addWidget(slider);
        midLayout->addWidget(spinbox);

        // A little bit extra space
        topLayout->addStretch(10);
        topLayout->addWidget(buttonBox);
        spinbox->setFocus();
    }

    int getPage() const
    {
        return spinbox->value();
    }

protected:
    QSpinBox *spinbox;
    QSlider *slider;
    QDialogButtonBox *buttonBox;
};
// END go to page dialog

void Part::slotGoToPage()
{
    GotoPageDialog pageDialog(m_pageView, m_document->currentPage() + 1, m_document->pages());
    if (pageDialog.exec() == QDialog::Accepted) {
        m_document->setViewportPage(pageDialog.getPage() - 1, nullptr, true);
    }
}

void Part::slotPreviousPage()
{
    if (m_document->isOpened() && !(m_document->currentPage() < 1)) {
        m_document->setViewportPage(m_document->currentPage() - 1, nullptr, true);
    }
}

void Part::slotNextPage()
{
    if (m_document->isOpened() && m_document->currentPage() < (m_document->pages() - 1)) {
        m_document->setViewportPage(m_document->currentPage() + 1, nullptr, true);
    }
}

void Part::slotGotoFirst()
{
    if (m_document->isOpened()) {
        m_document->setViewportPage(0, nullptr, true);
        m_beginningOfDocument->setEnabled(false);
    }
}

void Part::slotGotoLast()
{
    if (m_document->isOpened()) {
        DocumentViewport endPage(m_document->pages() - 1);
        endPage.rePos.enabled = true;
        endPage.rePos.normalizedX = 0;
        endPage.rePos.normalizedY = 1;
        endPage.rePos.pos = Okular::DocumentViewport::TopLeft;
        m_document->setViewport(endPage, nullptr, true);
        m_endOfDocument->setEnabled(false);
    }
}

void Part::slotHistoryBack()
{
    m_document->setPrevViewport();
}

void Part::slotHistoryNext()
{
    m_document->setNextViewport();
}

void Part::slotAddBookmark()
{
    DocumentViewport vp = m_document->viewport();
    if (m_document->bookmarkManager()->isBookmarked(vp)) {
        m_document->bookmarkManager()->removeBookmark(vp);
    } else {
        m_document->bookmarkManager()->addBookmark(vp);
    }
}

void Part::slotRenameBookmark(const DocumentViewport &viewport)
{
    Q_ASSERT(m_document->bookmarkManager()->isBookmarked(viewport));
    if (m_document->bookmarkManager()->isBookmarked(viewport)) {
        KBookmark bookmark = m_document->bookmarkManager()->bookmark(viewport);
        const QString newName = QInputDialog::getText(widget(), i18n("Rename Bookmark"), i18n("Enter the new name of the bookmark:"), QLineEdit::Normal, bookmark.fullText());
        if (!newName.isEmpty()) {
            m_document->bookmarkManager()->renameBookmark(&bookmark, newName);
        }
    }
}

void Part::slotRenameBookmarkFromMenu()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    Q_ASSERT(action);
    if (action) {
        DocumentViewport vp(action->data().toString());
        slotRenameBookmark(vp);
    }
}

void Part::slotRemoveBookmarkFromMenu()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    Q_ASSERT(action);
    if (action) {
        DocumentViewport vp(action->data().toString());
        slotRemoveBookmark(vp);
    }
}

void Part::slotRemoveBookmark(const DocumentViewport &viewport)
{
    Q_ASSERT(m_document->bookmarkManager()->isBookmarked(viewport));
    if (m_document->bookmarkManager()->isBookmarked(viewport)) {
        m_document->bookmarkManager()->removeBookmark(viewport);
    }
}

void Part::slotRenameCurrentViewportBookmark()
{
    slotRenameBookmark(m_document->viewport());
}

bool Part::aboutToShowContextMenu(QMenu * /*menu*/, QAction *action, QMenu *contextMenu)
{
    KBookmarkAction *ba = dynamic_cast<KBookmarkAction *>(action);
    if (ba != nullptr) {
        QAction *separatorAction = contextMenu->addSeparator();
        separatorAction->setObjectName(QStringLiteral("OkularPrivateRenameBookmarkActions"));
        QAction *renameAction = contextMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("Rename this Bookmark"), this, &Part::slotRenameBookmarkFromMenu);
        renameAction->setData(ba->property("htmlRef").toString());
        renameAction->setObjectName(QStringLiteral("OkularPrivateRenameBookmarkActions"));
        QAction *deleteAction = contextMenu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-remove"), QIcon::fromTheme(QStringLiteral("edit-delete-bookmark"))), i18n("Remove this Bookmark"), this, &Part::slotRemoveBookmarkFromMenu);
        deleteAction->setData(ba->property("htmlRef").toString());
        deleteAction->setObjectName(QStringLiteral("OkularPrivateRenameBookmarkActions"));
    }
    return ba;
}

void Part::slotPreviousBookmark()
{
    const KBookmark bookmark = m_document->bookmarkManager()->previousBookmark(m_document->viewport());

    if (!bookmark.isNull()) {
        DocumentViewport vp(bookmark.url().fragment(QUrl::FullyDecoded));
        m_document->setViewport(vp, nullptr, true);
    }
}

void Part::slotNextBookmark()
{
    const KBookmark bookmark = m_document->bookmarkManager()->nextBookmark(m_document->viewport());

    if (!bookmark.isNull()) {
        DocumentViewport vp(bookmark.url().fragment(QUrl::FullyDecoded));
        m_document->setViewport(vp, nullptr, true);
    }
}

void Part::slotFind()
{
    // when in presentation mode, there's already a search bar, taking care of
    // the 'find' requests
    if ((PresentationWidget *)m_presentationWidget != nullptr) {
        m_presentationWidget->slotFind();
    } else {
        slotShowFindBar();
    }
}

void Part::slotFindNext()
{
    if (m_findBar->isHidden()) {
        slotShowFindBar();
    } else {
        m_findBar->findNext();
    }
}

void Part::slotFindPrev()
{
    if (m_findBar->isHidden()) {
        slotShowFindBar();
    } else {
        m_findBar->findPrev();
    }
}

bool Part::saveFile()
{
    if (!isModified()) {
        return true;
    } else {
        return saveAs(url());
    }
}

bool Part::slotSaveFileAs(bool showOkularArchiveAsDefaultFormat)
{
    if (m_embedMode == PrintPreviewMode) {
        return false;
    }

    // Determine the document's mimetype
    QMimeDatabase db;
    QMimeType originalMimeType;
    const QString typeName = m_document->documentInfo().get(DocumentInfo::MimeType);
    if (!typeName.isEmpty()) {
        originalMimeType = db.mimeTypeForName(typeName);
    }

    // What data would we lose if we saved natively?
    bool wontSaveForms, wontSaveAnnotations;
    checkNativeSaveDataLoss(&wontSaveForms, &wontSaveAnnotations);

    const QMimeType okularArchiveMimeType = db.mimeTypeForName(QStringLiteral("application/vnd.kde.okular-archive"));

    // Prepare "Save As" dialog
    const QString originalMimeTypeFilter = i18nc("File type name and pattern", "%1 (%2)", originalMimeType.comment(), originalMimeType.globPatterns().join(QLatin1Char(' ')));
    const QString okularArchiveMimeTypeFilter = i18nc("File type name and pattern", "%1 (%2)", okularArchiveMimeType.comment(), okularArchiveMimeType.globPatterns().join(QLatin1Char(' ')));

    // What format choice should we show as default?
    QString selectedFilter = (isDocumentArchive || showOkularArchiveAsDefaultFormat || wontSaveForms || wontSaveAnnotations) ? okularArchiveMimeTypeFilter : originalMimeTypeFilter;

    QString filter = originalMimeTypeFilter + QStringLiteral(";;") + okularArchiveMimeTypeFilter;

    const QUrl saveUrl = QFileDialog::getSaveFileUrl(widget(), i18n("Save As"), url(), filter, &selectedFilter);

    if (!saveUrl.isValid() || saveUrl.isEmpty()) {
        return false;
    }

    // Has the user chosen to save in .okular archive format?
    const bool saveAsOkularArchive = (selectedFilter == okularArchiveMimeTypeFilter);

    if (saveAsOkularArchive) {
        // Non Plasma file dialogs are terrible and it's very easy to select saving a file as okular archive and call it hello.md
        // and that's bad because it is *not* an .md file so tell the user to fix it
        Q_ASSERT(okularArchiveMimeType.suffixes().count() == 1);
        Q_ASSERT(okularArchiveMimeType.suffixes().at(0) == okularArchiveMimeType.preferredSuffix());
        const QString wantedExtension = QLatin1Char('.') + okularArchiveMimeType.preferredSuffix();
        if (!saveUrl.path().endsWith(wantedExtension)) {
            const auto button = KMessageBox::questionYesNo(widget(),
                                                           i18n("You have chosen to save an Okular Archive without the file name ending with the '%1' extension. That is not allowed, do you want to choose a new name?", wantedExtension),
                                                           i18n("Unsupported extension"),
                                                           KGuiItem(i18nc("@action:button", "Choose New Name"), QStringLiteral("edit-rename")),
                                                           KStandardGuiItem::cancel());

            return button == KMessageBox::Yes ? slotSaveFileAs(showOkularArchiveAsDefaultFormat) : false;
        }
    }

    return saveAs(saveUrl, saveAsOkularArchive ? SaveAsOkularArchive : NoSaveAsFlags);
}

bool Part::saveAs(const QUrl &saveUrl)
{
    // Save in the same format (.okular vs native) as the current file
    return saveAs(saveUrl, isDocumentArchive ? SaveAsOkularArchive : NoSaveAsFlags);
}

static QUrl resolveSymlinksIfFileExists(const QUrl &saveUrl)
{
    if (saveUrl.isLocalFile()) {
        const QFileInfo fi(saveUrl.toLocalFile());
        return fi.exists() ? QUrl::fromLocalFile(fi.canonicalFilePath()) : saveUrl;
    } else {
        return saveUrl;
    }
}

bool Part::saveAs(const QUrl &saveUrl, SaveAsFlags flags)
{
    // TODO When we get different saving backends we need to query the backend
    // as to if it can save changes even if the open file has been modified,
    // since we only have poppler as saving backend for now we're skipping that check

    // Don't warn the user about external changes if what actually happened was that
    // the file on disk was deleted for some reason; in this case just go on normally
    // to avoid confusion or data loss.
    // Also don't warn if the file was modified on disk but the user is doing a Save As
    // with a different URL, since the original changed document is safe so there's
    // nothing to warn about.
    const QFileInfo fi(localFilePath());
    if (fi.exists() && m_fileLastModified != fi.lastModified() && saveUrl == realUrl()) {
        const int res = KMessageBox::warningYesNoCancel(widget(),
                                                        xi18nc("@info",
                                                               "The file <filename>%1</filename> has been modified by another program. If you save now, any "
                                                               "changes made in the other program will be lost. Are you sure you want to continue?",
                                                               realUrl().fileName()),
                                                        i18n("Save - Warning"),
                                                        KStandardGuiItem::cont(),                // <- KMessageBox::Yes
                                                        KGuiItem(i18n("Save a Copy Elsewhere")), // <- KMessageBox::No
                                                        KStandardGuiItem::cancel());             // <- KMessageBox::Cancel

        if (res == KMessageBox::No) {
            slotSaveFileAs(false);
        }
        if (res != KMessageBox::Yes) {
            return false;
        }
    }

    bool hasUserAcceptedReload = false;
    if (m_documentOpenWithPassword) {
        const int res = KMessageBox::warningYesNo(
            widget(),
            i18n("The current document is protected with a password.<br />In order to save, the file needs to be reloaded. You will be asked for the password again and your undo/redo history will be lost.<br />Do you want to continue?"),
            i18n("Save - Warning"),
            KStandardGuiItem::cont(),
            KStandardGuiItem::cancel());

        switch (res) {
        case KMessageBox::Yes:
            hasUserAcceptedReload = true;
            // do nothing
            break;
        case KMessageBox::No: // User said no to continue, so return true even if save didn't happen otherwise we will get an error
            return true;
        }
    }

    bool setModifiedAfterSave = false;

    QString tmpFileName;
    // don't turn the file_copy that use this to a file_move
    // doesn't work on Windows
    bool deleteTmpFileName = false;
    {
        // Own scope for the QTemporaryFile since we only care about the random name
        // we need to destroy it so the file gets deleted otherwise windows will complain
        // when trying to save over it because the file is still open
        QTemporaryFile tf;
        if (!tf.open()) {
            KMessageBox::information(widget(), i18n("Could not open the temporary file for saving."));
            return false;
        }
        tmpFileName = tf.fileName();
    }

    // Figure out the real save url, for symlinks we don't want to copy over the symlink but over the target file
    const QUrl realSaveUrl = resolveSymlinksIfFileExists(saveUrl);

    // Due to the way we write we can overwrite readonly files so check if it's one and just bail out early
    if (realSaveUrl.isLocalFile()) {
        const QFileInfo fi(realSaveUrl.toLocalFile());
        if (fi.exists() && !fi.isWritable()) {
            KMessageBox::information(widget(), xi18nc("@info", "Could not overwrite <filename>%1</filename> because that file is read-only. Try saving to another location or changing that file's permissions.", realSaveUrl.toLocalFile()));
            return false;
        }
    }

    QScopedPointer<QTemporaryFile> tempFile;
    KIO::Job *copyJob = nullptr; // this will be filled with the job that writes to saveUrl

    // Does the user want a .okular archive?
    if (flags & SaveAsOkularArchive) {
        if (!hasUserAcceptedReload && !m_document->canSwapBackingFile()) {
            const int res = KMessageBox::warningYesNo(widget(),
                                                      i18n("After saving, the current document format requires the file to be reloaded. Your undo/redo history will be lost.<br />Do you want to continue?"),
                                                      i18n("Save - Warning"),
                                                      KStandardGuiItem::cont(),
                                                      KStandardGuiItem::cancel());

            switch (res) {
            case KMessageBox::Yes:
                // do nothing
                break;
            case KMessageBox::No: // User said no to continue, so return true even if save didn't happen otherwise we will get an error
                return true;
            }
        }

        if (!m_document->saveDocumentArchive(tmpFileName)) {
            KMessageBox::information(widget(), i18n("File could not be saved in '%1'. Try to save it to another location.", tmpFileName));
            return false;
        }

        copyJob = KIO::file_copy(QUrl::fromLocalFile(tmpFileName), realSaveUrl, -1, KIO::Overwrite);
        deleteTmpFileName = true;
    } else {
        bool wontSaveForms, wontSaveAnnotations;
        checkNativeSaveDataLoss(&wontSaveForms, &wontSaveAnnotations);

        // If something can't be saved in this format, ask for confirmation
        QStringList listOfwontSaves;
        if (wontSaveForms) {
            listOfwontSaves << i18n("Filled form contents");
        }
        if (wontSaveAnnotations) {
            listOfwontSaves << i18n("User annotations");
        }
        if (!listOfwontSaves.isEmpty()) {
            if (saveUrl == url()) {
                // Save
                const QString warningMessage = i18n("You are about to save changes, but the current file format does not support saving the following elements. Please use the <i>Okular document archive</i> format to preserve them.");
                const int result = KMessageBox::warningYesNoList(widget(),
                                                                 warningMessage,
                                                                 listOfwontSaves,
                                                                 i18n("Warning"),
                                                                 KGuiItem(i18n("Save as Okular document archive..."), QStringLiteral("document-save-as")), // <- KMessageBox::Yes
                                                                 KStandardGuiItem::cancel());

                switch (result) {
                case KMessageBox::Yes: // -> Save as Okular document archive
                    return slotSaveFileAs(true /* showOkularArchiveAsDefaultFormat */);
                default:
                    return false;
                }
            } else {
                // Save as
                const QString warningMessage = m_document->canSwapBackingFile() ? i18n(
                                                                                      "You are about to save changes, but the current file format does not support saving the following elements. Please use the <i>Okular document "
                                                                                      "archive</i> format to preserve them. Click <i>Continue</i> to save the document and discard these elements.")
                                                                                : i18n(
                                                                                      "You are about to save changes, but the current file format does not support saving the following elements. Please use the <i>Okular document "
                                                                                      "archive</i> format to preserve them. Click <i>Continue</i> to save, but you will lose these elements as well as the undo/redo history.");
                const QString continueMessage = m_document->canSwapBackingFile() ? i18n("Continue") : i18n("Continue losing changes");
                const int result = KMessageBox::warningYesNoCancelList(widget(),
                                                                       warningMessage,
                                                                       listOfwontSaves,
                                                                       i18n("Warning"),
                                                                       KGuiItem(i18n("Save as Okular document archive..."), QStringLiteral("document-save-as")), // <- KMessageBox::Yes
                                                                       KGuiItem(continueMessage, QStringLiteral("arrow-right")));                                // <- KMessageBox::NO

                switch (result) {
                case KMessageBox::Yes: // -> Save as Okular document archive
                    return slotSaveFileAs(true /* showOkularArchiveAsDefaultFormat */);
                case KMessageBox::No: // -> Continue
                    setModifiedAfterSave = m_document->canSwapBackingFile();
                    break;
                case KMessageBox::Cancel:
                    return false;
                }
            }
        }

        if (m_document->canSaveChanges()) {
            // If the generator supports saving changes, save them

            QString errorText;
            if (!m_document->saveChanges(tmpFileName, &errorText)) {
                if (errorText.isEmpty()) {
                    KMessageBox::information(widget(), i18n("File could not be saved in '%1'. Try to save it to another location.", tmpFileName));
                } else {
                    KMessageBox::information(widget(), i18n("File could not be saved in '%1'. %2", tmpFileName, errorText));
                }

                return false;
            }

            copyJob = KIO::file_copy(QUrl::fromLocalFile(tmpFileName), realSaveUrl, -1, KIO::Overwrite);
            deleteTmpFileName = true;
        } else {
            // If the generators doesn't support saving changes, we will
            // just copy the original file.

            if (isDocumentArchive) {
                // Special case: if the user is extracting the contents of a
                // .okular archive back to the native format, we can't just copy
                // the open file (which is a .okular). So let's ask to core to
                // extract and give us the real file

                if (!m_document->extractArchivedFile(tmpFileName)) {
                    KMessageBox::information(widget(), i18n("File could not be saved in '%1'. Try to save it to another location.", tmpFileName));
                    return false;
                }

                copyJob = KIO::file_copy(QUrl::fromLocalFile(tmpFileName), realSaveUrl, -1, KIO::Overwrite);
                deleteTmpFileName = true;
            } else {
                // Otherwise just copy the open file.
                // make use of the already downloaded (in case of remote URLs) file,
                // no point in downloading that again
                QUrl srcUrl = QUrl::fromLocalFile(localFilePath());
                // duh, our local file disappeared...
                if (!QFile::exists(localFilePath())) {
                    if (url().isLocalFile()) {
#ifdef OKULAR_KEEP_FILE_OPEN
                        // local file: try to get it back from the open handle on it
                        tempFile.reset(m_keeper->copyToTemporary());
                        if (tempFile)
                            srcUrl = KUrl::fromPath(tempFile->fileName());
#else
                        const QString msg = i18n("Okular cannot copy %1 to the specified location.\n\nThe document does not exist anymore.", localFilePath());
                        KMessageBox::error(widget(), msg);
                        return false;
#endif
                    } else {
                        // we still have the original remote URL of the document,
                        // so copy the document from there
                        srcUrl = url();
                    }
                }

                if (srcUrl != saveUrl) {
                    copyJob = KIO::file_copy(srcUrl, realSaveUrl, -1, KIO::Overwrite);
                } else {
                    // Don't do a real copy in this case, just update the timestamps
                    copyJob = KIO::setModificationTime(realSaveUrl, QDateTime::currentDateTime());
                }
            }
        }
    }

    // Stop watching for changes while we write the new file (useful when
    // overwriting)
    if (url().isLocalFile()) {
        unsetFileToWatch();
    }

    const auto deleteTmpFileFunction = [deleteTmpFileName, tmpFileName] {
        Q_ASSERT(deleteTmpFileName == QFile::exists(tmpFileName));
        if (deleteTmpFileName) {
            QFile::remove(tmpFileName);
        }
    };

    KJobWidgets::setWindow(copyJob, widget());
    if (!copyJob->exec()) {
        KMessageBox::information(widget(), i18n("File could not be saved in '%1'. Error: '%2'. Try to save it to another location.", saveUrl.toDisplayString(), copyJob->errorString()));

        // Restore watcher
        if (url().isLocalFile()) {
            setFileToWatch(localFilePath());
        }

        deleteTmpFileFunction();

        return false;
    }
    deleteTmpFileFunction();

    m_document->setHistoryClean(true);

    if (m_document->isDocdataMigrationNeeded()) {
        m_document->docdataMigrationDone();
    }

    bool reloadedCorrectly = true;

    // Make the generator use the new file instead of the old one
    if (m_document->canSwapBackingFile() && !m_documentOpenWithPassword) {
        QWidget *currentSidebarItem = m_sidebar->currentItem();
        // this calls openFile internally, which in turn actually calls
        // m_document->swapBackingFile() instead of the regular loadDocument
        if (openUrl(saveUrl, true /* swapInsteadOfOpening */)) {
            if (setModifiedAfterSave) {
                m_document->setHistoryClean(false);
            }
        } else {
            reloadedCorrectly = false;
        }

        if (m_sidebar->currentItem() != currentSidebarItem) {
            m_sidebar->setCurrentItem(currentSidebarItem);
        }
    } else {
        // If the generator doesn't support swapping file, then just reload
        // the document from the new location
        if (!slotAttemptReload(true, saveUrl)) {
            reloadedCorrectly = false;
        }
    }

    // In case of file swapping errors, close the document to avoid inconsistencies
    if (!reloadedCorrectly) {
        qWarning() << "The document hasn't been reloaded/swapped correctly";
        closeUrl();
    }

    // Restore watcher
    if (url().isLocalFile()) {
        setFileToWatch(localFilePath());
    }

    // Set correct permission taking into account the umask value
#ifndef Q_OS_WIN
    const QString saveFilePath = saveUrl.toLocalFile();
    if (QFile::exists(saveFilePath)) {
        const mode_t mask = umask(0);
        umask(mask);
        const mode_t fileMode = 0666 & ~mask;
        chmod(QFile::encodeName(saveFilePath).constData(), fileMode);
    }
#endif

    return true;
}

// If the user wants to save in the original file's format, some features might
// not be available. Find out what cannot be saved in this format
void Part::checkNativeSaveDataLoss(bool *out_wontSaveForms, bool *out_wontSaveAnnotations) const
{
    bool wontSaveForms = false;
    bool wontSaveAnnotations = false;

    if (!m_document->canSaveChanges(Document::SaveFormsCapability)) {
        /* Set wontSaveForms only if there are forms */
        const int pagecount = m_document->pages();

        for (int pageno = 0; pageno < pagecount; ++pageno) {
            const Okular::Page *page = m_document->page(pageno);
            if (!page->formFields().empty()) {
                wontSaveForms = true;
                break;
            }
        }
    }

    if (!m_document->canSaveChanges(Document::SaveAnnotationsCapability)) {
        /* Set wontSaveAnnotations only if there are local annotations */
        const int pagecount = m_document->pages();

        for (int pageno = 0; pageno < pagecount; ++pageno) {
            const QList<Okular::Annotation *> annotations = m_document->page(pageno)->annotations();
            for (const Okular::Annotation *ann : annotations) {
                if (!(ann->flags() & Okular::Annotation::External)) {
                    wontSaveAnnotations = true;
                    break;
                }
            }
            if (wontSaveAnnotations) {
                break;
            }
        }
    }

    *out_wontSaveForms = wontSaveForms;
    *out_wontSaveAnnotations = wontSaveAnnotations;
}

void Part::slotPreferences()
{
    // Create dialog
    PreferencesDialog *dialog = new PreferencesDialog(m_pageView, Okular::Settings::self(), m_embedMode, m_document->editorCommandOverride());
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Show it
    dialog->show();
}

void Part::slotToggleChangeColors()
{
    slotSetChangeColors(!Okular::SettingsCore::changeColors());
}

void Part::slotSetChangeColors(bool active)
{
    Okular::SettingsCore::setChangeColors(active);
    Okular::Settings::self()->save();
}

void Part::slotAccessibilityPreferences()
{
    // Create dialog
    PreferencesDialog *dialog = new PreferencesDialog(m_pageView, Okular::Settings::self(), m_embedMode, m_document->editorCommandOverride());
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Show it
    dialog->switchToAccessibilityPage();
    dialog->show();
}

void Part::slotAnnotationPreferences()
{
    // Create dialog
    PreferencesDialog *dialog = new PreferencesDialog(m_pageView, Okular::Settings::self(), m_embedMode, m_document->editorCommandOverride());
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Show it
    dialog->switchToAnnotationsPage();
    dialog->show();
}

void Part::slotNewConfig()
{
    // Apply settings here. A good policy is to check whether the setting has
    // changed before applying changes.

    // Watch File
    setWatchFileModeEnabled(Okular::Settings::watchFile());

    // Main View (pageView)
    m_pageView->reparseConfig();

    // update document settings
    m_document->reparseConfig();

    // update TOC settings
    if (m_tocEnabled) {
        m_toc->reparseConfig();
    }

    // update ThumbnailList contents
    if (Okular::Settings::showLeftPanel() && !m_thumbnailList->isHidden()) {
        m_thumbnailList->updateWidgets();
    }

    // update Reviews settings
    m_reviewsWidget->reparseConfig();

    setWindowTitleFromDocument();

    if (m_presentationDrawingActions) {
        m_presentationDrawingActions->reparseConfig();
        if (factory()) {
            factory()->refreshActionProperties();
        }
    }
}

void Part::slotPrintPreview()
{
    if (m_document->pages() == 0) {
        return;
    }

    QPrinter printer;
    QString tempFilePattern;

    if (m_document->printingSupport() == Okular::Document::PostscriptPrinting) {
        tempFilePattern = (QDir::tempPath() + QLatin1String("/okular_XXXXXX.ps"));
    } else if (m_document->printingSupport() == Okular::Document::NativePrinting) {
        tempFilePattern = (QDir::tempPath() + QLatin1String("/okular_XXXXXX.pdf"));
    } else {
        return;
    }

    // Generate a temp filename for Print to File, then release the file so generator can write to it
    QTemporaryFile tf(tempFilePattern);
    tf.setAutoRemove(true);
    tf.open();
    printer.setOutputFileName(tf.fileName());
    tf.close();
    setupPrint(printer);
    doPrint(printer);
    if (QFile::exists(printer.outputFileName())) {
        Okular::FilePrinterPreview previewdlg(printer.outputFileName(), widget());
        previewdlg.exec();
    }
}

void Part::slotShowTOCMenu(const Okular::DocumentViewport &vp, const QPoint point, const QString &title)
{
    showMenu(m_document->page(vp.pageNumber), point, title, vp, true);
}

void Part::slotShowMenu(const Okular::Page *page, const QPoint point)
{
    showMenu(page, point);
}

void Part::showMenu(const Okular::Page *page, const QPoint point, const QString &bookmarkTitle, const Okular::DocumentViewport &vp, bool showTOCActions)
{
    if (m_embedMode == PrintPreviewMode) {
        return;
    }

    bool reallyShow = false;
    const bool currentPage = page && page->number() == m_document->viewport().pageNumber;

    if (!m_showMenuBarAction) {
        m_showMenuBarAction = findActionInKPartHierarchy<KToggleAction>(KStandardActionName(KStandardAction::ShowMenubar));
    }
    if (!m_showFullScreenAction) {
        m_showFullScreenAction = findActionInKPartHierarchy<KToggleFullScreenAction>(KStandardActionName(KStandardAction::FullScreen));
    }

    QMenu *popup = new QMenu(widget());
    if (showTOCActions) {
        popup->addAction(i18n("Expand whole section"), m_toc.data(), &TOC::expandRecursively);
        popup->addAction(i18n("Collapse whole section"), m_toc.data(), &TOC::collapseRecursively);
        popup->addAction(i18n("Expand all"), m_toc.data(), &TOC::expandAll);
        popup->addAction(i18n("Collapse all"), m_toc.data(), &TOC::collapseAll);
        reallyShow = true;
    }

    QAction *addBookmark = nullptr;
    QAction *removeBookmark = nullptr;
    QAction *fitPageWidth = nullptr;
    if (page) {
        popup->addAction(new OKMenuTitle(popup, i18n("Page %1", page->number() + 1)));
        if (m_thumbnailList->isVisible() && !Okular::Settings::syncThumbnailsViewport()) {
            const QIcon &syncIcon = QIcon::fromTheme(QStringLiteral("emblem-synchronizing"), QIcon::fromTheme(QStringLiteral("view-refresh")));
            popup->addAction(syncIcon, i18n("Sync Thumbnail with Page"), m_thumbnailList.data(), &ThumbnailList::syncThumbnail);
        }
        if ((!currentPage && m_document->bookmarkManager()->isBookmarked(page->number())) || (currentPage && m_document->bookmarkManager()->isBookmarked(m_document->viewport()))) {
            removeBookmark = popup->addAction(QIcon::fromTheme(QStringLiteral("bookmark-remove"), QIcon::fromTheme(QStringLiteral("edit-delete-bookmark"))), i18n("Remove Bookmark"));
        } else {
            addBookmark = popup->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add Bookmark"));
        }
        if (m_pageView->canFitPageWidth()) {
            fitPageWidth = popup->addAction(QIcon::fromTheme(QStringLiteral("zoom-fit-best")), i18n("Fit Width"));
        }
        popup->addAction(m_prevBookmark);
        popup->addAction(m_nextBookmark);
        reallyShow = true;
    }

    const int amountOfActions = popup->actions().count();
    if (m_showMenuBarAction && !m_showMenuBarAction->isChecked()) {
        if (m_hamburgerMenuAction) {
            m_hamburgerMenuAction->addToMenu(popup);
        } else if (m_showMenuBarAction) {
            popup->addAction(m_showMenuBarAction);
        }
    }
    if (m_showFullScreenAction && m_showFullScreenAction->isChecked()) {
        popup->addAction(m_showFullScreenAction);
    }
    if (popup->actions().count() > amountOfActions && popup->actions().constLast()->isVisible()) {
        popup->insertAction(popup->actions().at(amountOfActions), new OKMenuTitle(popup, i18n("Tools")));
        reallyShow = true;
    }

    if (reallyShow) {
        QAction *res = popup->exec(point);
        if (res) {
            if (res == addBookmark) {
                if (currentPage && bookmarkTitle.isEmpty()) {
                    m_document->bookmarkManager()->addBookmark(m_document->viewport());
                } else if (!bookmarkTitle.isEmpty()) {
                    m_document->bookmarkManager()->addBookmark(m_document->currentDocument(), vp, bookmarkTitle);
                } else {
                    m_document->bookmarkManager()->addBookmark(page->number());
                }
            } else if (res == removeBookmark) {
                if (currentPage) {
                    m_document->bookmarkManager()->removeBookmark(m_document->viewport());
                } else {
                    m_document->bookmarkManager()->removeBookmark(page->number());
                }
            } else if (res == fitPageWidth) {
                m_pageView->fitPageWidth(page->number());
            }
        }
    }
    delete popup;
}

template<class Action> Action *Part::findActionInKPartHierarchy(const QString &actionName)
{
    static_assert(std::is_base_of<QAction, Action>::value, "Calling this method to find something other than an Action makes no sense.");
    if (factory()) {
        const QList<KXMLGUIClient *> clients(factory()->clients());
        for (auto client : clients) {
            if (QAction *act = client->actionCollection()->action(actionName)) {
                if (Action *castedAction = qobject_cast<Action *>(act)) {
                    return castedAction;
                }
            }
        }
    }
    return nullptr;
}

KMainWindow *Part::findMainWindow()
{
    auto *potentialMainWindow = parent();
    while (potentialMainWindow) {
        if (auto *mainWindow = qobject_cast<KMainWindow *>(potentialMainWindow)) {
            return mainWindow;
        }
        potentialMainWindow = potentialMainWindow->parent();
    }
    return nullptr;
}

void Part::slotShowProperties()
{
    PropertiesDialog *d = new PropertiesDialog(widget(), m_document);
    connect(d, &QDialog::finished, d, &QObject::deleteLater);
    d->open();
}

void Part::slotShowEmbeddedFiles()
{
    EmbeddedFilesDialog *d = new EmbeddedFilesDialog(widget(), m_document);
    connect(d, &QDialog::finished, d, &QObject::deleteLater);
    d->open();
}

void Part::slotShowPresentation()
{
    if (!m_presentationWidget) {
        m_presentationWidget = new PresentationWidget(widget(), m_document, m_presentationDrawingActions, actionCollection());
    }
}

void Part::slotHidePresentation()
{
    if (m_presentationWidget) {
        delete (PresentationWidget *)m_presentationWidget;
    }
}

void Part::slotUpdateHamburgerMenu()
{
    auto ac = actionCollection();

    auto menu = m_hamburgerMenuAction->menu();
    if (!menu) {
        menu = new QMenu(widget());
        m_hamburgerMenuAction->setMenu(menu);
        if (!m_showMenuBarAction) {
            m_showMenuBarAction = findActionInKPartHierarchy<KToggleAction>(KStandardActionName(KStandardAction::ShowMenubar));
        }
        m_hamburgerMenuAction->setShowMenuBarAction(m_showMenuBarAction);
    } else {
        menu->clear();
    }

    QToolBar *visibleMainToolbar = nullptr;
    if (auto *mainWindow = findMainWindow()) {
        visibleMainToolbar = mainWindow->toolBar();
        if (!visibleMainToolbar->isVisible()) {
            visibleMainToolbar = nullptr;
        }
        const auto toolbars = mainWindow->toolBars();
        for (const auto &toolbar : toolbars) {
            m_hamburgerMenuAction->hideActionsOf(toolbar);
        }

        bool menuAvailable = false; // We already know the menu bar is hidden when this menu is opened.
        // If no menu is available, we want to add actions to the hamburger menu to show them again.
        // The hamburger menu serves as the fallback that is available through the right-click context menu.
        if (visibleMainToolbar && visibleMainToolbar->actions().contains(m_hamburgerMenuAction)) {
            menuAvailable = true;
        }
        if (!menuAvailable) {
            menu->addAction(m_showMenuBarAction);
            if (!visibleMainToolbar) {
                menu->addAction(findActionInKPartHierarchy(QStringLiteral("mainToolBar")));
            }
            menu->addSeparator();
        }
    }

    // When changing actions, keep "Simple by default, powerful when needed" in mind.
    // To retrieve an action, it is fastest to use a direct pointer if available (m_action), otherwise use
    // ac->action(actionName) and if the action isn't in the actionCollection() of this part,
    // use findActionInKPartHierarchy(actionName).
    menu->addAction(findActionInKPartHierarchy(KStandardActionName(KStandardAction::Open)));
    menu->addAction(findActionInKPartHierarchy(KStandardActionName(KStandardAction::OpenRecent)));
    menu->addAction(m_save);
    menu->addAction(m_saveAs);
    menu->addSeparator();
    menu->addAction(ac->action(QStringLiteral("mouse_drag")));
    if (!visibleMainToolbar || (visibleMainToolbar && !visibleMainToolbar->actions().contains(ac->action(QStringLiteral("mouse_selecttools"))))) {
        menu->addAction(ac->action(QStringLiteral("mouse_select")));
    }
    menu->addAction(m_copy);
    menu->addAction(m_find);
    menu->addAction(m_showLeftPanel);
    if (!visibleMainToolbar || (visibleMainToolbar && !visibleMainToolbar->actions().contains(ac->action(QStringLiteral("annotation_favorites"))))) {
        menu->addAction(ac->action(QStringLiteral("mouse_toggle_annotate")));
    }
    menu->addAction(ac->action(KStandardActionName(KStandardAction::Undo)));
    menu->addAction(ac->action(KStandardActionName(KStandardAction::Redo)));
    menu->addSeparator();

    menu->addAction(findActionInKPartHierarchy(KStandardActionName(KStandardAction::Print)));
    menu->addAction(m_printPreview);
    menu->addSeparator();
    menu->addAction(ac->action(QStringLiteral("add_digital_signature")));
    menu->addAction(m_showProperties);
    menu->addAction(m_openContainingFolder);
#if HAVE_PURPOSE
    menu->addAction(m_share);
#endif
    menu->addSeparator();

    menu->addAction(ac->action(QStringLiteral("zoom_to")));
    const QMenuBar *menuBar = m_hamburgerMenuAction->menuBar();
    if (menuBar && menuBar->actions().count() < 3) { // 3 to make sure none of the code below can crash.
        menuBar = nullptr;
    }
    auto curatedViewMenu = menu->addMenu(QIcon::fromTheme(QStringLiteral("page-2sides")), menuBar ? menuBar->actions().at(1)->text() : QStringLiteral("View"));
    if (!m_showFullScreenAction) {
        m_showFullScreenAction = findActionInKPartHierarchy<KToggleFullScreenAction>(KStandardActionName(KStandardAction::FullScreen));
    }
    curatedViewMenu->addAction(m_showFullScreenAction);
    curatedViewMenu->addAction(m_showPresentation);
    curatedViewMenu->addSeparator();
    curatedViewMenu->addAction(findActionInKPartHierarchy(QStringLiteral("view_render_mode")));
    if (auto *viewOrientationMenu = qobject_cast<QMenu *>(factory()->container(QStringLiteral("view_orientation"), this))) {
        curatedViewMenu->addAction(viewOrientationMenu->menuAction());
    }
    curatedViewMenu->addAction(findActionInKPartHierarchy(QStringLiteral("view_trim_mode")));
    curatedViewMenu->addSeparator();
    curatedViewMenu->addAction(ac->action(QStringLiteral("view_toggle_forms")));
    m_hamburgerMenuAction->hideActionsOf(curatedViewMenu);

#if HAVE_SPEECH
    auto speakMenu = menu->addMenu(QIcon::fromTheme(QStringLiteral("text-speak")), i18nc("@action:inmenu menu that contains actions to control text to speach", "Speak"));
    speakMenu->addAction(ac->action(QStringLiteral("speak_document")));
    speakMenu->addAction(ac->action(QStringLiteral("speak_current_page")));
    speakMenu->addAction(ac->action(QStringLiteral("speak_stop_all")));
    speakMenu->addAction(ac->action(QStringLiteral("speak_pause_resume")));
    m_hamburgerMenuAction->hideActionsOf(speakMenu);
#endif

    // Add the "Settings" menu from the menu bar.
    if (menuBar) {
        menu->addAction(menuBar->actions().at(menuBar->actions().count() - 3));
    }
}

void Part::slotTogglePresentation()
{
    if (m_document->isOpened()) {
        if (!m_presentationWidget) {
            m_presentationWidget = new PresentationWidget(widget(), m_document, m_presentationDrawingActions, actionCollection());
        } else {
            delete (PresentationWidget *)m_presentationWidget;
        }
    }
}

void Part::reload()
{
    if (m_document->isOpened()) {
        slotReload();
    }
}

void Part::enableStartWithPrint()
{
    m_cliPrint = true;
}

void Part::enableExitAfterPrint()
{
    m_cliPrintAndExit = true;
}

#define kKPlugin QStringLiteral("KPlugin")

void Part::slotAboutBackend()
{
    const KPluginMetaData data = m_document->generatorInfo();
    if (!data.isValid()) {
        return;
    }

    // Here we do a bit of magic because KPluginMetaData doesn't have setters
    // so we get the json info from it, modify it and use that for the KAboutPluginDialog
    // in case the internals of KPluginMetaData change it won't be too bad, at most we're
    // missing the icon or the generator extra description
    QJsonObject rawData = data.rawData();
    const QIcon icon = QIcon::fromTheme(data.iconName());

    // fall back to mime type icon
    if (icon.isNull()) {
        const Okular::DocumentInfo documentInfo = m_document->documentInfo(QSet<DocumentInfo::Key>() << DocumentInfo::MimeType);
        const QString mimeTypeName = documentInfo.get(DocumentInfo::MimeType);
        if (!mimeTypeName.isEmpty()) {
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForName(mimeTypeName);
            if (type.isValid()) {
                QJsonObject kplugin = rawData[kKPlugin].toObject();
                kplugin[QStringLiteral("Icon")] = type.iconName();
                rawData[kKPlugin] = kplugin;
            }
        }
    }

    const QString extraDescription = m_document->metaData(QStringLiteral("GeneratorExtraDescription")).toString();

    if (!extraDescription.isEmpty()) {
        const QString descriptionAndLang = QStringLiteral("Description[%1]").arg(QLocale().name());
        QJsonObject kplugin = rawData[kKPlugin].toObject();
        kplugin[descriptionAndLang] = QStringLiteral("%1\n\n%2").arg(data.description(), extraDescription);
        rawData[kKPlugin] = kplugin;
    }

    KAboutPluginDialog dlg(KPluginMetaData(rawData, data.fileName()), widget());
    dlg.exec();
}

void Part::slotExportAs(QAction *act)
{
    QList<QAction *> acts = m_exportAs->menu() ? m_exportAs->menu()->actions() : QList<QAction *>();
    int id = acts.indexOf(act);
    if ((id < 0) || (id >= acts.count())) {
        return;
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType;
    switch (id) {
    case 0:
        mimeType = mimeDatabase.mimeTypeForName(QStringLiteral("text/plain"));
        break;
    default:
        mimeType = m_exportFormats.at(id - 1).mimeType();
        break;
    }
    QString filter = i18nc("File type name and pattern", "%1 (%2)", mimeType.comment(), mimeType.globPatterns().join(QLatin1Char(' ')));

    QString fileName = QFileDialog::getSaveFileName(widget(), QString(), QString(), filter);

    if (!fileName.isEmpty()) {
        bool saved = false;
        switch (id) {
        case 0:
            saved = m_document->exportToText(fileName);
            break;
        default:
            saved = m_document->exportTo(fileName, m_exportFormats.at(id - 1));
            break;
        }
        if (!saved) {
            KMessageBox::information(widget(), i18n("File could not be saved in '%1'. Try to save it to another location.", fileName));
        }
    }
}

void Part::slotReload()
{
    // stop the dirty handler timer, otherwise we may conflict with the
    // auto-refresh system
    m_dirtyHandler->stop();

    slotAttemptReload();
}

void Part::slotPrint()
{
    if (m_document->pages() == 0) {
        return;
    }

#ifdef Q_OS_WIN
    QPrinter printer(QPrinter::HighResolution);
#else
    QPrinter printer;
#endif
    QWidget *printConfigWidget = nullptr;

    // Must do certain QPrinter setup before creating QPrintDialog
    setupPrint(printer);

    // Create the Print Dialog with extra config widgets if required
    if (m_document->canConfigurePrinter()) {
        printConfigWidget = m_document->printConfigurationWidget();
    } else {
        printConfigWidget = new DefaultPrintOptionsWidget();
    }

    QPrintDialog printDialog(&printer, widget());
    printDialog.setWindowTitle(i18nc("@title:window", "Print"));
    QList<QWidget *> options;
    if (printConfigWidget) {
        options << printConfigWidget;
    }
    printDialog.setOptionTabs(options);

    // Set the available Print Range
    printDialog.setMinMax(1, m_document->pages());
    printDialog.setFromTo(1, m_document->pages());

    // If the user has bookmarked pages for printing, then enable Selection
    if (!m_document->bookmarkedPageRange().isEmpty()) {
        printDialog.addEnabledOption(QAbstractPrintDialog::PrintSelection);
    }

    // If the Document type doesn't support print to both PS & PDF then disable the Print Dialog option
    if (printDialog.isOptionEnabled(QAbstractPrintDialog::PrintToFile) && !m_document->supportsPrintToFile()) {
        printDialog.setEnabledOptions(printDialog.enabledOptions() ^ QAbstractPrintDialog::PrintToFile);
    }

    // Enable the Current Page option in the dialog.
    if (m_document->pages() > 1 && currentPage() > 0) {
        printDialog.setOption(QAbstractPrintDialog::PrintCurrentPage);
    }

    bool success = true;
    if (printDialog.exec()) {
        // set option for margins if widget is of corresponding type that holds this information
        PrintOptionsWidget *optionWidget = dynamic_cast<PrintOptionsWidget *>(printConfigWidget);
        if (optionWidget != nullptr) {
            printer.setFullPage(optionWidget->ignorePrintMargins());
        } else {
            // printConfigurationWidget() method should always return an object of type Okular::PrintOptionsWidget,
            // (signature does not (yet) require it for ABI stability reasons), so Q_EMIT a warning if the object is of another type
            qWarning() << "printConfigurationWidget() method did not return an Okular::PrintOptionsWidget. This is strongly discouraged!";
        }

        success = doPrint(printer);
    }

    if (m_cliPrintAndExit) {
        exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
    }
}

void Part::setupPrint(QPrinter &printer)
{
    printer.setOrientation(m_document->orientation());

    // title
    QString title = m_document->metaData(QStringLiteral("DocumentTitle")).toString();
    if (title.isEmpty()) {
        title = m_document->currentDocument().fileName();
    }
    if (!title.isEmpty()) {
        printer.setDocName(title);
    }
}

bool Part::doPrint(QPrinter &printer)
{
    if (!m_document->isAllowed(Okular::AllowPrint)) {
        KMessageBox::error(widget(), i18n("Printing this document is not allowed."));
        return false;
    }

    const Document::PrintError printError = m_document->print(printer);
    if (printError != Document::NoPrintError) {
        const QString error = Okular::Document::printErrorString(printError);
        if (error.isEmpty()) {
            KMessageBox::error(widget(), i18n("Could not print the document. Unknown error. Please report to bugs.kde.org"));
        } else {
            KMessageBox::error(widget(), i18n("Could not print the document. Detailed error is \"%1\". Please report to bugs.kde.org", error));
        }
        return false;
    }
    return true;
}

void Part::psTransformEnded(int exit, QProcess::ExitStatus status)
{
    Q_UNUSED(exit)
    if (status != QProcess::NormalExit) {
        return;
    }

    QProcess *senderobj = sender() ? qobject_cast<QProcess *>(sender()) : nullptr;
    if (senderobj) {
        senderobj->close();
        senderobj->deleteLater();
    }

    setLocalFilePath(m_temporaryLocalFile);
    openUrl(QUrl::fromLocalFile(m_temporaryLocalFile));
    m_temporaryLocalFile.clear();
}

void Part::displayInfoMessage(const QString &message, KMessageWidget::MessageType messageType, int duration)
{
    if (!Okular::Settings::showOSD()) {
        if (messageType == KMessageWidget::Error) {
            KMessageBox::error(widget(), message);
        }
        return;
    }

    // hide messageWindow if string is empty
    if (message.isEmpty()) {
        m_infoMessage->animatedHide();
    }

    // display message (duration is length dependent)
    if (duration < 0) {
        duration = 500 + 100 * message.length();
    }
    m_infoTimer->start(duration);
    m_infoMessage->setText(message);
    m_infoMessage->setMessageType(messageType);
    m_infoMessage->setVisible(true);
}

void Part::errorMessage(const QString &message, int duration)
{
    displayInfoMessage(message, KMessageWidget::Error, duration);
}

void Part::warningMessage(const QString &message, int duration)
{
    displayInfoMessage(message, KMessageWidget::Warning, duration);
}

void Part::noticeMessage(const QString &message, int duration)
{
    // less important message -> simpler display widget in the PageView
    m_pageView->displayMessage(message, QString(), PageViewMessage::Info, duration);
}

void Part::moveSplitter(int sideWidgetSize)
{
    m_sidebar->moveSplitter(sideWidgetSize);
}

void Part::unsetDummyMode()
{
    if (m_embedMode == PrintPreviewMode) {
        return;
    }

    m_sidebar->setSidebarVisibility(Okular::Settings::showLeftPanel());

    // add back and next in history
    m_historyBack = KStandardAction::documentBack(this, SLOT(slotHistoryBack()), actionCollection());
    m_historyBack->setWhatsThis(i18n("Go to the place you were before"));
    connect(m_pageView.data(), &PageView::mouseBackButtonClick, m_historyBack, &QAction::trigger);

    m_historyNext = KStandardAction::documentForward(this, SLOT(slotHistoryNext()), actionCollection());
    m_historyNext->setWhatsThis(i18n("Go to the place you were after"));
    connect(m_pageView.data(), &PageView::mouseForwardButtonClick, m_historyNext, &QAction::trigger);

    m_pageView->setupActions(actionCollection());

    // attach the actions of the children widgets too
    m_formsMessage->addAction(m_pageView->toggleFormsAction());

    m_signatureMessage->addAction(m_showSignaturePanel);

    // ensure history actions are in the correct state
    updateViewActions();
}

bool Part::handleCompressed(QString &destpath, const QString &path, KFilterDev::CompressionType compressionType)
{
    m_tempfile = nullptr;

    // we are working with a compressed file, decompressing
    // temporary file for decompressing
    QTemporaryFile *newtempfile = new QTemporaryFile();
    newtempfile->setAutoRemove(true);

    if (!newtempfile->open()) {
        KMessageBox::error(widget(),
                           i18n("<qt><strong>File Error!</strong> Could not create temporary file "
                                "<nobr><strong>%1</strong></nobr>.</qt>",
                                newtempfile->errorString()));
        delete newtempfile;
        return false;
    }

    // decompression filer
    KCompressionDevice dev(path, compressionType);

    if (!dev.open(QIODevice::ReadOnly)) {
        KMessageBox::detailedError(widget(),
                                   i18n("<qt><strong>File Error!</strong> Could not open the file "
                                        "<nobr><strong>%1</strong></nobr> for uncompression. "
                                        "The file will not be loaded.</qt>",
                                        path),
                                   i18n("<qt>This error typically occurs if you do "
                                        "not have enough permissions to read the file. "
                                        "You can check ownership and permissions if you "
                                        "right-click on the file in the Dolphin "
                                        "file manager, then choose the 'Properties' option, "
                                        "and select 'Permissions' tab in the opened window.</qt>"));

        delete newtempfile;
        return false;
    }

    char buf[65536];
    int read = 0, wrtn = 0;

    while ((read = dev.read(buf, sizeof(buf))) > 0) {
        wrtn = newtempfile->write(buf, read);
        if (read != wrtn) {
            break;
        }
    }
    if ((read != 0) || (newtempfile->size() == 0)) {
        KMessageBox::detailedError(widget(),
                                   i18n("<qt><strong>File Error!</strong> Could not uncompress "
                                        "the file <nobr><strong>%1</strong></nobr>. "
                                        "The file will not be loaded.</qt>",
                                        path),
                                   i18n("<qt>This error typically occurs if the file is corrupt. "
                                        "If you want to be sure, try to decompress the file manually "
                                        "using command-line tools.</qt>"));
        delete newtempfile;
        return false;
    }
    m_tempfile = newtempfile;
    destpath = m_tempfile->fileName();
    return true;
}

void Part::rebuildBookmarkMenu(bool unplugActions)
{
    if (unplugActions) {
        unplugActionList(QStringLiteral("bookmarks_currentdocument"));
        qDeleteAll(m_bookmarkActions);
        m_bookmarkActions.clear();
    }
    QUrl u = m_document->currentDocument();
    if (u.isValid()) {
        m_bookmarkActions = m_document->bookmarkManager()->actionsForUrl(u);
    }
    bool havebookmarks = true;
    if (m_bookmarkActions.isEmpty()) {
        havebookmarks = false;
        QAction *a = new QAction(nullptr);
        a->setText(i18n("No Bookmarks"));
        a->setEnabled(false);
        m_bookmarkActions.append(a);
    }
    plugActionList(QStringLiteral("bookmarks_currentdocument"), m_bookmarkActions);

    if (factory()) {
        const QList<KXMLGUIClient *> clients(factory()->clients());
        bool containerFound = false;
        for (int i = 0; !containerFound && i < clients.size(); ++i) {
            QMenu *container = dynamic_cast<QMenu *>(factory()->container(QStringLiteral("bookmarks"), clients[i]));
            if (container && container->actions().contains(m_bookmarkActions.first())) {
                container->installEventFilter(this);
                containerFound = true;
            }
        }
    }

    m_prevBookmark->setEnabled(havebookmarks);
    m_nextBookmark->setEnabled(havebookmarks);
}

bool Part::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ContextMenu: {
        QContextMenuEvent *e = static_cast<QContextMenuEvent *>(event);
        QMenu *menu = static_cast<QMenu *>(watched);

        QScopedPointer<QMenu> ctxMenu(new QMenu);

        QPoint pos;
        bool ret = false;
        if (e->reason() == QContextMenuEvent::Mouse) {
            pos = e->pos();
            ret = aboutToShowContextMenu(menu, menu->actionAt(e->pos()), ctxMenu.data());
        } else if (menu->activeAction()) {
            pos = menu->actionGeometry(menu->activeAction()).center();
            ret = aboutToShowContextMenu(menu, menu->activeAction(), ctxMenu.data());
        }
        ctxMenu->exec(menu->mapToGlobal(pos));

        if (ret) {
            event->accept();
        }
        return ret;
    }

    default:
        break;
    }

    return KParts::ReadWritePart::eventFilter(watched, event);
}

void Part::updateAboutBackendAction()
{
    const KPluginMetaData data = m_document->generatorInfo();
    m_aboutBackend->setEnabled(data.isValid());
}

void Part::resetStartArguments()
{
    m_cliPrint = false;
    m_cliPrintAndExit = false;
}

#if HAVE_PURPOSE
void Part::slotShareActionFinished(const QJsonObject &output, int error, const QString &message)
{
    if (error) {
        KMessageBox::error(widget(), i18n("There was a problem sharing the document: %1", message), i18n("Share"));
    } else {
        const QString url = output[QStringLiteral("url")].toString();
        if (url.isEmpty()) {
            m_pageView->displayMessage(i18n("Document shared successfully"));
        } else {
            KMessageBox::information(widget(), i18n("You can find the shared document at: <a href=\"%1\">%1</a>", url), i18n("Share"), QString(), KMessageBox::Notify | KMessageBox::AllowLink);
        }
    }
}
#endif

void Part::setReadWrite(bool readwrite)
{
    m_document->setAnnotationEditingEnabled(readwrite);
    ReadWritePart::setReadWrite(readwrite);
}

void Part::enableStartWithFind(const QString &text)
{
    m_textToFindOnOpen = QString(text);
}

void Part::setEditorCmd(const QString &editorCmd)
{
    m_document->setEditorCommandOverride(editorCmd);
}

void Part::slotOpenContainingFolder()
{
    KIO::highlightInFileManager({QUrl(localFilePath())});
}

} // namespace Okular

#include "part.moc"

/* kate: replace-tabs on; indent-width 4; */
