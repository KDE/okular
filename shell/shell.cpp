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

// qt/kde includes
#include <qcursor.h>
#include <qtimer.h>
#include <kaction.h>
#include <kapplication.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kparts/componentfactory.h>
#include <kio/netaccess.h>
#include <kmainwindowiface.h>

// local includes
#include "shell.h"

using namespace KPDF;

Shell::Shell()
  : KParts::MainWindow(0, "KPDF::Shell"), m_menuBarWasShown(true), m_toolBarWasShown(true)
{
  init();
}

Shell::Shell(const KURL &url)
 : KParts::MainWindow(0, "KPDF::Shell"), m_menuBarWasShown(true), m_toolBarWasShown(true)
{
  m_openUrl = url;
  init();
}

void Shell::init()
{
  // set the shell's ui resource file
  setXMLFile("shell.rc");

  // this routine will find and load our Part.  it finds the Part by
  // name which is a bad idea usually.. but it's alright in this
  // case since our Part is made for this Shell
  KParts::Factory *factory = (KParts::Factory *) KLibLoader::self()->factory("libkpdfpart");
  if (factory)
  {
    // now that the Part is loaded, we cast it to a Part to get
    // our hands on it
    m_part = (KParts::ReadOnlyPart*) factory->createPart(this, "kpdf_part", this, 0, "KParts::ReadOnlyPart");
    if (m_part)
    {
      // then, setup our actions
      setupActions();
      // tell the KParts::MainWindow that this is indeed the main widget
      setCentralWidget(m_part->widget());
      // and integrate the part's GUI with the shell's
      setupGUI(Keys | Save);
      createGUI(m_part);
      m_showToolBarAction = static_cast<KToggleAction*>(toolBarMenuAction());
    }
  }
  else
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, i18n("Unable to find kpdf part."));
    m_part = 0;
    return;
  }
  connect( this, SIGNAL( restoreDocument(KConfig*) ),m_part, SLOT( restoreDocument(KConfig*)));
  connect( this, SIGNAL( saveDocumentRestoreInfo(KConfig*) ), m_part, SLOT( saveDocumentRestoreInfo(KConfig*)));
  connect( m_part, SIGNAL( enablePrintAction(bool) ), m_printAction, SLOT( setEnabled(bool)));
  
  readSettings();
  if (!KGlobal::config()->hasGroup("MainWindow"))
  {
    KMainWindowInterface kmwi(this);
    kmwi.maximize();
  }
  setAutoSaveSettings();
  
  if (m_openUrl.isValid()) QTimer::singleShot(0, this, SLOT(delayedOpen()));
}

void Shell::delayedOpen()
{
   openURL(m_openUrl);
}

Shell::~Shell()
{
    if(m_part) writeSettings();
}

void Shell::openURL( const KURL & url )
{
    if ( m_part )
    {
        bool openOk = m_part->openURL( url );
        if ( openOk )
          m_recent->addURL( url );
        else
          m_recent->removeURL( url );
    }
}


void Shell::readSettings()
{
    m_recent->loadEntries( KGlobal::config() );
    m_recent->setEnabled( true ); // force enabling
    m_recent->setToolTip( i18n("Click to open a file\nClick and hold to open a recent file") );

    KGlobal::config()->setDesktopGroup();
    bool fullScreen = KGlobal::config()->readBoolEntry( "FullScreen", false );
    setFullScreen( fullScreen );
}

void Shell::writeSettings()
{
    m_recent->saveEntries( KGlobal::config() );
    KGlobal::config()->setDesktopGroup();
    KGlobal::config()->writeEntry( "FullScreen", m_fullScreenAction->isChecked());
    KGlobal::config()->sync();
}

void Shell::setupActions()
{
  KAction * openAction = KStdAction::open(this, SLOT(fileOpen()), actionCollection());
  m_recent = KStdAction::openRecent( this, SLOT( openURL( const KURL& ) ), actionCollection() );
  connect( m_recent, SIGNAL( activated() ), openAction, SLOT( activate() ) );
  m_recent->setWhatsThis( i18n( "<b>Click</b> to open a file or <b>Click and hold</b> to select a recent file" ) );
  m_printAction = KStdAction::print( m_part, SLOT( slotPrint() ), actionCollection() );
  m_printAction->setEnabled( false );
  KStdAction::quit(this, SLOT(slotQuit()), actionCollection());

  setStandardToolBarMenuEnabled(true);

  m_showMenuBarAction = KStdAction::showMenubar( this, SLOT( slotShowMenubar() ), actionCollection());
  KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
  m_fullScreenAction = KStdAction::fullScreen( this, SLOT( slotUpdateFullScreen() ), actionCollection(), this );
}

void Shell::saveProperties(KConfig* config)
{
  // the 'config' object points to the session managed
  // config file.  anything you write here will be available
  // later when this app is restored
    emit saveDocumentRestoreInfo(config);
}

void Shell::readProperties(KConfig* config)
{
  // the 'config' object points to the session managed
  // config file.  this function is automatically called whenever
  // the app is being restored.  read in here whatever you wrote
  // in 'saveProperties'
  if(m_part)
  {
    emit restoreDocument(config);
  }
}

  void
Shell::fileOpen()
{
  // this slot is called whenever the File->Open menu is selected,
  // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
  // button is clicked
    KURL url = KFileDialog::getOpenURL( QString::null, "application/pdf application/postscript" );//getOpenFileName();

  if (!url.isEmpty())
    openURL(url);
}

  void
Shell::optionsConfigureToolbars()
{
  KEditToolbar dlg(factory());
  connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(applyNewToolbarConfig()));
  dlg.exec();
}

  void
Shell::applyNewToolbarConfig()
{
  applyMainWindowSettings(KGlobal::config(), "MainWindow");
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
