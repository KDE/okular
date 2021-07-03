/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2002 Chris Cheney <ccheney@cheney.cx>
    SPDX-FileCopyrightText: 2003 Benjamin Meyer <benjamin@csh.rit.edu>
    SPDX-FileCopyrightText: 2003-2004 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003-2004 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2003 Luboš Luňák <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2003 Malcolm Hunter <malcolm.hunter@gmx.co.uk>
    SPDX-FileCopyrightText: 2004 Dominique Devriese <devriese@kde.org>
    SPDX-FileCopyrightText: 2004 Dirk Mueller <mueller@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "shell.h"

// qt/kde includes
#include <KActionCollection>
#include <KConfigGroup>
#include <KIO/Global>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KRecentFilesAction>
#include <KSharedConfig>
#include <KStandardAction>
#include <KToggleFullScreenAction>
#include <KToolBar>
#include <KUrlMimeData>
#include <KWindowSystem>
#include <KXMLGUIFactory>
#include <QApplication>
#include <QDBusConnection>
#include <QDragMoveEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QObject>
#include <QScreen>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#ifdef WITH_KACTIVITIES
#include <KActivities/ResourceInstance>
#endif

#include <kio_version.h>
#include <kxmlgui_version.h>

// local includes
#include "../interfaces/viewerinterface.h"
#include "kdocumentviewer.h"
#include "shellutils.h"

static const char *shouldShowMenuBarComingFromFullScreen = "shouldShowMenuBarComingFromFullScreen";
static const char *shouldShowToolBarComingFromFullScreen = "shouldShowToolBarComingFromFullScreen";

static const char *const SESSION_URL_KEY = "Urls";
static const char *const SESSION_TAB_KEY = "ActiveTab";

Shell::Shell(const QString &serializedOptions)
    : KParts::MainWindow()
    , m_menuBarWasShown(true)
    , m_toolBarWasShown(true)
#ifndef Q_OS_WIN
    , m_activityResource(nullptr)
#endif
    , m_isValid(true)
{
    setObjectName(QStringLiteral("okular::Shell#"));
    setContextMenuPolicy(Qt::NoContextMenu);
    // otherwise .rc file won't be found by unit test
    setComponentName(QStringLiteral("okular"), QString());
    // set the shell's ui resource file
    setXMLFile(QStringLiteral("shell.rc"));
    m_fileformatsscanned = false;
    m_showMenuBarAction = nullptr;
    // this routine will find and load our Part.  it finds the Part by
    // name which is a bad idea usually.. but it's alright in this
    // case since our Part is made for this Shell
    KPluginLoader loader(QStringLiteral("okularpart"));
    m_partFactory = loader.factory();
    if (!m_partFactory) {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        m_isValid = false;
        KMessageBox::error(this, i18n("Unable to find the Okular component: %1", loader.errorString()));
        return;
    }

    // now that the Part plugin is loaded, create the part
    KParts::ReadWritePart *const firstPart = m_partFactory->create<KParts::ReadWritePart>(this);
    if (firstPart) {
        // Setup tab bar
        m_tabWidget = new QTabWidget(this);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setElideMode(Qt::ElideRight);
        m_tabWidget->tabBar()->hide();
        m_tabWidget->setDocumentMode(true);
        m_tabWidget->setMovable(true);

        m_tabWidget->setAcceptDrops(true);
        m_tabWidget->tabBar()->installEventFilter(this);

        connect(m_tabWidget, &QTabWidget::currentChanged, this, &Shell::setActiveTab);
        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &Shell::closeTab);
        connect(m_tabWidget->tabBar(), &QTabBar::tabMoved, this, &Shell::moveTabData);

        setCentralWidget(m_tabWidget);

        // then, setup our actions
        setupActions();
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
        // and integrate the part's GUI with the shell's
        setupGUI(Keys | ToolBar | Save);

        m_tabs.append(firstPart);
        m_tabWidget->addTab(firstPart->widget(), QString()); // triggers setActiveTab that calls createGUI( part )

        connectPart(firstPart);

        readSettings();

        m_unique = ShellUtils::unique(serializedOptions);
        if (m_unique) {
            m_unique = QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.okular"));
            if (!m_unique)
                KMessageBox::information(this, i18n("There is already a unique Okular instance running. This instance won't be the unique one."));
        } else {
            QString serviceName = QStringLiteral("org.kde.okular-") + QString::number(qApp->applicationPid());
            QDBusConnection::sessionBus().registerService(serviceName);
        }
        if (ShellUtils::noRaise(serializedOptions)) {
            setAttribute(Qt::WA_ShowWithoutActivating);
        }

        QDBusConnection::sessionBus().registerObject(QStringLiteral("/okularshell"), this, QDBusConnection::ExportScriptableSlots);
    } else {
        m_isValid = false;
        KMessageBox::error(this, i18n("Unable to find the Okular component."));
    }

#if KXMLGUI_VERSION > QT_VERSION_CHECK(5, 78, 0)
    connect(guiFactory(), &KXMLGUIFactory::shortcutsSaved, this, &Shell::reloadAllXML);
#endif
}

void Shell::reloadAllXML()
{
    for (const TabState &tab : qAsConst(m_tabs)) {
        tab.part->reloadXML();
    }
}

void Shell::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape && window()->isFullScreen()) {
        setFullScreen(false);
    }
}

bool Shell::eventFilter(QObject *obj, QEvent *event)
{
    QDragMoveEvent *dmEvent = dynamic_cast<QDragMoveEvent *>(event);
    if (dmEvent) {
        bool accept = dmEvent->mimeData()->hasUrls();
        event->setAccepted(accept);
        return accept;
    }

    QDropEvent *dEvent = dynamic_cast<QDropEvent *>(event);
    if (dEvent) {
        const QList<QUrl> list = KUrlMimeData::urlsFromMimeData(dEvent->mimeData());
        handleDroppedUrls(list);
        dEvent->setAccepted(true);
        return true;
    }

    // Handle middle button click events on the tab bar
    if (obj == m_tabWidget->tabBar() && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
        if (mEvent->button() == Qt::MiddleButton) {
            int tabIndex = m_tabWidget->tabBar()->tabAt(mEvent->pos());
            if (tabIndex != -1) {
                closeTab(tabIndex);
                return true;
            }
        }
    }
    return KParts::MainWindow::eventFilter(obj, event);
}

bool Shell::isValid() const
{
    return m_isValid;
}

void Shell::showOpenRecentMenu()
{
    m_recent->menu()->popup(QCursor::pos());
}

Shell::~Shell()
{
    if (!m_tabs.empty()) {
        writeSettings();
        for (const TabState &tab : qAsConst(m_tabs)) {
            tab.part->closeUrl(false);
        }
        m_tabs.clear();
    }
    if (m_unique)
        QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.kde.okular"));

    delete m_tabWidget;
}

// Open a new document if we have space for it
// This can hang if called on a unique instance and openUrl pops a messageBox
bool Shell::openDocument(const QUrl &url, const QString &serializedOptions)
{
    if (m_tabs.size() <= 0)
        return false;

    KParts::ReadWritePart *const part = m_tabs[0].part;

    // Return false if we can't open new tabs and the only part is occupied
    if (!qobject_cast<Okular::ViewerInterface *>(part)->openNewFilesInTabs() && !part->url().isEmpty() && !ShellUtils::unique(serializedOptions)) {
        return false;
    }

    openUrl(url, serializedOptions);

    return true;
}

bool Shell::openDocument(const QString &urlString, const QString &serializedOptions)
{
    return openDocument(QUrl(urlString), serializedOptions);
}

bool Shell::canOpenDocs(int numDocs, int desktop)
{
    if (m_tabs.size() <= 0 || numDocs <= 0 || m_unique)
        return false;

    KParts::ReadWritePart *const part = m_tabs[0].part;
    const bool allowTabs = qobject_cast<Okular::ViewerInterface *>(part)->openNewFilesInTabs();

    if (!allowTabs && (numDocs > 1 || !part->url().isEmpty()))
        return false;

    const KWindowInfo winfo(window()->effectiveWinId(), KWindowSystem::WMDesktop);
    if (winfo.desktop() != desktop)
        return false;

    return true;
}

void Shell::openUrl(const QUrl &url, const QString &serializedOptions)
{
    const int activeTab = m_tabWidget->currentIndex();
    if (activeTab < m_tabs.size()) {
        KParts::ReadWritePart *const activePart = m_tabs[activeTab].part;
        if (!activePart->url().isEmpty()) {
            if (m_unique) {
                applyOptionsToPart(activePart, serializedOptions);
                activePart->openUrl(url);
            } else {
                if (qobject_cast<Okular::ViewerInterface *>(activePart)->openNewFilesInTabs()) {
                    openNewTab(url, serializedOptions);
                } else {
                    Shell *newShell = new Shell(serializedOptions);
                    newShell->show();
                    newShell->openUrl(url, serializedOptions);
                }
            }
        } else {
            m_tabWidget->setTabText(activeTab, url.fileName());
            applyOptionsToPart(activePart, serializedOptions);
            bool openOk = activePart->openUrl(url);
            const bool isstdin = url.fileName() == QLatin1String("-") || url.scheme() == QLatin1String("fd");
            if (!isstdin) {
                if (openOk) {
#ifdef WITH_KACTIVITIES
                    if (!m_activityResource)
                        m_activityResource = new KActivities::ResourceInstance(window()->winId(), this);

                    m_activityResource->setUri(url);
#endif
                    m_recent->addUrl(url);
                } else {
                    m_recent->removeUrl(url);
                    closeTab(activeTab);
                }
            }
        }
    }
}

void Shell::closeUrl()
{
    closeTab(m_tabWidget->currentIndex());

    // When closing the current tab two things can happen:
    //  * the focus was on the tab
    //  * the focus was somewhere in the toolbar
    // we don't have other places that accept focus
    //  * If it was on the tab, logic says it should go back to the next current tab
    //  * If it was on the toolbar, we could leave it there, but since we redo the menus/toobars for the new tab, it gets kind of lost
    //    so it's easier to set it to the next current tab which also makes sense as consistency
    if (m_tabWidget->count() >= 0) {
        KParts::ReadWritePart *const newPart = m_tabs[m_tabWidget->currentIndex()].part;
        newPart->widget()->setFocus();
    }
}

void Shell::readSettings()
{
    m_recent->loadEntries(KSharedConfig::openConfig()->group("Recent Files"));
    m_recent->setEnabled(true); // force enabling

    const KConfigGroup group = KSharedConfig::openConfig()->group("Desktop Entry");
    bool fullScreen = group.readEntry("FullScreen", false);
    setFullScreen(fullScreen);

    if (fullScreen) {
        m_menuBarWasShown = group.readEntry(shouldShowMenuBarComingFromFullScreen, true);
        m_toolBarWasShown = group.readEntry(shouldShowToolBarComingFromFullScreen, true);
    }
}

void Shell::writeSettings()
{
    m_recent->saveEntries(KSharedConfig::openConfig()->group("Recent Files"));
    KConfigGroup group = KSharedConfig::openConfig()->group("Desktop Entry");
    group.writeEntry("FullScreen", m_fullScreenAction->isChecked());
    if (m_fullScreenAction->isChecked()) {
        group.writeEntry(shouldShowMenuBarComingFromFullScreen, m_menuBarWasShown);
        group.writeEntry(shouldShowToolBarComingFromFullScreen, m_toolBarWasShown);
    }
    KSharedConfig::openConfig()->sync();
}

void Shell::setupActions()
{
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    m_recent = KStandardAction::openRecent(this, SLOT(openUrl(QUrl)), actionCollection());
    m_recent->setToolBarMode(KRecentFilesAction::MenuMode);
    connect(m_recent, &QAction::triggered, this, &Shell::showOpenRecentMenu);
    m_recent->setToolTip(i18n("Click to open a file\nClick and hold to open a recent file"));
    m_recent->setWhatsThis(i18n("<b>Click</b> to open a file or <b>Click and hold</b> to select a recent file"));
    m_printAction = KStandardAction::print(this, SLOT(print()), actionCollection());
    m_printAction->setEnabled(false);
    m_closeAction = KStandardAction::close(this, SLOT(closeUrl()), actionCollection());
    m_closeAction->setEnabled(false);
    KStandardAction::quit(this, SLOT(close()), actionCollection());

    setStandardToolBarMenuEnabled(true);

    m_showMenuBarAction = KStandardAction::showMenubar(this, SLOT(slotShowMenubar()), actionCollection());
    m_fullScreenAction = KStandardAction::fullScreen(this, SLOT(slotUpdateFullScreen()), this, actionCollection());

    m_nextTabAction = actionCollection()->addAction(QStringLiteral("tab-next"));
    m_nextTabAction->setText(i18n("Next Tab"));
    actionCollection()->setDefaultShortcuts(m_nextTabAction, KStandardShortcut::tabNext());
    m_nextTabAction->setEnabled(false);
    connect(m_nextTabAction, &QAction::triggered, this, &Shell::activateNextTab);

    m_prevTabAction = actionCollection()->addAction(QStringLiteral("tab-previous"));
    m_prevTabAction->setText(i18n("Previous Tab"));
    actionCollection()->setDefaultShortcuts(m_prevTabAction, KStandardShortcut::tabPrev());
    m_prevTabAction->setEnabled(false);
    connect(m_prevTabAction, &QAction::triggered, this, &Shell::activatePrevTab);

    m_undoCloseTab = actionCollection()->addAction(QStringLiteral("undo-close-tab"));
    m_undoCloseTab->setText(i18n("Undo close tab"));
    actionCollection()->setDefaultShortcut(m_undoCloseTab, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    m_undoCloseTab->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    m_undoCloseTab->setEnabled(false);
    connect(m_undoCloseTab, &QAction::triggered, this, &Shell::undoCloseTab);
}

void Shell::saveProperties(KConfigGroup &group)
{
    if (!m_isValid) // part couldn't be loaded, nothing to save
        return;

    // Gather lists of settings to preserve
    QStringList urls;
    for (const TabState &tab : qAsConst(m_tabs)) {
        urls.append(tab.part->url().url());
    }
    group.writePathEntry(SESSION_URL_KEY, urls);
    group.writeEntry(SESSION_TAB_KEY, m_tabWidget->currentIndex());
}

void Shell::readProperties(const KConfigGroup &group)
{
    // Reopen documents based on saved settings
    QStringList urls = group.readPathEntry(SESSION_URL_KEY, QStringList());
    int desiredTab = group.readEntry<int>(SESSION_TAB_KEY, 0);

    while (!urls.isEmpty()) {
        openUrl(QUrl(urls.takeFirst()));
    }

    if (desiredTab < m_tabs.size()) {
        setActiveTab(desiredTab);
    }
}

void Shell::fileOpen()
{
    // this slot is called whenever the File->Open menu is selected,
    // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
    // button is clicked
    const int activeTab = m_tabWidget->currentIndex();
    if (!m_fileformatsscanned) {
        const KDocumentViewer *const doc = qobject_cast<KDocumentViewer *>(m_tabs[activeTab].part);
        Q_ASSERT(doc);

        m_fileformats = doc->supportedMimeTypes();

        m_fileformatsscanned = true;
    }

    QUrl startDir;
    const KParts::ReadWritePart *const curPart = m_tabs[activeTab].part;
    if (curPart->url().isLocalFile())
        startDir = KIO::upUrl(curPart->url());

    QPointer<QFileDialog> dlg(new QFileDialog(this));
    dlg->setDirectoryUrl(startDir);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setOption(QFileDialog::HideNameFilterDetails, true);
    dlg->setFileMode(QFileDialog::ExistingFiles); // Allow selection of more than one file

    QMimeDatabase mimeDatabase;
    // Unfortunately non Plasma file dialogs don't support the "All supported files" when using
    // setMimeTypeFilters instead of setNameFilters, so for those use setNameFilters which is a bit
    // worse because doesn't show you pdf files named bla.blo when you say "show me the pdf files", but
    // that's solvable by choosing "All Files" and it's not that common while it's more convenient to
    // only get shown the files that the application can open by default instead of all of them
#if KIO_VERSION >= QT_VERSION_CHECK(5, 73, 0)
    const bool useMimeTypeFilters = qgetenv("XDG_CURRENT_DESKTOP").toLower() == QStringLiteral("kde");
#else
    const bool useMimeTypeFilters = false;
#endif
    if (useMimeTypeFilters) {
        QStringList mimetypes;
        for (const QString &mimeName : qAsConst(m_fileformats)) {
            QMimeType mimeType = mimeDatabase.mimeTypeForName(mimeName);
            mimetypes << mimeType.name();
        }
        mimetypes.prepend(QStringLiteral("application/octet-stream"));
        dlg->setMimeTypeFilters(mimetypes);
    } else {
        QSet<QString> globPatterns;
        QMap<QString, QStringList> namedGlobs;
        for (const QString &mimeName : qAsConst(m_fileformats)) {
            QMimeType mimeType = mimeDatabase.mimeTypeForName(mimeName);
            const QStringList globs(mimeType.globPatterns());
            if (globs.isEmpty()) {
                continue;
            }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            globPatterns.unite(QSet<QString>(globs.begin(), globs.end()));
#else
            globPatterns.unite(globs.toSet());
#endif

            namedGlobs[mimeType.comment()].append(globs);
        }
        QStringList namePatterns;
        for (auto it = namedGlobs.cbegin(); it != namedGlobs.cend(); ++it) {
            namePatterns.append(it.key() + QLatin1String(" (") + it.value().join(QLatin1Char(' ')) + QLatin1Char(')'));
        }

        const QStringList allGlobPatterns = globPatterns.values();
        namePatterns.prepend(i18n("All files (*)"));
        namePatterns.prepend(i18n("All supported files (%1)", allGlobPatterns.join(QLatin1Char(' '))));
        dlg->setNameFilters(namePatterns);
    }

    dlg->setWindowTitle(i18n("Open Document"));
    if (dlg->exec() && dlg) {
        const QList<QUrl> urlList = dlg->selectedUrls();
        for (const QUrl &url : urlList) {
            openUrl(url);
        }
    }

    if (dlg) {
        delete dlg.data();
    }
}

void Shell::tryRaise()
{
    KWindowSystem::forceActiveWindow(window()->effectiveWinId());
}

// only called when starting the program
void Shell::setFullScreen(bool useFullScreen)
{
    if (useFullScreen)
        setWindowState(windowState() | Qt::WindowFullScreen); // set
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen); // reset
}

void Shell::setCaption(const QString &caption)
{
    bool modified = false;

    const int activeTab = m_tabWidget->currentIndex();
    if (activeTab >= 0 && activeTab < m_tabs.size()) {
        KParts::ReadWritePart *const activePart = m_tabs[activeTab].part;
        QString tabCaption = activePart->url().fileName();
        if (activePart->isModified()) {
            modified = true;
            if (!tabCaption.isEmpty()) {
                tabCaption.append(QStringLiteral(" *"));
            }
        }

        m_tabWidget->setTabText(activeTab, tabCaption);
    }

    setCaption(caption, modified);
}

void Shell::showEvent(QShowEvent *e)
{
    if (!menuBar()->isNativeMenuBar() && m_showMenuBarAction)
        m_showMenuBarAction->setChecked(menuBar()->isVisible());

    KParts::MainWindow::showEvent(e);
}

void Shell::slotUpdateFullScreen()
{
    if (m_fullScreenAction->isChecked()) {
        m_menuBarWasShown = !menuBar()->isHidden();
        menuBar()->hide();

        m_toolBarWasShown = !toolBar()->isHidden();
        toolBar()->hide();

        KToggleFullScreenAction::setFullScreen(this, true);
    } else {
        if (m_menuBarWasShown) {
            menuBar()->show();
        }
        if (m_toolBarWasShown) {
            toolBar()->show();
        }
        KToggleFullScreenAction::setFullScreen(this, false);
    }
}

void Shell::slotShowMenubar()
{
    if (menuBar()->isHidden())
        menuBar()->show();
    else
        menuBar()->hide();
}

QSize Shell::sizeHint() const
{
    const QSize baseSize = QApplication::primaryScreen()->availableSize() * 0.6;
    // Set an arbitrary yet sensible sane minimum size for very small screens;
    // for example we don't want people using 1366x768 screens to get a tiny
    // default window size of 820 x 460 which will elide most of the toolbar buttons.
    return baseSize.expandedTo(QSize(1000, 700));
}

bool Shell::queryClose()
{
    if (m_tabs.count() > 1) {
        const QString dontAskAgainName = QStringLiteral("ShowTabWarning");
        KMessageBox::ButtonCode dummy = KMessageBox::Yes;
        if (shouldBeShownYesNo(dontAskAgainName, dummy)) {
            QDialog *dialog = new QDialog(this);
            dialog->setWindowTitle(i18n("Confirm Close"));

            QDialogButtonBox *buttonBox = new QDialogButtonBox(dialog);
            buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), KGuiItem(i18n("Close Tabs"), QStringLiteral("tab-close")));
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), KStandardGuiItem::cancel());

            bool checkboxResult = true;
            const int result = KMessageBox::createKMessageBox(dialog,
                                                              buttonBox,
                                                              QMessageBox::Question,
                                                              i18n("You are about to close %1 tabs. Are you sure you want to continue?", m_tabs.count()),
                                                              QStringList(),
                                                              i18n("Warn me when I attempt to close multiple tabs"),
                                                              &checkboxResult,
                                                              KMessageBox::Notify);

            if (!checkboxResult) {
                saveDontShowAgainYesNo(dontAskAgainName, dummy);
            }

            if (result != QDialogButtonBox::Yes) {
                return false;
            }
        }
    }

    for (int i = 0; i < m_tabs.size(); ++i) {
        KParts::ReadWritePart *const part = m_tabs[i].part;

        // To resolve confusion about multiple modified docs, switch to relevant tab
        if (part->isModified())
            setActiveTab(i);

        if (!part->queryClose())
            return false;
    }
    return true;
}

void Shell::setActiveTab(int tab)
{
    m_tabWidget->setCurrentIndex(tab);
#if KXMLGUI_VERSION <= QT_VERSION_CHECK(5, 78, 0)
    m_tabs[tab].part->reloadXML();
#endif
    createGUI(m_tabs[tab].part);

    m_printAction->setEnabled(m_tabs[tab].printEnabled);
    m_closeAction->setEnabled(m_tabs[tab].closeEnabled);
}

void Shell::closeTab(int tab)
{
    KParts::ReadWritePart *const part = m_tabs[tab].part;
    QUrl url = part->url();
    if (part->closeUrl() && m_tabs.count() > 1) {
        if (part->factory())
            part->factory()->removeClient(part);
        part->disconnect();
        part->deleteLater();
        m_tabs.removeAt(tab);
        m_tabWidget->removeTab(tab);
        m_undoCloseTab->setEnabled(true);
        m_closedTabUrls.append(url);

        if (m_tabWidget->count() == 1) {
            m_tabWidget->tabBar()->hide();
            m_nextTabAction->setEnabled(false);
            m_prevTabAction->setEnabled(false);
        }
    }
}

void Shell::openNewTab(const QUrl &url, const QString &serializedOptions)
{
    const int previousActiveTab = m_tabWidget->currentIndex();
    KParts::ReadWritePart *const activePart = m_tabs[previousActiveTab].part;

    bool activateTabIfAlreadyOpen;
    QMetaObject::invokeMethod(activePart, "activateTabIfAlreadyOpenFile", Q_RETURN_ARG(bool, activateTabIfAlreadyOpen));

    if (activateTabIfAlreadyOpen) {
        const int tabIndex = findTabIndex(url);

        if (tabIndex >= 0) {
            setActiveTab(tabIndex);
            m_recent->addUrl(url);
            return;
        }
    }

    // Tabs are hidden when there's only one, so show it
    if (m_tabs.size() == 1) {
        m_tabWidget->tabBar()->show();
        m_nextTabAction->setEnabled(true);
        m_prevTabAction->setEnabled(true);
    }

    const int newIndex = m_tabs.size();

    // Make new part
    m_tabs.append(m_partFactory->create<KParts::ReadWritePart>(this));
    connectPart(m_tabs[newIndex].part);

    // Update GUI
    KParts::ReadWritePart *const part = m_tabs[newIndex].part;
    m_tabWidget->addTab(part->widget(), url.fileName());

    applyOptionsToPart(part, serializedOptions);

    setActiveTab(m_tabs.size() - 1);

    if (part->openUrl(url)) {
        m_recent->addUrl(url);
    } else {
        setActiveTab(previousActiveTab);
        closeTab(m_tabs.size() - 1);
        m_recent->removeUrl(url);
    }
}

void Shell::applyOptionsToPart(QObject *part, const QString &serializedOptions)
{
    KDocumentViewer *const doc = qobject_cast<KDocumentViewer *>(part);
    const QString find = ShellUtils::find(serializedOptions);
    if (ShellUtils::startInPresentation(serializedOptions))
        doc->startPresentation();
    if (ShellUtils::showPrintDialog(serializedOptions))
        QMetaObject::invokeMethod(part, "enableStartWithPrint");
    if (ShellUtils::showPrintDialogAndExit(serializedOptions))
        QMetaObject::invokeMethod(part, "enableExitAfterPrint");
    if (!find.isEmpty())
        QMetaObject::invokeMethod(part, "enableStartWithFind", Q_ARG(QString, find));
}

void Shell::connectPart(QObject *part)
{
    // We're abusing the fact we know the part is our part here
    connect(this, SIGNAL(moveSplitter(int)), part, SLOT(moveSplitter(int)));                     // clazy:exclude=old-style-connect
    connect(part, SIGNAL(enablePrintAction(bool)), this, SLOT(setPrintEnabled(bool)));           // clazy:exclude=old-style-connect
    connect(part, SIGNAL(enableCloseAction(bool)), this, SLOT(setCloseEnabled(bool)));           // clazy:exclude=old-style-connect
    connect(part, SIGNAL(mimeTypeChanged(QMimeType)), this, SLOT(setTabIcon(QMimeType)));        // clazy:exclude=old-style-connect
    connect(part, SIGNAL(urlsDropped(QList<QUrl>)), this, SLOT(handleDroppedUrls(QList<QUrl>))); // clazy:exclude=old-style-connect
    // clang-format off
    // Otherwise the QSize,QSize gets turned into QSize, QSize that is not normalized signals and is slightly slower
    connect(part, SIGNAL(fitWindowToPage(QSize,QSize)), this, SLOT(slotFitWindowToPage(QSize,QSize)));   // clazy:exclude=old-style-connect
    // clang-format on
}

void Shell::print()
{
    QMetaObject::invokeMethod(m_tabs[m_tabWidget->currentIndex()].part, "slotPrint");
}

void Shell::setPrintEnabled(bool enabled)
{
    int i = findTabIndex(sender());
    if (i != -1) {
        m_tabs[i].printEnabled = enabled;
        if (i == m_tabWidget->currentIndex())
            m_printAction->setEnabled(enabled);
    }
}

void Shell::setCloseEnabled(bool enabled)
{
    int i = findTabIndex(sender());
    if (i != -1) {
        m_tabs[i].closeEnabled = enabled;
        if (i == m_tabWidget->currentIndex())
            m_closeAction->setEnabled(enabled);
    }
}

void Shell::activateNextTab()
{
    if (m_tabs.size() < 2)
        return;

    const int activeTab = m_tabWidget->currentIndex();
    const int nextTab = (activeTab == m_tabs.size() - 1) ? 0 : activeTab + 1;

    setActiveTab(nextTab);
}

void Shell::activatePrevTab()
{
    if (m_tabs.size() < 2)
        return;

    const int activeTab = m_tabWidget->currentIndex();
    const int prevTab = (activeTab == 0) ? m_tabs.size() - 1 : activeTab - 1;

    setActiveTab(prevTab);
}

void Shell::undoCloseTab()
{
    if (m_closedTabUrls.isEmpty()) {
        return;
    }

    const QUrl lastTabUrl = m_closedTabUrls.takeLast();

    if (m_closedTabUrls.isEmpty()) {
        m_undoCloseTab->setEnabled(false);
    }

    openUrl(lastTabUrl);
}

void Shell::setTabIcon(const QMimeType &mimeType)
{
    int i = findTabIndex(sender());
    if (i != -1) {
        m_tabWidget->setTabIcon(i, QIcon::fromTheme(mimeType.iconName()));
    }
}

int Shell::findTabIndex(QObject *sender) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].part == sender) {
            return i;
        }
    }
    return -1;
}

int Shell::findTabIndex(const QUrl &url) const
{
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [&url](const TabState state) { return state.part->url() == url; });
    return (it != m_tabs.end()) ? std::distance(m_tabs.begin(), it) : -1;
}

void Shell::handleDroppedUrls(const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        openUrl(url);
    }
}

void Shell::moveTabData(int from, int to)
{
    m_tabs.move(from, to);
}

void Shell::slotFitWindowToPage(const QSize pageViewSize, const QSize pageSize)
{
    const int xOffset = pageViewSize.width() - pageSize.width();
    const int yOffset = pageViewSize.height() - pageSize.height();
    showNormal();
    resize(width() - xOffset, height() - yOffset);
    emit moveSplitter(pageSize.width());
}

/* kate: replace-tabs on; indent-width 4; */
