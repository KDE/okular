/*
 * kpdf_shell.cpp
 *
 * Copyright (C) 2001  <kurt@granroth.org>
 */
#include "kpdf_shell.h"
#include "kpdf_shell.moc"

#include <kaction.h>
#include <kconfig.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kurl.h>

using namespace KPDF;

Shell::Shell()
  : KParts::MainWindow(0, "KPDF::Shell")
{
  // set the shell's ui resource file
  setXMLFile("kpdf_shell.rc");

  // then, setup our actions
  setupActions();

  // this routine will find and load our Part.  it finds the Part by
  // name which is a bad idea usually.. but it's alright in this
  // case since our Part is made for this Shell
  KLibFactory *factory = KLibLoader::self()->factory("libkpdfpart");
  if (factory)
  {
    // now that the Part is loaded, we cast it to a Part to get
    // our hands on it
    m_part = static_cast<KParts::ReadOnlyPart*>(
                factory->create(this, "kpdf_part", "KParts::ReadOnlyPart"));

    if (m_part)
    {
      // tell the KParts::MainWindow that this is indeed the main widget
      setCentralWidget(m_part->widget());

      // and integrate the part's GUI with the shell's
      createGUI(m_part);
    }
  }
  else
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, "Could not find our Part!");
    kapp->quit();
    // we return here, cause kapp->quit() only means "exit the
    // next time we enter the event loop...
    return;
  }
  readSettings();
}

Shell::~Shell()
{
    writeSettings();
}

void Shell::openURL( const KURL & url )
{
    m_part->openURL( url );
    recent->addURL (url);
}


void Shell::readSettings()
{
    recent->loadEntries( KGlobal::config() );
}

void Shell::writeSettings()
{
    saveMainWindowSettings(KGlobal::config(), "MainWindow");
    recent->saveEntries( KGlobal::config() );
}


  void
Shell::setupActions()
{
  KStdAction::open(this, SLOT(fileOpen()), actionCollection());
  recent = KStdAction::openRecent( this, SLOT( openURL( const KURL& ) ),
				    actionCollection() );
  KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
  KStdAction::quit(kapp, SLOT(quit()), actionCollection());

  //createStandardStatusBarAction();
  setStandardToolBarMenuEnabled(true);

  KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
}

  void
Shell::saveProperties(KConfig* /*config*/)
{
  // the 'config' object points to the session managed
  // config file.  anything you write here will be available
  // later when this app is restored
}

  void
Shell::readProperties(KConfig* /*config*/)
{
  // the 'config' object points to the session managed
  // config file.  this function is automatically called whenever
  // the app is being restored.  read in here whatever you wrote
  // in 'saveProperties'
}

  void
Shell::fileOpen()
{
  // this slot is called whenever the File->Open menu is selected,
  // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
  // button is clicked
    KURL url = KFileDialog::getOpenURL();//getOpenFileName();

  if (!url.isEmpty())
    openURL(url);
}

  void
Shell::fileSaveAs()
{
    // this slot is called whenever the File->Save As menu is selected,
    /*
    QString file_name = KFileDialog::getSaveFileName();
    if (file_name.isEmpty() == false)
        saveAs(file_name);
    */
}

  void
Shell::optionsConfigureKeys()
{
  KKeyDialog::configureKeys(actionCollection(), "kpdf_shell.rc");
}

  void
Shell::optionsConfigureToolbars()
{
  saveMainWindowSettings(KGlobal::config(), "MainWindow");

  // use the standard toolbar editor
  KEditToolbar dlg(factory());
  connect(&dlg, SIGNAL(newToolbarConfig()),
      this, SLOT(applyNewToolbarConfig()));
  dlg.exec();
}

  void
Shell::applyNewToolbarConfig()
{
  applyMainWindowSettings(KGlobal::config(), "MainWindow");
}

// vim:ts=2:sw=2:tw=78:et
