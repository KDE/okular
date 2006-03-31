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
#include <ktoolbar.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kparts/componentfactory.h>
#include <kio/netaccess.h>
#include <kmainwindowiface.h>
#include <ktrader.h>
#include <ktempfile.h>
#include <kfilterbase.h>
#include <kfilterdev.h>

// local includes
#include "shell.h"

using namespace oKular;

Shell::Shell()
  : KParts::MainWindow(0, "oKular::Shell"), m_menuBarWasShown(true), m_toolBarWasShown(true)
{
  init();
}

Shell::Shell(const KUrl &url)
  : KParts::MainWindow(0, "oKular::Shell"), m_menuBarWasShown(true), m_toolBarWasShown(true)
{
  m_openUrl = url;
  init();
}

void Shell::init()
{
  // set the shell's ui resource file
  setXMLFile("shell.rc");
  m_fileformats=0L;
  m_tempfile=0L;
  // this routine will find and load our Part.  it finds the Part by
  // name which is a bad idea usually.. but it's alright in this
  // case since our Part is made for this Shell
  KParts::Factory *factory = (KParts::Factory *) KLibLoader::self()->factory("liboKularpart");
  if (factory)
  {
    // now that the Part is loaded, we cast it to a Part to get
    // our hands on it
    m_part = (KParts::ReadOnlyPart*) factory->createPart(this, "oKular_part", this, 0, "KParts::ReadOnlyPart");
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
    KMessageBox::error(this, i18n("Unable to find oKular part."));
    m_part = 0;
    return;
  }
  connect( this, SIGNAL( restoreDocument(const KUrl &, int) ),m_part, SLOT( restoreDocument(const KUrl &, int)));
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
    if ( m_part ) writeSettings();
    if ( m_fileformats ) delete m_fileformats;
    if ( m_tempfile ) delete m_tempfile; 
}

void Shell::openURL( const KUrl & url )
{
    if ( m_part )
    {
        bool openOk = m_part->openURL( url );
        if ( openOk ) m_recent->addUrl( url );
        else m_recent->removeUrl( url );
        m_printAction->setEnabled( openOk );
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
  m_recent = KStdAction::openRecent( this, SLOT( openURL( const KUrl& ) ), actionCollection() );
  connect( m_recent, SIGNAL( activated() ), openAction, SLOT( trigger() ) );
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
    KUrl url ( config->readPathEntry( "URL" ) );
    if ( url.isValid() ) emit restoreDocument(url, config->readNumEntry( "Page", 1 ));
  }
}

// this comes from kviewpart/kviewpart.cpp, fileformats function	

QStringList* Shell::fileFormats()
{
	QString constraint("([X-KDE-Priority] > 0) and (exist Library) ") ;
	KTrader::OfferList offers=KTrader::self()->query("oKular/Generator",QString::null,constraint, QString::null);
    QStringList supportedMimeTypes;
    QStringList * supportedPattern;

    if (offers.isEmpty())
	{
        return 0;
	}

    bool bzip2Available = (KFilterBase::findFilterByMimeType( "application/x-bzip2" ) != 0L);
    supportedPattern = new QStringList;
    KTrader::OfferList::ConstIterator iterator = offers.begin();
    KTrader::OfferList::ConstIterator end = offers.end();
    QStringList::Iterator mimeType;
    QString tmp;
    QStringList mimeTypes,pattern,extensions;
    QString allExt,comment;
    for (; iterator != end; ++iterator)
    {
        KService::Ptr service = *iterator;
        mimeTypes = service->serviceTypes();
        
        for (mimeType=mimeTypes.begin();mimeType!=mimeTypes.end();++mimeType)
        {
            if (! (*mimeType).contains("oKular"))
            {
                supportedMimeTypes << *mimeType;
            
               pattern = KMimeType::mimeType(*mimeType)->patterns();
               extensions.clear();
               while(!pattern.isEmpty())
               {
                    tmp=pattern.front().trimmed();
                    extensions.append(tmp);
                    if (tmp.indexOf(".gz", -3) == -1)
                        extensions.append(tmp+".gz");
                    if ((bzip2Available) && (tmp.indexOf(".bz2", -4) == -1)) 
                        extensions.append(tmp+".bz2");
		    pattern.pop_front();
                }
	       	comment=KMimeType::mimeType(*mimeType)->comment();
		if (! comment.contains("Unknown"))
	                supportedPattern->append(extensions.join(" ") + "|" + comment);
                allExt+=extensions.join(" ");
            }
        }
    }

	supportedPattern->prepend(allExt + "|All Files");
	
	
	return supportedPattern;
	

}
bool Shell::handleCompressed(KUrl & url, const QString &path, const KMimeType::Ptr mimetype)
{

    // we are working with a compressed file, decompressing
    // temporary file for decompressing
    m_tempfile = new KTempFile;
    if ( !m_tempfile )
    {
        KMessageBox::error(this, 
            i18n("<qt><strong>File Error!</strong> Could not create "
            "temporary file.</qt>"));
        return false;
    }

    m_tempfile->setAutoDelete(true);

    // something failed while creating the tempfile
    if ( m_tempfile->status() != 0 )
    {
        KMessageBox::error( this, 
            i18n("<qt><strong>File Error!</strong> Could not create temporary file "
                "<nobr><strong>%1</strong></nobr>.</qt>").arg(
                strerror(m_tempfile->status())));
                delete m_tempfile;
                return false;
    }

    // decompression filer
    QIODevice* filterDev;
    if (( mimetype->parentMimeType() == "application/x-gzip" ) ||
        ( mimetype->parentMimeType() == "application/x-bzip2" ))
            filterDev = KFilterDev::deviceForFile(path, mimetype->parentMimeType());
    else
            filterDev = KFilterDev::deviceForFile(path);

    if (!filterDev)
    {
        delete m_tempfile;
        return false;
    }

    if ( !filterDev->open(IO_ReadOnly) )
    {
        KMessageBox::detailedError( this, 
            i18n("<qt><strong>File Error!</strong> Could not open the file "
            "<nobr><strong>%1</strong></nobr> for uncompression. "
            "The file will not be loaded.</qt>").arg(path),
            i18n("<qt>This error typically occurs if you do "
            "not have enough permissions to read the file. " 
            "You can check ownership and permissions if you "
            "right-click on the file in the Konqueror "
            "file manager and then choose the 'Properties' menu.</qt>"));

            delete filterDev;
            delete m_tempfile;
            return false;
    }

    QByteArray buf(1024);
    int read = 0, wrtn = 0;

    while ((read = filterDev->readBlock(buf.data(), buf.size())) > 0)
    {
        wrtn = m_tempfile->file()->writeBlock(buf.data(), read);
        if ( read != wrtn )
            break;
    }
    delete filterDev;
    if ((read != 0) || (m_tempfile->file()->size() == 0))
    {
        KMessageBox::detailedError(this, 
            i18n("<qt><strong>File Error!</strong> Could not uncompress "
            "the file <nobr><strong>%1</strong></nobr>. "
            "The file will not be loaded.</qt>").arg( path ),
            i18n("<qt>This error typically occurs if the file is corrupt. "
            "If you want to be sure, try to decompress the file manually "
            "using command-line tools.</qt>"));
        delete m_tempfile;
        return false;
    }
    m_tempfile->close();
    url=m_tempfile->name();
}

void Shell::fileOpen()
{
	// this slot is called whenever the File->Open menu is selected,
	// the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
	// button is clicked
    if (!m_fileformats)
	   m_fileformats = fileFormats();

	if (m_fileformats)
    {
        KUrl url = KFileDialog::getOpenURL( QString::null, m_fileformats->join("\n") );//getOpenFileName();
        bool reallyOpen=!url.isEmpty();
        if (reallyOpen)
        {
            QString path = url.path();
            KMimeType::Ptr mimetype = KMimeType::findByPath( path );
            if (( mimetype->name() == "application/x-gzip" ) 
                  || ( mimetype->name() == "application/x-bzip2" ) 
                  || ( mimetype->parentMimeType() == "application/x-gzip" ) 
                  || ( mimetype->parentMimeType() == "application/x-bzip2" )
               )
            {
                reallyOpen=handleCompressed(url,path,mimetype);
            }
        }
        if (reallyOpen)
            openURL(url);
    }
    else
    {
        KMessageBox::error(this, i18n("No oKular plugins were found."));
        slotQuit();
    }
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
