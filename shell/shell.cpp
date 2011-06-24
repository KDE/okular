/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2002 by Chris Cheney <ccheney@cheney.cx>                *
 *   Copyright (C) 2003 by Benjamin Meyer <benjamin@csh.rit.edu>           *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003-2004 by Albert Astals Cid <tsdgeos@terra.es>       *
 *   Copyright (C) 2003 by Luboš Luňák <l.lunak@kde.org>                   *
 *   Copyright (C) 2003 by Malcolm Hunter <malcolm.hunter@gmx.co.uk>       *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Dirk Mueller <mueller@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "shell.h"

// qt/kde includes
#include <qdesktopwidget.h>
#include <qtimer.h>
#include <QtDBus/qdbusconnection.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kfiledialog.h>
#include <kpluginloader.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandardaction.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kio/netaccess.h>
#include <krecentfilesaction.h>
#include <kservicetypetrader.h>
#include <ktoggleaction.h>
#include <ktogglefullscreenaction.h>
#include <kactioncollection.h>

// local includes
#include "kdocumentviewer.h"
#include "shellutils.h"

#include <iostream>
using namespace std;

static const char *shouldShowMenuBarComingFromFullScreen = "shouldShowMenuBarComingFromFullScreen";
static const char *shouldShowToolBarComingFromFullScreen = "shouldShowToolBarComingFromFullScreen";

Shell::Shell(KCmdLineArgs* args, int argIndex)
  : KParts::MainWindow(), m_args(args), m_menuBarWasShown(true), m_toolBarWasShown(true)
{
  if (m_args && argIndex != -1)
  {
    m_openUrl = ShellUtils::urlFromArg(m_args->arg(argIndex),
        ShellUtils::qfileExistFunc(), m_args->getOption("page"));
  }
  init();
}

void Shell::init()
{
  setObjectName( QLatin1String( "okular::Shell" ) );
  setContextMenuPolicy( Qt::NoContextMenu );
  // set the shell's ui resource file
  setXMLFile("shell.rc");
  m_doc=0L;
  m_fileformatsscanned = false;
  m_showMenuBarAction = 0;
  // this routine will find and load our Part.  it finds the Part by
  // name which is a bad idea usually.. but it's alright in this
  // case since our Part is made for this Shell
  KPluginFactory *factory = KPluginLoader("okularpart").factory();
  if (!factory)
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, i18n("Unable to find the Okular component."));
    m_part = 0;
    return;
  }

  // now that the Part is loaded, we cast it to a Part to get
  // our hands on it
//  cout << "creating a part from factory ... " << endl;
  m_part = factory->create< KParts::ReadOnlyPart >( this );
//  cout << "creation of part from factory finished ... " << endl;
  if (m_part)
  {
    // then, setup our actions
    setupActions();
    // tell the KParts::MainWindow that this is indeed the main widget
    setCentralWidget(m_part->widget());
    // and integrate the part's GUI with the shell's
    setupGUI(Keys | ToolBar | Save);
    createGUI(m_part);
    m_doc = qobject_cast<KDocumentViewer*>(m_part);
  }

  connect( this, SIGNAL( restoreDocument(const KConfigGroup&) ),m_part, SLOT( restoreDocument(const KConfigGroup&)));
  connect( this, SIGNAL( saveDocumentRestoreInfo(KConfigGroup&) ), m_part, SLOT( saveDocumentRestoreInfo(KConfigGroup&)));
  connect( m_part, SIGNAL( enablePrintAction(bool) ), m_printAction, SLOT( setEnabled(bool)));

  readSettings();

  if (m_args && m_args->isSet("unique") && m_args->count() == 1)
  {
    QDBusConnection::sessionBus().registerService("org.kde.okular");
  }

  if (m_openUrl.isValid()) QTimer::singleShot(0, this, SLOT(delayedOpen()));
}

void Shell::delayedOpen()
{
   openUrl( m_openUrl );
}

Shell::~Shell()
{
    if ( m_part ) writeSettings();
    delete m_part;
    if ( m_args )
        m_args->clear();
}

void Shell::openUrl( const KUrl & url )
{
//    cout << "Shell::openUrl ??? " << endl;
    if ( m_part )
    {
        if ( m_doc && m_args && m_args->isSet( "presentation" ) ){
//            cout << "Shell:openUrl: startPresentaion() ??? " << endl;
            m_doc->startPresentation();
        }
        bool openOk = m_part->openUrl( url );

//        cout << "Shell::openUrl(): file opened as openOk is true :):)" << endl;

        const bool isstdin = url.fileName( KUrl::ObeyTrailingSlash ) == QLatin1String( "-" );
        if ( !isstdin )
        {
            if ( openOk )
                m_recent->addUrl( url );
            else
                m_recent->removeUrl( url );
        }
    }
}


void Shell::readSettings()
{
    m_recent->loadEntries( KGlobal::config()->group( "Recent Files" ) );
    m_recent->setEnabled( true ); // force enabling

    const KConfigGroup group = KGlobal::config()->group( "Desktop Entry" );
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
    m_recent->saveEntries( KGlobal::config()->group( "Recent Files" ) );
    KConfigGroup group = KGlobal::config()->group( "Desktop Entry" );
    group.writeEntry( "FullScreen", m_fullScreenAction->isChecked() );
    if (m_fullScreenAction->isChecked())
    {
        group.writeEntry( shouldShowMenuBarComingFromFullScreen, m_menuBarWasShown );
        group.writeEntry( shouldShowToolBarComingFromFullScreen, m_toolBarWasShown );
    }
    KGlobal::config()->sync();
}

void Shell::setupActions()
{
  KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
  m_recent = KStandardAction::openRecent( this, SLOT( openUrl( const KUrl& ) ), actionCollection() );
  m_recent->setToolBarMode( KRecentFilesAction::MenuMode );
  m_recent->setToolButtonPopupMode( QToolButton::DelayedPopup );
  connect( m_recent, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
  m_recent->setToolTip( i18n("Click to open a file\nClick and hold to open a recent file") );
  m_recent->setWhatsThis( i18n( "<b>Click</b> to open a file or <b>Click and hold</b> to select a recent file" ) );
  m_printAction = KStandardAction::print( m_part, SLOT( slotPrint() ), actionCollection() );
  m_printAction->setEnabled( false );
  KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());

  setStandardToolBarMenuEnabled(true);

  m_showMenuBarAction = KStandardAction::showMenubar( this, SLOT( slotShowMenubar() ), actionCollection());
  m_fullScreenAction = KStandardAction::fullScreen( this, SLOT( slotUpdateFullScreen() ), this,actionCollection() );
}

void Shell::saveProperties(KConfigGroup &group)
{
  // the 'config' object points to the session managed
  // config file.  anything you write here will be available
  // later when this app is restored
    emit saveDocumentRestoreInfo(group);
}

void Shell::readProperties(const KConfigGroup &group)
{
  // the 'config' object points to the session managed
  // config file.  this function is automatically called whenever
  // the app is being restored.  read in here whatever you wrote
  // in 'saveProperties'
  if(m_part)
  {
    emit restoreDocument(group);
  }
}

QStringList Shell::fileFormats() const
{
    QStringList supportedPatterns;

    QString constraint( "(Library == 'okularpart')" );
    QLatin1String basePartService( "KParts/ReadOnlyPart" );
    KService::List offers = KServiceTypeTrader::self()->query( basePartService, constraint );
    KService::List::ConstIterator it = offers.constBegin(), itEnd = offers.constEnd();
    for ( ; it != itEnd; ++it )
    {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();
        foreach ( const QString& mimeType, mimeTypes )
            if ( mimeType != basePartService )
                supportedPatterns.append( mimeType );
    }

    return supportedPatterns;
}

void Shell::fileOpen()
{
	// this slot is called whenever the File->Open menu is selected,
	// the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
	// button is clicked
    if ( !m_fileformatsscanned )
    {
        if ( m_doc )
            m_fileformats = m_doc->supportedMimeTypes();

        if ( m_fileformats.isEmpty() )
            m_fileformats = fileFormats();

        m_fileformatsscanned = true;
    }

    QString startDir;
    if ( m_part->url().isLocalFile() )
        startDir = m_part->url().toLocalFile();
    KFileDialog dlg( startDir, QString(), this );
    dlg.setOperationMode( KFileDialog::Opening );

    // A directory may be a document. E.g. comicbook generator.
    if ( m_fileformats.contains( "inode/directory" ) )
        dlg.setMode( dlg.mode() | KFile::Directory );

    if ( m_fileformatsscanned && m_fileformats.isEmpty() )
        dlg.setFilter( i18n( "*|All Files" ) );
    else
        dlg.setMimeFilter( m_fileformats );
    dlg.setCaption( i18n( "Open Document" ) );
    if ( !dlg.exec() )
        return;
    KUrl url = dlg.selectedUrl();
    if ( !url.isEmpty() )
        openUrl( url );
}

void Shell::slotQuit()
{
    kapp->closeAllWindows();
}

// only called when starting the program
void Shell::setFullScreen( bool useFullScreen )
{
    if( useFullScreen )
        setWindowState( windowState() | Qt::WindowFullScreen ); // set
    else
        setWindowState( windowState() & ~Qt::WindowFullScreen ); // reset
}

void Shell::showEvent(QShowEvent *e)
{
    if (m_showMenuBarAction)
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

#include "shell.moc"

/* kate: replace-tabs on; indent-width 4; */
