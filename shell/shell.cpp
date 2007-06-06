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
#include <qtimer.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandardaction.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kparts/componentfactory.h>
#include <kio/netaccess.h>
#include <krecentfilesaction.h>
#include <kservicetypetrader.h>
#include <ktoggleaction.h>
#include <ktogglefullscreenaction.h>
#include <kactioncollection.h>

// local includes
#include "kdocumentviewer.h"

Shell::Shell(KCmdLineArgs* args, const KUrl &url)
  : KParts::MainWindow(), m_args(args), m_menuBarWasShown(true), m_toolBarWasShown(true)
{
  m_openUrl = url;
  init();
}

void Shell::init()
{
  setObjectName( QLatin1String( "okular::Shell" ) );
  setContextMenuPolicy( Qt::NoContextMenu );
  // set the shell's ui resource file
  setXMLFile("shell.rc");
  m_doc=0L;
  // this routine will find and load our Part.  it finds the Part by
  // name which is a bad idea usually.. but it's alright in this
  // case since our Part is made for this Shell
  KParts::Factory *factory = (KParts::Factory *) KLibLoader::self()->factory("libokularpart");
  if (factory)
  {
    // now that the Part is loaded, we cast it to a Part to get
    // our hands on it
    m_part = (KParts::ReadOnlyPart*) factory->createPart(this, this);
    if (m_part)
    {
      // we don't want the dummy mode
      QMetaObject::invokeMethod(m_part, "unsetDummyMode");

      // then, setup our actions
      setupActions();
      // tell the KParts::MainWindow that this is indeed the main widget
      setCentralWidget(m_part->widget());
      // and integrate the part's GUI with the shell's
      setupGUI(Keys | Save);
      createGUI(m_part);
      m_showToolBarAction = static_cast<KToggleAction*>(toolBarMenuAction());
      m_doc = qobject_cast<KDocumentViewer*>(m_part);
    }
  }
  else
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, i18n("Unable to find okular part."));
    m_part = 0;
    return;
  }
  connect( this, SIGNAL( restoreDocument(const KConfigGroup&) ),m_part, SLOT( restoreDocument(const KConfigGroup&)));
  connect( this, SIGNAL( saveDocumentRestoreInfo(KConfigGroup&) ), m_part, SLOT( saveDocumentRestoreInfo(KConfigGroup&)));
  connect( m_part, SIGNAL( enablePrintAction(bool) ), m_printAction, SLOT( setEnabled(bool)));

  readSettings();

  if (!KGlobal::config()->hasGroup("MainWindow"))
  {
    showMaximized();
  }
  setAutoSaveSettings();

  if (m_openUrl.isValid()) QTimer::singleShot(0, this, SLOT(delayedOpen()));
}

void Shell::delayedOpen()
{
   uint page = 0;
   if (m_args && m_doc)
   {
       QByteArray pageopt = m_args->getOption("page");
       page = pageopt.toUInt();
   }
   openUrl(m_openUrl, page);
}

Shell::~Shell()
{
    if ( m_part ) writeSettings();
    delete m_part;
    if ( m_args )
        m_args->clear();
}

void Shell::openUrl( const KUrl & url, uint page )
{
    if ( m_part )
    {
        if ( m_doc && m_args && m_args->isSet( "presentation" ) )
            m_doc->startPresentation();
        bool openOk = page > 0 && m_doc ? m_doc->openDocument( url, page ) : m_part->openUrl( url );
        if ( openOk ) m_recent->addUrl( url );
        else m_recent->removeUrl( url );
    }
}


void Shell::readSettings()
{
    m_recent->loadEntries( KGlobal::config()->group( "Recent Files" ) );
    m_recent->setEnabled( true ); // force enabling
    m_recent->setToolTip( i18n("Click to open a file\nClick and hold to open a recent file") );

    const KConfigGroup group = KGlobal::config()->group( "Desktop Entry" );
    bool fullScreen = group.readEntry( "FullScreen", false );
    setFullScreen( fullScreen );
}

void Shell::writeSettings()
{
    m_recent->saveEntries( KGlobal::config()->group( "Recent Files" ) );
    KConfigGroup group = KGlobal::config()->group( "Desktop Entry" );
    group.writeEntry( "FullScreen", m_fullScreenAction->isChecked() );
    KGlobal::config()->sync();
}

void Shell::setupActions()
{
  KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
  m_recent = KStandardAction::openRecent( this, SLOT( openUrl( const KUrl& ) ), actionCollection() );
  m_recent->setToolBarMode( KRecentFilesAction::MenuMode );
  m_recent->setToolButtonPopupMode( QToolButton::DelayedPopup );
  connect( m_recent, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
  m_recent->setWhatsThis( i18n( "<b>Click</b> to open a file or <b>Click and hold</b> to select a recent file" ) );
  m_printAction = KStandardAction::print( m_part, SLOT( slotPrint() ), actionCollection() );
  m_printAction->setEnabled( false );
  KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());

  setStandardToolBarMenuEnabled(true);

  m_showMenuBarAction = KStandardAction::showMenubar( this, SLOT( slotShowMenubar() ), actionCollection());
  KStandardAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
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
	QString constraint("([X-KDE-Priority] > 0) and (exist Library) ") ;
    KService::List offers = KServiceTypeTrader::self()->query("okular/Generator",constraint);
    QStringList supportedPatterns;

    if (offers.isEmpty())
    {
        return supportedPatterns;
    }

    KService::List::ConstIterator iterator = offers.begin();
    KService::List::ConstIterator end = offers.end();
    for (; iterator != end; ++iterator)
    {
        QStringList mimeTypes = (*iterator)->serviceTypes();
        foreach (const QString& mimeType, mimeTypes )
        {
            if ( !mimeType.contains( "okular" ) )
            {
                supportedPatterns.append( mimeType );
            }
        }
    }

    return supportedPatterns;
}

void Shell::fileOpen()
{
	// this slot is called whenever the File->Open menu is selected,
	// the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
	// button is clicked
    if (m_fileformats.isEmpty())
    {
        if ( m_doc )
            m_fileformats = m_doc->supportedMimeTypes();

        if ( m_fileformats.isEmpty() )
            m_fileformats = fileFormats();
    }

    if (!m_fileformats.isEmpty())
    {
        QString startDir;
        if ( m_openUrl.isLocalFile() )
            startDir = m_openUrl.path();
        KFileDialog dlg( startDir, QString(), this );
        dlg.setOperationMode( KFileDialog::Opening );
        dlg.setMimeFilter( m_fileformats );
        dlg.setCaption( i18n( "Open a document" ) );
        if (!dlg.exec())
            return;
        KUrl url = dlg.selectedUrl();
        if (!url.isEmpty())
            openUrl(url);
    }
    else
    {
        KMessageBox::error(this, i18n("No okular plugins were found."));
        slotQuit();
    }
}

  void
Shell::optionsConfigureToolbars()
{
  KEditToolBar dlg(factory());
  connect(&dlg, SIGNAL(newToolBarConfig()), this, SLOT(applyNewToolbarConfig()));
  dlg.exec();
}

  void
Shell::applyNewToolbarConfig()
{
  applyMainWindowSettings(KGlobal::config()->group("MainWindow"));
}

void Shell::slotQuit()
{
    kapp->closeAllWindows();
}

// only called when starting the program
void Shell::setFullScreen( bool useFullScreen )
{
    if( useFullScreen )
        showFullScreen();
    else
        showNormal();
}

void Shell::slotUpdateFullScreen()
{
    if(m_fullScreenAction->isChecked())
    {
      m_menuBarWasShown = m_showMenuBarAction->isChecked();
      m_showMenuBarAction->setChecked(false);
      menuBar()->hide();
      
      m_toolBarWasShown = m_showToolBarAction->isChecked();
      m_showToolBarAction->setChecked(false);
      toolBar()->hide();
      
      showFullScreen();
    }
    else
    {
      if (m_menuBarWasShown)
      {
        m_showMenuBarAction->setChecked(true);
        menuBar()->show();
      }
      if (m_toolBarWasShown)
      {
        m_showToolBarAction->setChecked(true);
        toolBar()->show();
      }
      showNormal();
    }
}

void Shell::slotShowMenubar()
{
    if ( m_showMenuBarAction->isChecked() )
        menuBar()->show();
    else
        menuBar()->hide();
}

#include "shell.moc"
