/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2002 by Chris Cheney <ccheney@cheney.cx>                *
 *   Copyright (C) 2003 by Benjamin Meyer <benjamin@csh.rit.edu>           *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003-2004 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2003 by Luboš Luňák <l.lunak@kde.org>                   *
 *   Copyright (C) 2003 by Malcolm Hunter <malcolm.hunter@gmx.co.uk>       *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "shell.h"

// qt/kde includes
#include <QDesktopWidget>
#include <QTimer>
#include <QDBusConnection>
#include <QMenuBar>
#include <QApplication>
#include <QFileDialog>
#include <KPluginLoader>
#include <KMessageBox>
#include <QMimeType>
#include <QMimeDatabase>
#include <KStandardAction>
#include <KToolBar>
#include <KRecentFilesAction>
#include <KServiceTypeTrader>
#include <KToggleFullScreenAction>
#include <KActionCollection>
#include <KWindowSystem>
#include <QTabWidget>
#include <KXMLGUIFactory>
#include <QDragMoveEvent>
#include <QTabBar>
#include <KConfigGroup>
#include <KUrlMimeData>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KIO/Global>
#ifndef Q_OS_WIN
#include <KActivities/ResourceInstance>
#endif

// local includes
#include "kdocumentviewer.h"
#include "../interfaces/viewerinterface.h"
#include "shellutils.h"

static const char *shouldShowMenuBarComingFromFullScreen = "shouldShowMenuBarComingFromFullScreen";
static const char *shouldShowToolBarComingFromFullScreen = "shouldShowToolBarComingFromFullScreen";

static const char* const SESSION_URL_KEY = "Urls";
static const char* const SESSION_TAB_KEY = "ActiveTab";

Shell::Shell( const QString &serializedOptions )
  : KParts::MainWindow(), m_menuBarWasShown(true), m_toolBarWasShown(true)
#ifndef Q_OS_WIN
    , m_activityResource(nullptr)
#endif
    , m_isValid(true)
{
  setObjectName( QStringLiteral( "okular::Shell#" ) );
  setContextMenuPolicy( Qt::NoContextMenu );
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
  if (!m_partFactory)
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    m_isValid = false;
    KMessageBox::error(this, i18n("Unable to find the Okular component: %1", loader.errorString()));
    return;
  }

  // now that the Part plugin is loaded, create the part
  KParts::ReadWritePart* const firstPart = m_partFactory->create< KParts::ReadWritePart >( this );
  if (firstPart)
  {
    // Setup tab bar
    m_tabWidget = new QTabWidget( this );
    m_tabWidget->setTabsClosable( true );
    m_tabWidget->setElideMode( Qt::ElideRight );
    m_tabWidget->tabBar()->hide();
    m_tabWidget->setDocumentMode( true );
    m_tabWidget->setMovable( true );

    m_tabWidget->setAcceptDrops(true);
    m_tabWidget->installEventFilter(this);

    connect( m_tabWidget, &QTabWidget::currentChanged, this, &Shell::setActiveTab );
    connect( m_tabWidget, &QTabWidget::tabCloseRequested, this, &Shell::closeTab );
    connect( m_tabWidget->tabBar(), &QTabBar::tabMoved, this, &Shell::moveTabData );

    setCentralWidget( m_tabWidget );

    // then, setup our actions
    setupActions();
    connect( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater );
    // and integrate the part's GUI with the shell's
    setupGUI(Keys | ToolBar | Save);
    createGUI(firstPart);
    connectPart( firstPart );

    m_tabs.append( firstPart );
    m_tabWidget->addTab( firstPart->widget(), QString() );

    readSettings();

    m_unique = ShellUtils::unique(serializedOptions);
    if (m_unique)
    {
        m_unique = QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.okular"));
        if (!m_unique)
            KMessageBox::information(this, i18n("There is already a unique Okular instance running. This instance won't be the unique one."));
    }
    else
    {
        QString serviceName = QStringLiteral("org.kde.okular-") + QString::number(qApp->applicationPid());
        QDBusConnection::sessionBus().registerService(serviceName);
    }
    if (ShellUtils::noRaise(serializedOptions))
    {
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/okularshell"), this, QDBusConnection::ExportScriptableSlots);
  }
  else
  {
    m_isValid = false;
    KMessageBox::error(this, i18n("Unable to find the Okular component."));
  }
}

bool Shell::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    QDragMoveEvent* dmEvent = dynamic_cast<QDragMoveEvent*>(event);
    if (dmEvent) {
        bool accept = dmEvent->mimeData()->hasUrls();
        event->setAccepted(accept);
        return accept;
    }

    QDropEvent* dEvent = dynamic_cast<QDropEvent*>(event);
    if (dEvent) {
        const QList<QUrl> list = KUrlMimeData::urlsFromMimeData(dEvent->mimeData());
        handleDroppedUrls(list);
        dEvent->setAccepted(true);
        return true;
    }
    return false;
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
    if( !m_tabs.empty() )
    {
        writeSettings();
        for( QList<TabState>::iterator it = m_tabs.begin(); it != m_tabs.end(); ++it )
        {
           it->part->closeUrl( false );
        }
        m_tabs.clear();
    }
    if (m_unique)
        QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.kde.okular"));

    delete m_tabWidget;
}

// Open a new document if we have space for it
// This can hang if called on a unique instance and openUrl pops a messageBox
bool Shell::openDocument( const QUrl& url, const QString &serializedOptions )
{
    if( m_tabs.size() <= 0 )
       return false;

    KParts::ReadWritePart* const part = m_tabs[0].part;

    // Return false if we can't open new tabs and the only part is occupied
    if ( !dynamic_cast<Okular::ViewerInterface*>(part)->openNewFilesInTabs()
         && !part->url().isEmpty()
         && !ShellUtils::unique(serializedOptions))
    {
        return false;
    }

    openUrl( url, serializedOptions );

    return true;
}

bool Shell::openDocument( const QString& urlString, const QString &serializedOptions )
{
    return openDocument(QUrl(urlString), serializedOptions);
}

bool Shell::canOpenDocs( int numDocs, int desktop )
{
   if( m_tabs.size() <= 0 || numDocs <= 0 || m_unique )
      return false;

   KParts::ReadWritePart* const part = m_tabs[0].part;
   const bool allowTabs = dynamic_cast<Okular::ViewerInterface*>(part)->openNewFilesInTabs();

   if( !allowTabs && (numDocs > 1 || !part->url().isEmpty()) )
      return false;

   const KWindowInfo winfo( window()->effectiveWinId(), KWindowSystem::WMDesktop );
   if( winfo.desktop() != desktop )
      return false;

   return true;
}

void Shell::openUrl( const QUrl & url, const QString &serializedOptions )
{
    const int activeTab = m_tabWidget->currentIndex();
    if ( activeTab < m_tabs.size() )
    {
        KParts::ReadWritePart* const activePart = m_tabs[activeTab].part;
        if( !activePart->url().isEmpty() )
        {
            if( m_unique )
            {
                applyOptionsToPart( activePart, serializedOptions );
                activePart->openUrl( url );
            }
            else
            {
                if( dynamic_cast<Okular::ViewerInterface *>(activePart)->openNewFilesInTabs() )
                {
                    openNewTab( url, serializedOptions );
                }
                else
                {
                    Shell* newShell = new Shell( serializedOptions );
                    newShell->show();
                    newShell->openUrl( url, serializedOptions );
                }
            }
        }
        else
        {
            m_tabWidget->setTabText( activeTab, url.fileName() );
            applyOptionsToPart( activePart, serializedOptions );
            bool openOk = activePart->openUrl( url );
            const bool isstdin = url.fileName() == QLatin1String( "-" );
            if ( !isstdin )
            {
                if ( openOk )
                {
#ifndef Q_OS_WIN
                    if ( !m_activityResource )
                        m_activityResource = new KActivities::ResourceInstance( window()->winId(), this );

                    m_activityResource->setUri( url );
#endif
                    m_recent->addUrl( url );
                }
                else
                    m_recent->removeUrl( url );
            }
        }
    }
}

void Shell::closeUrl()
{
    closeTab( m_tabWidget->currentIndex() );
}

void Shell::readSettings()
{
    m_recent->loadEntries( KSharedConfig::openConfig()->group( "Recent Files" ) );
    m_recent->setEnabled( true ); // force enabling

    const KConfigGroup group = KSharedConfig::openConfig()->group( "Desktop Entry" );
    bool fullScreen = group.readEntry( "FullScreen", false );
    setFullScreen( fullScreen );

    if (fullScreen)
    {
        m_menuBarWasShown = group.readEntry( shouldShowMenuBarComingFromFullScreen, true );
        m_toolBarWasShown = group.readEntry( shouldShowToolBarComingFromFullScreen, true );
    }
}

void Shell::writeSettings()
{
    m_recent->saveEntries( KSharedConfig::openConfig()->group( "Recent Files" ) );
    KConfigGroup group = KSharedConfig::openConfig()->group( "Desktop Entry" );
    group.writeEntry( "FullScreen", m_fullScreenAction->isChecked() );
    if (m_fullScreenAction->isChecked())
    {
        group.writeEntry( shouldShowMenuBarComingFromFullScreen, m_menuBarWasShown );
        group.writeEntry( shouldShowToolBarComingFromFullScreen, m_toolBarWasShown );
    }
    KSharedConfig::openConfig()->sync();
}

void Shell::setupActions()
{
  KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
  m_recent = KStandardAction::openRecent( this, SLOT(openUrl(QUrl)), actionCollection() );
  m_recent->setToolBarMode( KRecentFilesAction::MenuMode );
  connect( m_recent, &QAction::triggered, this, &Shell::showOpenRecentMenu );
  m_recent->setToolTip( i18n("Click to open a file\nClick and hold to open a recent file") );
  m_recent->setWhatsThis( i18n( "<b>Click</b> to open a file or <b>Click and hold</b> to select a recent file" ) );
  m_printAction = KStandardAction::print( this, SLOT(print()), actionCollection() );
  m_printAction->setEnabled( false );
  m_closeAction = KStandardAction::close( this, SLOT(closeUrl()), actionCollection() );
  m_closeAction->setEnabled( false );
  KStandardAction::quit(this, SLOT(close()), actionCollection());

  setStandardToolBarMenuEnabled(true);

  m_showMenuBarAction = KStandardAction::showMenubar( this, SLOT(slotShowMenubar()), actionCollection());
  m_fullScreenAction = KStandardAction::fullScreen( this, SLOT(slotUpdateFullScreen()), this,actionCollection() );

  m_nextTabAction = actionCollection()->addAction(QStringLiteral("tab-next"));
  m_nextTabAction->setText( i18n("Next Tab") );
  actionCollection()->setDefaultShortcuts(m_nextTabAction, KStandardShortcut::tabNext());
  m_nextTabAction->setEnabled( false );
  connect( m_nextTabAction, &QAction::triggered, this, &Shell::activateNextTab );

  m_prevTabAction = actionCollection()->addAction(QStringLiteral("tab-previous"));
  m_prevTabAction->setText( i18n("Previous Tab") );
  actionCollection()->setDefaultShortcuts(m_prevTabAction, KStandardShortcut::tabPrev());
  m_prevTabAction->setEnabled( false );
  connect( m_prevTabAction, &QAction::triggered, this, &Shell::activatePrevTab );
}

void Shell::saveProperties(KConfigGroup &group)
{
    if ( !m_isValid ) // part couldn't be loaded, nothing to save
        return;

    // Gather lists of settings to preserve
    QStringList urls;
    for( int i = 0; i < m_tabs.size(); ++i )
    {
        urls.append( m_tabs[i].part->url().url() );
    }
    group.writePathEntry( SESSION_URL_KEY, urls );
    group.writeEntry( SESSION_TAB_KEY, m_tabWidget->currentIndex() );
}

void Shell::readProperties(const KConfigGroup &group)
{
    // Reopen documents based on saved settings
    QStringList urls = group.readPathEntry( SESSION_URL_KEY, QStringList() );

    while( !urls.isEmpty() )
    {
        openUrl( QUrl(urls.takeFirst()) );
    }

    int desiredTab = group.readEntry<int>( SESSION_TAB_KEY, 0 );
    if( desiredTab < m_tabs.size() )
    {
        setActiveTab( desiredTab );
    }
}

QStringList Shell::fileFormats() const
{
    QStringList supportedPatterns;

    QString constraint( QStringLiteral("(Library == 'okularpart')") );
    QLatin1String basePartService( "KParts/ReadOnlyPart" );
    KService::List offers = KServiceTypeTrader::self()->query( basePartService, constraint );
    KService::List::ConstIterator it = offers.constBegin(), itEnd = offers.constEnd();
    for ( ; it != itEnd; ++it )
    {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->mimeTypes();

        supportedPatterns += mimeTypes;
    }

    return supportedPatterns;
}

void Shell::fileOpen()
{
	// this slot is called whenever the File->Open menu is selected,
	// the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
	// button is clicked
    const int activeTab = m_tabWidget->currentIndex();
    if ( !m_fileformatsscanned )
    {
        const KDocumentViewer* const doc = qobject_cast<KDocumentViewer*>(m_tabs[activeTab].part);
        if ( doc )
            m_fileformats = doc->supportedMimeTypes();

        if ( m_fileformats.isEmpty() )
            m_fileformats = fileFormats();

        m_fileformatsscanned = true;
    }

    QUrl startDir;
    const KParts::ReadWritePart* const curPart = m_tabs[activeTab].part;
    if ( curPart->url().isLocalFile() )
        startDir = KIO::upUrl(curPart->url());

    QPointer<QFileDialog> dlg( new QFileDialog( this ));
    dlg->setDirectoryUrl( startDir );
    dlg->setAcceptMode( QFileDialog::AcceptOpen );
    dlg->setOption( QFileDialog::HideNameFilterDetails, true );

    QMimeDatabase mimeDatabase;
    QSet<QString> globPatterns;
    QMap<QString, QStringList> namedGlobs;
    foreach ( const QString &mimeName, m_fileformats ) {
        QMimeType mimeType = mimeDatabase.mimeTypeForName( mimeName );
        const QStringList globs( mimeType.globPatterns() );
        if ( globs.isEmpty() ) {
            continue;
        }

        globPatterns.unite( globs.toSet() ) ;

        namedGlobs[ mimeType.comment() ].append( globs );

    }
    QStringList namePatterns;
    foreach( const QString &name, namedGlobs.keys()) {
        namePatterns.append( name +
                             QStringLiteral(" (") +
                             namedGlobs[name].join( QLatin1Char(' ') ) +
                             QStringLiteral(")")
                           );
    }

    namePatterns.prepend( i18n("All files (*)") );
    namePatterns.prepend( i18n("All supported files (%1)", globPatterns.toList().join( QLatin1Char(' ') ) ) );
    dlg->setNameFilters( namePatterns );

    dlg->setWindowTitle( i18n("Open Document") );
    if ( dlg->exec() && dlg ) {
        foreach(const QUrl& url, dlg->selectedUrls())
        {
            openUrl( url );
        }
    }

    if ( dlg ) {
        delete dlg.data();
    }
}

void Shell::tryRaise()
{
    KWindowSystem::forceActiveWindow( window()->effectiveWinId() );
}

// only called when starting the program
void Shell::setFullScreen( bool useFullScreen )
{
    if( useFullScreen )
        setWindowState( windowState() | Qt::WindowFullScreen ); // set
    else
        setWindowState( windowState() & ~Qt::WindowFullScreen ); // reset
}

void Shell::setCaption( const QString &caption )
{
    bool modified = false;

    const int activeTab = m_tabWidget->currentIndex();
    if ( activeTab >= 0 && activeTab < m_tabs.size() )
    {
        KParts::ReadWritePart* const activePart = m_tabs[activeTab].part;
        QString tabCaption = activePart->url().fileName();
        if ( activePart->isModified() ) {
            modified = true;
            if ( !tabCaption.isEmpty() ) {
                tabCaption.append( QStringLiteral( " *" ) );
            }
        }

        m_tabWidget->setTabText( activeTab, tabCaption );
    }

    setCaption( caption, modified );
}

void Shell::showEvent(QShowEvent *e)
{
    if (!menuBar()->isNativeMenuBar() && m_showMenuBarAction)
        m_showMenuBarAction->setChecked( menuBar()->isVisible() );

    KParts::MainWindow::showEvent(e);
}

void Shell::slotUpdateFullScreen()
{
    if(m_fullScreenAction->isChecked())
    {
      m_menuBarWasShown = !menuBar()->isHidden();
      menuBar()->hide();

      m_toolBarWasShown = !toolBar()->isHidden();
      toolBar()->hide();

      KToggleFullScreenAction::setFullScreen(this, true);
    }
    else
    {
      if (m_menuBarWasShown)
      {
        menuBar()->show();
      }
      if (m_toolBarWasShown)
      {
        toolBar()->show();
      }
      KToggleFullScreenAction::setFullScreen(this, false);
    }
}

void Shell::slotShowMenubar()
{
    if ( menuBar()->isHidden() )
        menuBar()->show();
    else
        menuBar()->hide();
}

QSize Shell::sizeHint() const
{
    return QApplication::desktop()->availableGeometry( this ).size() * 0.75;
}

bool Shell::queryClose()
{
    if (m_tabs.count() > 1)
    {
        const QString dontAskAgainName = "ShowTabWarning";
        KMessageBox::ButtonCode dummy;
        if (shouldBeShownYesNo(dontAskAgainName, dummy))
        {
            QDialog *dialog = new QDialog(this);
            dialog->setWindowTitle(i18n("Confirm Close"));

            QDialogButtonBox *buttonBox = new QDialogButtonBox(dialog);
            buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), KGuiItem(i18n("Close Tabs"), "tab-close"));
            KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), KStandardGuiItem::cancel());


            bool checkboxResult = true;
            const int result = KMessageBox::createKMessageBox(dialog, buttonBox, QMessageBox::Question,
                                                i18n("You are about to close %1 tabs. Are you sure you want to continue?", m_tabs.count()), QStringList(),
                                                i18n("Warn me when I attempt to close multiple tabs"),
                                                &checkboxResult, KMessageBox::Notify);

            if (!checkboxResult)
            {
                saveDontShowAgainYesNo(dontAskAgainName, dummy);
            }

            if (result != QDialogButtonBox::Yes)
            {
                return false;
            }
        }
    }

    for( int i = 0; i < m_tabs.size(); ++i )
    {
        KParts::ReadWritePart* const part = m_tabs[i].part;

        // To resolve confusion about multiple modified docs, switch to relevant tab
        if( part->isModified() )
            setActiveTab( i );

        if( !part->queryClose() )
           return false;
    }
    return true;
}

void Shell::setActiveTab( int tab )
{
    m_tabWidget->setCurrentIndex( tab );
    createGUI( m_tabs[tab].part );
    m_printAction->setEnabled( m_tabs[tab].printEnabled );
    m_closeAction->setEnabled( m_tabs[tab].closeEnabled );
}

void Shell::closeTab( int tab )
{
    KParts::ReadWritePart* const part = m_tabs[tab].part;
    if( part->closeUrl() && m_tabs.count() > 1 )
    {
        if( part->factory() )
            part->factory()->removeClient( part );
        part->disconnect();
        part->deleteLater();
        m_tabs.removeAt( tab );
        m_tabWidget->removeTab( tab );

        if( m_tabWidget->count() == 1 )
        {
            m_tabWidget->tabBar()->hide();
            m_nextTabAction->setEnabled( false );
            m_prevTabAction->setEnabled( false );
        }
    }

}

void Shell::openNewTab( const QUrl& url, const QString &serializedOptions )
{
    // Tabs are hidden when there's only one, so show it
    if( m_tabs.size() == 1 )
    {
        m_tabWidget->tabBar()->show();
        m_nextTabAction->setEnabled( true );
        m_prevTabAction->setEnabled( true );
    }

    const int newIndex = m_tabs.size();

    // Make new part
    m_tabs.append( m_partFactory->create<KParts::ReadWritePart>(this) );
    connectPart( m_tabs[newIndex].part );

    // Update GUI
    KParts::ReadWritePart* const part = m_tabs[newIndex].part;
    m_tabWidget->addTab( part->widget(), url.fileName() );

    applyOptionsToPart(part, serializedOptions);

    int previousActiveTab = m_tabWidget->currentIndex();
    setActiveTab( m_tabs.size() - 1 );

    if( part->openUrl(url) )
        m_recent->addUrl( url );
    else
        setActiveTab( previousActiveTab );
}

void Shell::applyOptionsToPart( QObject* part, const QString &serializedOptions )
{
    KDocumentViewer* const doc = qobject_cast<KDocumentViewer*>(part);
    if ( ShellUtils::startInPresentation(serializedOptions) )
        doc->startPresentation();
    if ( ShellUtils::showPrintDialog(serializedOptions) )
        QMetaObject::invokeMethod( part, "enableStartWithPrint" );
}

void Shell::connectPart( QObject* part )
{
    connect( this, SIGNAL(moveSplitter(int)), part, SLOT(moveSplitter(int)) );
    connect( part, SIGNAL(enablePrintAction(bool)), this, SLOT(setPrintEnabled(bool)));
    connect( part, SIGNAL(enableCloseAction(bool)), this, SLOT(setCloseEnabled(bool)));
    connect( part, SIGNAL(mimeTypeChanged(QMimeType)), this, SLOT(setTabIcon(QMimeType)));
    connect( part, SIGNAL(urlsDropped(QList<QUrl>)), this, SLOT(handleDroppedUrls(QList<QUrl>)) );
    connect( part, SIGNAL(fitWindowToPage(QSize,QSize)), this, SLOT(slotFitWindowToPage(QSize,QSize)) );
}

void Shell::print()
{
    QMetaObject::invokeMethod( m_tabs[m_tabWidget->currentIndex()].part, "slotPrint" );
}

void Shell::setPrintEnabled( bool enabled )
{
    int i = findTabIndex( sender() );
    if( i != -1 )
    {
        m_tabs[i].printEnabled = enabled;
        if( i == m_tabWidget->currentIndex() )
            m_printAction->setEnabled( enabled );
    }
}

void Shell::setCloseEnabled( bool enabled )
{
    int i = findTabIndex( sender() );
    if( i != -1 )
    {
        m_tabs[i].closeEnabled = enabled;
        if( i == m_tabWidget->currentIndex() )
            m_closeAction->setEnabled( enabled );
    }
}

void Shell::activateNextTab()
{
    if( m_tabs.size() < 2 )
        return;

    const int activeTab = m_tabWidget->currentIndex();
    const int nextTab = (activeTab == m_tabs.size()-1) ? 0 : activeTab+1;

    setActiveTab( nextTab );
}

void Shell::activatePrevTab()
{
    if( m_tabs.size() < 2 )
        return;

    const int activeTab = m_tabWidget->currentIndex();
    const int prevTab = (activeTab == 0) ? m_tabs.size()-1 : activeTab-1;

    setActiveTab( prevTab );
}

void Shell::setTabIcon( const QMimeType& mimeType )
{
    int i = findTabIndex( sender() );
    if( i != -1 )
    {
        m_tabWidget->setTabIcon( i, QIcon::fromTheme(mimeType.iconName()) );
    }
}

int Shell::findTabIndex( QObject* sender )
{
    for( int i = 0; i < m_tabs.size(); ++i )
    {
        if( m_tabs[i].part == sender )
        {
            return i;
        }
    }
    return -1;
}

void Shell::handleDroppedUrls( const QList<QUrl>& urls )
{
    foreach( const QUrl& url, urls )
    {
        openUrl( url );
    }
}

void Shell::moveTabData( int from, int to )
{
   m_tabs.move( from, to );
}

void Shell::slotFitWindowToPage(const QSize& pageViewSize, const QSize& pageSize )
{
    const int xOffset = pageViewSize.width() - pageSize.width();
    const int yOffset = pageViewSize.height() - pageSize.height();
    showNormal();
    resize( width() - xOffset, height() - yOffset);
    moveSplitter(pageSize.width());
}

/* kate: replace-tabs on; indent-width 4; */
