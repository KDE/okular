/* This file is part of the KDE libraries
   Copyright (C) 2001, 2003 Christophe Devriese <oelewapperke@kde.org>
   Copyright (C) 2001  <kurt@granroth.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include "kpdf_shell.h"
#include "kpdf_shell.moc"
#include "kpdf_part.h"
#include "part.h"
#include "kpdf_pagewidget.h"

#include <kaction.h>
#include <kconfig.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kparts/componentfactory.h>
#include <kio/netaccess.h>

#include <qcursor.h>

using namespace KPDF;

Shell::Shell()
  : KParts::MainWindow(0, "KPDF::Shell")
{
  // set the shell's ui resource file
  setXMLFile("kpdf_shell.rc");

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
      // then, setup our actions
      setupActions();
      // tell the KParts::MainWindow that this is indeed the main widget
      setCentralWidget(m_part->widget());
      // and integrate the part's GUI with the shell's
      setupGUI(ToolBar | Keys | Save);
      createGUI(m_part);
    }
  }
  else
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, i18n("Unable to find kpdf part."));
    kapp->quit();
    // we return here, cause kapp->quit() only means "exit the
    // next time we enter the event loop...
    return;
  }
  PDFPartView * partView = static_cast<PDFPartView *>(m_part->widget());
  connect( partView->outputdev, SIGNAL( rightClick() ),SLOT( slotRMBClick() ) );
  
  readSettings();
}

Shell::~Shell()
{
    writeSettings();
}

void Shell::openURL( const KURL & url )
{
    if ( m_part && m_part->openURL( url ) ) recent->addURL (url);
}


void Shell::readSettings()
{
    recent->loadEntries( KGlobal::config() );
    KGlobal::config()->setDesktopGroup();
    bool fullScreen = KGlobal::config()->readBoolEntry( "FullScreen", false );
    setFullScreen( fullScreen );
}

void Shell::writeSettings()
{
    saveMainWindowSettings(KGlobal::config(), "MainWindow");
    recent->saveEntries( KGlobal::config() );
    KGlobal::config()->setDesktopGroup();
    KGlobal::config()->writeEntry( "FullScreen", m_fullScreenAction->isChecked());
    KGlobal::config()->sync();
}

  void
Shell::setupActions()
{
  KStdAction::open(this, SLOT(fileOpen()), actionCollection());
  recent = KStdAction::openRecent( this, SLOT( openURL( const KURL& ) ),
				    actionCollection() );
  KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
  KStdAction::print(m_part, SLOT(print()), actionCollection());
  KStdAction::quit(this, SLOT(slotQuit()), actionCollection());

  
  setStandardToolBarMenuEnabled(true);

  m_showMenuBarAction = KStdAction::showMenubar( this, SLOT( slotShowMenubar() ), actionCollection(), "options_show_menubar" );
  m_fullScreenAction = KStdAction::fullScreen( this, SLOT( slotUpdateFullScreen() ), actionCollection(), this );
  m_popup = new KPopupMenu( this, "rmb popup" );
  m_popup->insertTitle( i18n( "Full Screen Options" ) );
  m_fullScreenAction->plug( m_popup );
}

  void
Shell::saveProperties(KConfig* config)
{
  // the 'config' object points to the session managed
  // config file.  anything you write here will be available
  // later when this app is restored
    config->writePathEntry( "URL", m_part->url().url() );
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
    KURL url ( config->readPathEntry( "URL" ) );
    if ( url.isValid() )
        openURL( url );
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
  KURL saveURL = KFileDialog::getSaveURL(
					 m_part->url().isLocalFile()
					 ? m_part->url().url()
					 : m_part->url().fileName(),
					 QString::null,
					 m_part->widget(),
					 QString::null );
  if( !KIO::NetAccess::upload( m_part->url().path(),
			       saveURL, static_cast<QWidget*>( 0 ) ) )
	; // TODO: Proper error dialog

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

void Shell::setFullScreen( bool useFullScreen )
{
    if( useFullScreen )
        showFullScreen();
    else
        showNormal();
}

void Shell::slotUpdateFullScreen()
{
    if( m_fullScreenAction->isChecked())
    {
	menuBar()->hide();
	toolBar()->hide();
        //todo fixme
	//m_pdfpart->setFullScreen( true );
	showFullScreen();
#if 0
	kapp->installEventFilter( m_fsFilter );
	if ( m_gvpart->document()->isOpen() )
		slotFitToPage();
#endif
    }
    else
    {
	//kapp->removeEventFilter( m_fsFilter );
        //m_pdfpart->setFullScreen( false );
	menuBar()->show();
	toolBar()->show();
	showNormal();
    }
}

void Shell::slotRMBClick()
{
    m_popup->exec( QCursor::pos() );
}


// vim:ts=2:sw=2:tw=78:et
