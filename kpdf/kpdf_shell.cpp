/*
 * kpdf_shell.cpp
 *
 * Copyright (C) 2001  <kurt@granroth.org>
 */
#include "kpdf_shell.h"
#include "kpdf_shell.moc"

#include <kkeydialog.h>
#include <kconfig.h>
#include <kurl.h>

#include <kedittoolbar.h>

#include <kaction.h>
#include <kstdaction.h>

#include <klibloader.h>
#include <kmessagebox.h>
#include <kstatusbar.h>

using namespace KPDF;

Shell::Shell()
  : KParts::MainWindow(0, "KPDF::Shell")
{
  // set the shell's ui resource file
  setXMLFile("kpdf_shell.rc");

  // then, setup our actions
  setupActions();

  // and a status bar
  statusBar()->show();

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
}

Shell::~Shell()
{
}

  void
Shell::load(const KURL& url)
{
  m_part->openURL( url );
}

  void
Shell::setupActions()
{
  KStdAction::quit(kapp, SLOT(quit()), actionCollection());

  m_toolbarAction   = KStdAction::showToolbar(this, SLOT(optionsShowToolbar()), actionCollection());
  m_statusbarAction = KStdAction::showStatusbar(this, SLOT(optionsShowStatusbar()), actionCollection());

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
Shell::optionsShowToolbar()
{
  // this is all very cut and paste code for showing/hiding the
  // toolbar
  if (m_toolbarAction->isChecked())
    toolBar()->show();
  else
    toolBar()->hide();
}

  void
Shell::optionsShowStatusbar()
{
  // this is all very cut and paste code for showing/hiding the
  // statusbar
  if (m_statusbarAction->isChecked())
    statusBar()->show();
  else
    statusBar()->hide();
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
