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
#include <kpopupmenu.h>
#include <kparts/componentfactory.h>
#include <kio/netaccess.h>
#include <kmainwindowiface.h> 

// local includes
#include "shell.h"

using namespace KPDF;

Shell::Shell()
  : KParts::MainWindow(0, "KPDF::Shell"), m_menuBarWasShown(true), m_toolBarWasShown(true)
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
  connect( this, SIGNAL( restoreDocument(const KURL &, int) ),m_part, SLOT( restoreDocument(const KURL &, int)));
  connect( this, SIGNAL( saveDocumentRestoreInfo(KConfig*) ), m_part, SLOT( saveDocumentRestoreInfo(KConfig*)));

  readSettings();
  if (!KGlobal::config()->hasGroup("MainWindow"))
  {
    KMainWindowInterface kmwi(this);
    kmwi.maximize();
  }
}

Shell::~Shell()
{
    if(m_part) writeSettings();
}

void Shell::openURL( const KURL & url )
{
    if ( m_part && m_part->openURL( url ) ) m_recent->addURL (url);
    else m_recent->removeURL(url);
}


void Shell::readSettings()
{
    m_recent->loadEntries( KGlobal::config() );
    KGlobal::config()->setDesktopGroup();
    bool fullScreen = KGlobal::config()->readBoolEntry( "FullScreen", false );
    setFullScreen( fullScreen );
    
    // necessary to make fullscreen mode obey the last showmenubar /  showtoolbarsettings
    KGlobal::config()->setGroup("MainWindow");
    if (KGlobal::config()->readBoolEntry( "MenuBar", true ))
    {
      m_showMenuBarAction->setChecked(true);
      menuBar()->show();
    }
    else
    {
      m_showMenuBarAction->setChecked(false);
      menuBar()->hide();
    }

    KGlobal::config()->setGroup("MainWindow Toolbar mainToolBar");
    if (KGlobal::config()->readBoolEntry("Hidden", false))
    {
      m_showToolBarAction->setChecked(true);
      toolBar()->hide();
    }
    else
    {
      m_showToolBarAction->setChecked(false);
      toolBar()->show();
    }
}

void Shell::writeSettings()
{
    saveMainWindowSettings(KGlobal::config(), "MainWindow");
    m_recent->saveEntries( KGlobal::config() );
    KGlobal::config()->setDesktopGroup();
    KGlobal::config()->writeEntry( "FullScreen", m_fullScreenAction->isChecked());
    KGlobal::config()->sync();
}

  void
Shell::setupActions()
{
  KStdAction::open(this, SLOT(fileOpen()), actionCollection());
  m_recent = KStdAction::openRecent( this, SLOT( openURL( const KURL& ) ),
				    actionCollection() );
  KStdAction::print(m_part, SLOT(slotPrint()), actionCollection());
  KStdAction::quit(this, SLOT(slotQuit()), actionCollection());


  setStandardToolBarMenuEnabled(true);

  m_showMenuBarAction = KStdAction::showMenubar( this, SLOT( slotShowMenubar() ), actionCollection());
  KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
  m_fullScreenAction = KStdAction::fullScreen( this, SLOT( slotUpdateFullScreen() ), actionCollection(), this );
}

  void
Shell::saveProperties(KConfig* config)
{
  // the 'config' object points to the session managed
  // config file.  anything you write here will be available
  // later when this app is restored
    emit saveDocumentRestoreInfo(config);
}

void Shell::slotShowMenubar()
{
    if ( m_showMenuBarAction->isChecked() )
        menuBar()->show();
    else
        menuBar()->hide();
}


void Shell::readProperties(KConfig* config)
{
  // the 'config' object points to the session managed
  // config file.  this function is automatically called whenever
  // the app is being restored.  read in here whatever you wrote
  // in 'saveProperties'
  if(m_part)
  {
    KURL url ( config->readPathEntry( "URL" ) );
    if ( url.isValid() ) emit restoreDocument(url, config->readNumEntry( "Page", 1 ));
  }
}

  void
Shell::fileOpen()
{
  // this slot is called whenever the File->Open menu is selected,
  // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
  // button is clicked
    KURL url = KFileDialog::getOpenURL( QString::null, "application/pdf" );//getOpenFileName();

  if (!url.isEmpty())
    openURL(url);
}

  void
Shell::optionsConfigureToolbars()
{
  saveMainWindowSettings(KGlobal::config(), "MainWindow");
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

#include "shell.moc"
