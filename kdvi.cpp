//
// Application: kdvi
//
// Previewer for TeX DVI files.
//
#include <unistd.h>
#include <signal.h>

#include <qlayout.h>
#include <qdir.h>
#include <qfiledialog.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qfileinfo.h>
#include <qkeycode.h>
#include <qlineedit.h>
#include <qframe.h>
#include <qdragobject.h>
#include <qsplitter.h>

#include <kmenubar.h>
#include <kmessagebox.h>
#include <kapp.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kconfig.h>

#include <kio/netaccess.h>

#include "kdvi.h"
#include "scrbox.h"
#include "print.h"
#include "pushbutton.h"
#include "prefs.h"
#include "version.h"

enum {ID_STAT_SHRINK, ID_STAT_PAGE, ID_STAT_MSG, ID_STAT_XY};
enum {ID_OPT_PK = 3, ID_OPT_PS, ID_OPT_MB, ID_OPT_BB, ID_OPT_TB, ID_OPT_SB, ID_OPT_SC };

kdvi::kdvi( const QString &fname, QWidget *, const char *name )
	: KTMainWindow( name )
{
	msg = NULL;
	ssmenu = NULL;
	hbl = NULL;
	prefs = NULL;

	keys = new KAccel(this);

	readConfig();
	setMinimumSize( 400, 60 );
	setCaption( kapp->caption() );
	tipgroup = new QToolTipGroup( this, "TipGroup" );
	connect( tipgroup, SIGNAL(showTip(const QString &)), 
		 SLOT(showTip(const QString &)) );
	connect( tipgroup, SIGNAL(removeTip()), SLOT(removeTip()) );

  // Create KPanner for toolBar2 and dviwindow
  
  	kpan = new QSplitter( QSplitter::Horizontal, this, "splitter");

	setView( kpan, TRUE );
	setFrameBorderWidth( 4 );
	
  // Create a dvi window

	dviwin = new dviWindow( basedpi, mfmode, paper, makepk,
				kpan, "dviWindow" );
	connect( dviwin, SIGNAL(currentPage(int)), SLOT(setPage(int)) );
	connect( dviwin, SIGNAL(shrinkChanged(int)), SLOT(shrinkChanged(int)) );
	connect( dviwin, SIGNAL(fileChanged()), SLOT(fileChanged()) );
	connect( dviwin, SIGNAL(statusChange(const QString &)),
			 SLOT(showTip(const QString &)) );
	connect( dviwin, SIGNAL(setPoint(QPoint)), SLOT(showPoint(QPoint)) );
	
	// Create a menubar

	makeMenuBar();

	// Create toolbars
	makeButtons();
	makeToolBar2( kpan );

	kpan->moveToLast(dviwin);
	
    // Create a statusbar

	makeStatusBar( i18n("No document") );

	QValueList<int> size;
	size << 10 << 90;
	kpan->setSizes(size);

  // Create RMB menu

	rmbmenu = new QPopupMenu;
	rmbmenu->setMouseTracking( TRUE );
	rmbmenu->connectItem( rmbmenu->insertItem(i18n("Toggle Menubar")),
		      this, SLOT(toggleShowMenubar()) );
	rmbmenu->connectItem( rmbmenu->insertItem(i18n("Mark page")),
			marklist, SLOT(markSelected()) );
	rmbmenu->connectItem( rmbmenu->insertItem(i18n("Redraw")),
			dviwin, SLOT(drawPage()) );
	rmbmenu->connectItem( rmbmenu->insertItem(i18n("Preferences ...")),
			this, SLOT(optionsPreferences()) );

  // Bind keys

	bindKeys();
	updateMenuAccel();

	setAcceptDrops(true);

  // Read config options

	applyPreferences();

	selectSmall();
	dviwin->installEventFilter( this );

	message( "" );
	openFile(fname);
}

kdvi::~kdvi()
{
}

// This avoids seg fault at destructor:

void kdvi::closeEvent( QCloseEvent *e )
{
  KTMainWindow::closeEvent(e);
  e->accept();
}

static QPopupMenu *m_f, *m_v, *m_p, *m_o, *m_h;
static int m_fn, m_fo, m_fr, m_fp, m_fx, m_vi, m_vo, m_vf, m_vw, m_vr, m_pp, m_pn, m_pf, m_pl, m_pg,
	m_op, m_ok, m_of, m_o0, m_om, m_ob, m_ot, m_os, m_ol,  m_hc; // , m_ha, m_hq;
	
void kdvi::makeMenuBar()
{
	QPopupMenu *p = new QPopupMenu;
	CHECK_PTR( p );

	m_fn = p->insertItem( i18n("&New"),	this, SLOT(fileNew()) );
	m_fo = p->insertItem( i18n("&Open ..."),	this, SLOT(fileOpen()) );

	recentmenu = new QPopupMenu;
	CHECK_PTR( recentmenu );

	connect( recentmenu, SIGNAL(activated(int)), SLOT(openRecent(int)) );

	m_fr = p->insertItem( i18n("Open &recent"),	recentmenu );
	
	m_fp = p->insertItem( i18n("&Print ..."),	this, SLOT(filePrint()));
	m_fx = p->insertItem( i18n("&Quit"), this, SLOT(fileExit()));

	m_f = p;
	menuBar()->insertItem( i18n("&File"), p, -2 );

	p = new QPopupMenu;
	CHECK_PTR( p );
	m_vi = p->insertItem( i18n("Zoom &in"),	dviwin, SLOT(prevShrink()) );
	m_vo = p->insertItem( i18n("Zoom &out"),	dviwin, SLOT(nextShrink()) );
	m_vf = p->insertItem( i18n("&Fit to page"),	this, SLOT(viewFitPage()) );
	m_vw = p->insertItem( i18n("Fit to page &width"),	this, SLOT(viewFitPageWidth()));
	p->insertSeparator();
	m_vr = p->insertItem( i18n("&Redraw page"),	dviwin, SLOT(drawPage()) );

	m_v = p;
	menuBar()->insertItem( i18n("&View"), p, -2 );

	p = new QPopupMenu;
	CHECK_PTR( p );
	m_pp = p->insertItem( i18n("&Previous"),	dviwin, SLOT(prevPage()) );
	m_pn = p->insertItem( i18n("&Next"),		dviwin, SLOT(nextPage()) );
	m_pf = p->insertItem( i18n("&First"),	dviwin, SLOT(firstPage()) );
	m_pl = p->insertItem( i18n("&Last"),		dviwin, SLOT(lastPage()) );
	m_pg = p->insertItem( i18n("&Go to ..."),	this,   SLOT(pageGoto()) );

	m_p = p;
	menuBar()->insertItem( i18n("&Page"), p, -2 );

	p = new QPopupMenu;
	CHECK_PTR( p );
	p->setCheckable( TRUE );
	m_op = p->insertItem( i18n("&Preferences ..."), this, SLOT(optionsPreferences()));
	m_ok = p->insertItem( i18n("&Keys ..."), this, SLOT(configKeys()));
	p->insertSeparator();
	m_of = p->insertItem( i18n("Make PK-&fonts"), this, SLOT(toggleMakePK()) );
	p->setItemChecked( m_of, makepk );
	m_o0 = p->insertItem( i18n("Show PS"), this, SLOT(toggleShowPS()));
	p->setItemChecked( m_o0, showPS );
	m_om = p->insertItem( i18n("Show &Menubar"), this, SLOT(toggleShowMenubar()) );
	p->setItemChecked( m_om, !hideMenubar );
	m_ob = p->insertItem( i18n("Show &Buttons"), this, SLOT(toggleShowButtons()) );
	p->setItemChecked( m_ob, !hideButtons );
	m_ot = p->insertItem( i18n("Show Page Lis&t"), this, SLOT(toggleVertToolbar()) );
	p->setItemChecked( m_ol, vertToolbar );
	m_os = p->insertItem( i18n("Show &Statusbar"), this, SLOT(toggleShowStatusbar()) );
	p->setItemChecked( m_os, !hideStatusbar );
	m_ol = p->insertItem( i18n("Show Scro&llbars"), this, SLOT(toggleShowScrollbars()) );
	p->setItemChecked( m_ol, !hideScrollbars );

	m_o = p;
	menuBar()->insertItem( i18n("&Options"), p, -2 );
	optionsmenu = p;

	menuBar()->insertSeparator();

	QPopupMenu *help = helpMenu(i18n("DVI Viewer")
					     + " " + KDVI_VERSION
					     + i18n("\n\nby Markku Hihnala")
					     + " (mah@ee.oulu.fi)");

	m_h = help;
	m_hc = help->idAt(0);
	menuBar()->insertItem( i18n("&Help"), help );
	if ( hideMenubar )	menuBar()->hide();
}

void kdvi::makeButtons()
{
	QPixmap pm;

#define I(f,o,s,h) toolBar()->insertButton( BarIcon(f),\
	 0, SIGNAL(clicked()), o, SLOT(s()), TRUE, h);

	I( "fileopen",	this,	fileOpen,	i18n("Open document ...") )
	I( "reload",	dviwin,	drawPage,	i18n("Reload document") )
	I( "fileprint",	this,	filePrint,	i18n("Print ...") )
	toolBar()->insertSeparator();
	I( "start",		dviwin,	firstPage, 	i18n("Go to first page") )
	I( "back",		dviwin,	prevPage,	i18n("Go to previous page") )
	I( "forwpage",	dviwin,	goForward,	i18n("Go down then top of next page") )
	I( "forward",	dviwin,	nextPage,	i18n("Go to next page") )
	I( "finish",	dviwin,	lastPage,	i18n("Go to last page") )
	toolBar()->insertSeparator();
	I( "viewmag-",	dviwin,	nextShrink,	i18n("Decrease magnification") )
	I( "smalltext",	this,	selectSmall,	i18n("Small text") )
	I( "largetext",	this,	selectLarge,	i18n("Large text") )
	I( "viewmag+",	dviwin,	prevShrink,	i18n("Increase magnification") )
#undef	I

}

void kdvi::makeToolBar2(QWidget *parent)
{
	QPixmap pm;

	toolBar2 = new QFrame( parent );

	QBoxLayout * gl = new QBoxLayout( toolBar2, QBoxLayout::Down );

	sbox = new ScrollBox( toolBar2 );
	connect( sbox, SIGNAL(valueChanged(QPoint)),
		 dviwin, SLOT(scroll(QPoint)) );
	connect( sbox, SIGNAL(button3Pressed()), dviwin, SLOT(nextPage()) );
	connect( sbox, SIGNAL(button2Pressed()), dviwin, SLOT(prevPage()) );
	connect( dviwin, SIGNAL(pageSizeChanged( QSize )),
		 sbox, SLOT(setPageSize( QSize )) );
	connect( dviwin, SIGNAL(viewSizeChanged( QSize )),
		 sbox, SLOT(setViewSize( QSize )) );
	connect( dviwin, SIGNAL(currentPosChanged( QPoint )),
		 sbox, SLOT(setViewPos( QPoint )) );
	QToolTip::add( sbox, "", tipgroup, i18n("Scroll window and switch the page") );
	sbox->setFixedSize(70,80);
	gl->addWidget( sbox );

  // Create a MarkList

	marklist = new MarkList( toolBar2 );
	connect( marklist, SIGNAL(selected(const QString &)),
		SLOT(pageActivated(const QString &)) );
	QToolTip::add( marklist, "", tipgroup, i18n("Select page and mark pages for printing") );

	gl->addWidget( marklist );
	gl->activate();

	sbox->setPageSize( dviwin->pageSize() );
}

void kdvi::makeStatusBar( QString )
{
	QPixmap pm;

	//statusBar()->setInsertOrder( KStatusBar::RightToLeft );
	statusBar()->insertItem(i18n("X:0000, Y:0000 "), ID_STAT_XY);
	statusBar()->changeItem("", ID_STAT_XY);
	statusBar()->insertItem(i18n("Shrink: xx"), ID_STAT_SHRINK);
	statusBar()->changeItem("", ID_STAT_SHRINK);
	statusBar()->insertItem(i18n("Page: xxxx / xxxx"), ID_STAT_PAGE);
	statusBar()->changeItem("", ID_STAT_PAGE);
	statusBar()->insertItem("", ID_STAT_MSG);

}


void kdvi::bindKeys()
{
	//kKeys->registerWidget("kdvi", this);

#define AKCF(f,k,o,s)	keys->insertItem(f, f, k);\
			keys->connectItem( f, o, SLOT(s()));

	AKCF(i18n("New window"),	"CTRL+N",	this,	fileNew		)
	AKCF(i18n("Open file"),	"CTRL+O",	this,	fileOpen	)
	AKCF(i18n("Print dialog"),	"CTRL+P",	this,	filePrint	)
	AKCF(i18n("Quit"),		"CTRL+Q",	qApp,	quit		)
	AKCF(i18n("Zoom in"),	"Plus",		dviwin,	prevShrink	)
	AKCF(i18n("Zoom out"),	"Minus",	dviwin,	nextShrink	)
	AKCF(i18n("Fit window"),	"Asterisk",	this,	viewFitPage	)
	AKCF(i18n("Fit width"),	"Slash",	this,	viewFitPageWidth)
	AKCF(i18n("Redraw page"),	"CTRL+R",	dviwin,	drawPage	)
	AKCF(i18n("Next page"),	"Next",		dviwin,	nextPage	)
	AKCF(i18n("Previous page"),"Prior",	dviwin,	prevPage	)
	AKCF(i18n("Last page"),	"CTRL+Next",	dviwin,	lastPage	)
	AKCF(i18n("First page"),	"CTRL+Prior",	dviwin,	firstPage	)
	AKCF(i18n("Goto page"),	"CTRL+G",	this,	pageGoto	)
	AKCF(i18n("Configure keys"),"CTRL+K",	this,	configKeys	)
	AKCF(i18n("Toggle show PS"),"CTRL+V",	this,	toggleShowPS	)
	AKCF(i18n("Toggle menu bar"),"CTRL+M",	this,	toggleShowMenubar)
	AKCF(i18n("Toggle button bar"),"CTRL+B",	this,	toggleShowButtons)
	AKCF(i18n("Toggle page list"),"CTRL+T",	this,	toggleVertToolbar)
	AKCF(i18n("Toggle status bar"),"CTRL+S",	this,	toggleShowStatusbar)
	AKCF(i18n("Toggle scroll bars"),"CTRL+L",	this,	toggleShowScrollbars)
	AKCF(i18n("Help"),		"F1",		this,	helpContents	)

#undef AKCF

	config->setGroup( "kdvi" );
}

/*
static void changeMenuAccel ( QPopupMenu *menu, int id,
	const char *functionName )
{
	QString s = menu->text( id );
	if ( !s ) return;
	
	int i = s.find('\t');
	
	QString k = keyToString( kKeys->readCurrentKey( functionName ) );
	if( !k ) return;
	
	if ( i >= 0 )
		s.replace( i+1, s.length()-i, k );
	else {
		s += '\t';
		s += k;
	}
	
	menu->changeItem( s, id );
}
*/

void kdvi::updateMenuAccel()
{
	keys->changeMenuAccel( m_f, m_fn, i18n("New window") );
	keys->changeMenuAccel( m_f, m_fo, i18n("Open file") );
	keys->changeMenuAccel( m_f, m_fp, i18n("Print dialog") );
	keys->changeMenuAccel( m_f, m_fx, i18n("Quit") );
	keys->changeMenuAccel( m_v, m_vi, i18n("Zoom in") );
	keys->changeMenuAccel( m_v, m_vo, i18n("Zoom out") );
	keys->changeMenuAccel( m_v, m_vf, i18n("Fit window") );
	keys->changeMenuAccel( m_v, m_vw, i18n("Fit width") );
	keys->changeMenuAccel( m_v, m_vr, i18n("Redraw page") );
	keys->changeMenuAccel( m_p, m_pp, i18n("Previous page") );
	keys->changeMenuAccel( m_p, m_pn, i18n("Next page") );
	keys->changeMenuAccel( m_p, m_pf, i18n("First page") );
	keys->changeMenuAccel( m_p, m_pl, i18n("Last page") );
	keys->changeMenuAccel( m_p, m_pg, i18n("Goto page") );
	keys->changeMenuAccel( m_o, m_o0, i18n("Toggle show PS") );
	keys->changeMenuAccel( m_o, m_om, i18n("Toggle menu bar") );
	keys->changeMenuAccel( m_o, m_ob, i18n("Toggle button bar") );
	keys->changeMenuAccel( m_o, m_ot, i18n("Toggle page list") );
	keys->changeMenuAccel( m_o, m_os, i18n("Toggle status bar") );
	keys->changeMenuAccel( m_o, m_ol, i18n("Toggle scroll bars") );
	keys->changeMenuAccel( m_h, m_hc, i18n("Help") );
}

void kdvi::configKeys()
{
	KKeyDialog::configureKeys(keys);
	updateMenuAccel();
	config->setGroup( "kdvi" );
}

void kdvi::resizeEvent( QResizeEvent* e )
{
	KTMainWindow::resizeEvent( e );
	
	config->setGroup( "kdvi" );
	config->writeEntry( "Width", width() );
	config->writeEntry( "Height", height() );
}

bool kdvi::eventFilter( QObject *obj , QEvent *e )
{
	if ( obj != dviwin || e->type() != QEvent::MouseButtonPress )
		return FALSE;
	QMouseEvent *me = (QMouseEvent*)e;
	if ( me->button() != RightButton )
		return FALSE;
	rmbmenu->popup( dviwin->mapToGlobal(me->pos()), 0 );
	return TRUE;
}

void kdvi::fileNew( )
{
	newWindow();
}

void kdvi::newWindow( const char *name )
{	// Start a new process as the dvi and font routines are not reentrant
	// This needs to be changed..
	KProcess kp;
	kp.setExecutable( "kdvi" );
	if ( name )
		kp << name;
	kp.start( KProcess::DontCare, KProcess::NoCommunication );
}

void kdvi::fileOpen()
{
	QString dir;
	if ( dviName )
		dir = QFileInfo( dviName ).dirPath();	

	message( i18n("File open dialog is open") );
	KURL url = KFileDialog::getOpenURL( dir, "*.dvi");
	message( "" );

	QString f;
	KIO::NetAccess::download( url, f );
	openFile( f );
	KIO::NetAccess::removeTempFile( f );
}

void kdvi::fileExit()
{
	close();
	qApp->quit();
}

void kdvi::openRecent(QPoint p)
{
	recentmenu->popup( p );
}

void kdvi::openRecent(int id)
{
	openFile( recent.at(id) );
}

void kdvi::updateMarklist()
{
	QString s;
	marklist->setAutoUpdate( FALSE );
	marklist->clear();
	for (int i = dviwin->totalPages(); i>0; i--)
	{
		s.sprintf( "%4d", i );
		marklist->insertItem( s, 0 );
	}
	marklist->select(0);
	marklist->setAutoUpdate( TRUE );
	marklist->update();
}

void kdvi::openFile( QString name)
{
	if ( name.isEmpty() )
		return;

        // if name is just a relative filename, make it an absolute one
        if( ( name.at(0) != '/') && ( name.find( ':' ) == -1 ) )
            name = QDir().absPath() + "/" + name;

	QString oname( name );
	//name.detach();
	if ( ! QFileInfo( name ).isReadable() )
		name.append( ".dvi" );
	if ( ! QFileInfo( name ).isReadable() )
	{
		KMessageBox::sorry( this,
				i18n("Can't read file:\n") +
				oname);
		return;
	}
	QDir::setCurrent( QFileInfo( name ).dirPath() );
	dviName = name.copy();
	message( i18n("Opening ") + name + " ...");

	dviwin->setFile( name );
	dviwin->repaint();
	setCaption( QString()+kapp->caption()+": "+name );
	setPage();
	shrinkChanged( dviwin->shrink() );
	if (-1==recent.find(name))
	{
		recent.insert(0,name);
		if (recent.count()>(unsigned)recentmax)
			recent.removeLast();
		config->setGroup("RecentFiles");
		int i=recent.count();
		while ( i-->0 )
			config->writeEntry(QString().setNum(i),
						recent.at(i));
		config->setGroup( "kdvi" );
		recentmenu->clear();
		for (int n=recent.count(); n-->0;)
			recentmenu->insertItem( recent.at(n), n, 0 );
	}
	updateMarklist();
	message( i18n("Opened ") + name );
}

void kdvi::filePrint()
{
	if (!dviName)
		return;

	print * printdlg = new print( this, "printdlg" );

	message( i18n("Print dialog is open") );
	printdlg->setFile( dviName );
	printdlg->setCurrentPage( dviwin->page(), dviwin->totalPages() );
	printdlg->setMarkList( marklist->markList() );
	printdlg->exec();
	message( "" );
	delete printdlg;
}

void kdvi::viewFitPage()
{
	resize( rect().size() + dviwin->pageSize() - dviwin->viewSize()	);
	resize( rect().size() + dviwin->pageSize() - dviwin->viewSize()	);
	resize( rect().size() + dviwin->pageSize() - dviwin->viewSize()	);

	message( i18n("View size fits page") );
}

void kdvi::viewFitPageWidth()
{
	QSize s = rect().size();
	resize( s.width() + dviwin->pageSize().width() - dviwin->viewSize().width(),
		s.height() );

	message( i18n("View width fits page") );
}

void kdvi::optionsPreferences()
{
	if ( !prefs )
	{	prefs = new kdviprefs;
		connect( prefs, SIGNAL(preferencesChanged()),
			SLOT(applyPreferences()));
	}
	prefs->show();
}

void kdvi::applyPreferences()
{
	QString s;
	config->setGroup( "kdvi" );

	s = config->readEntry( "FontPath" );
	if ( !s.isEmpty() && s != dviwin->fontPath() )
		dviwin->setFontPath( s );

	basedpi = config->readNumEntry( "BaseResolution" );
	if ( basedpi <= 0 )
		config->writeEntry( "BaseResolution", basedpi = 300 );
	if ( basedpi != dviwin->resolution() )
		dviwin->setResolution( basedpi );

	mfmode =  config->readEntry( "MetafontMode" );
	if ( mfmode.isNull() )
		config->writeEntry( "MetafontMode", mfmode = "/" );
	if ( mfmode != dviwin->metafontMode() )
		dviwin->setMetafontMode( mfmode );

	paper = config->readEntry( "Paper" );
	if ( paper.isNull() )
		config->writeEntry( "Paper", paper = "A4" );
	if ( paper != dviwin->paper() )
		dviwin->setPaper( paper );

	s = config->readEntry( "Gamma" );
	if ( !s.isEmpty() && s.toFloat() != dviwin->gamma() )
		dviwin->setGamma( s.toFloat() );

	makepk = config->readNumEntry( "MakePK" );
	applyMakePK();

	showPS = config->readNumEntry( "ShowPS" );
	applyShowPS();
	
	dviwin->setAntiAlias( config->readNumEntry( "PS Anti Alias", 1 ) );

	hideMenubar = config->readNumEntry( "HideMenubar" );
	applyShowMenubar();

	hideButtons = config->readNumEntry( "HideButtons" );
	applyShowButtons();

	vertToolbar = config->readNumEntry( "VertToolbar" );
	applyVertToolbar();

	hideStatusbar = config->readNumEntry( "HideStatusbar" );
	applyShowStatusbar();

	hideScrollbars = config->readNumEntry( "HideScrollbars" );
	applyShowScrollbars();

	smallShrink = config->readNumEntry( "SmallShrink" );
	if (!smallShrink) smallShrink = 6;

	largeShrink = config->readNumEntry( "LargeShrink" );
	if (!largeShrink) largeShrink = 2;

	config->setGroup( "RecentFiles" );
	s = config->readEntry( "MaxCount" );
	if ( s.isNull() || (recentmax = s.toInt())<=0 )
		config->writeEntry( "MaxCount", recentmax = 10 );

	int n = 0;
	recent.clear();
	while ( n < recentmax )
	{
		s = config->readEntry(QString().setNum(n++));
		if ( !s.isEmpty() )
			recent.insert( 0, s );
		else	break;
	}
	recentmenu->clear();
	for (n=recent.count(); n-->0;)
		recentmenu->insertItem( recent.at(n), n, 0 );
	config->setGroup( "kdvi" );

	message(i18n("Preferences applied"));
}

PageDialog::PageDialog() : QDialog( 0, 0, 1 ),
	gb(i18n(" Page "),this), ed(&gb), ok(&gb), cancel(&gb)
{
	setCaption( i18n("Go to page") );
	gb.setFrameStyle( QFrame::Box | QFrame::Sunken );
	gb.setLineWidth( 1 );
	QBoxLayout l1( this, QBoxLayout::LeftToRight, 15 );
	l1.addWidget( &gb );
	QBoxLayout l2( &gb, QBoxLayout::Down, 15, 10 );
	l2.addSpacing(fontMetrics().height());
	l2.addWidget( &ed );
	ed.setFocus();
	connect( &ed, SIGNAL(returnPressed()), SLOT(go()) );
	QBoxLayout l3( QBoxLayout::LeftToRight, 15 );
	l2.addLayout( &l3 );
	l3.addWidget( &ok );
	l3.addWidget( &cancel );
	ok.setText( i18n("Go to") );
	connect( &ok, SIGNAL(clicked()), SLOT(go()) );
	cancel.setText( i18n("Cancel") );
	resize( 300, 150 );
	l1.activate();
	l2.activate();
	setFixedSize(size());
	connect( &cancel, SIGNAL(clicked()), SLOT(reject()) );
}

void kdvi::pageGoto()
{
	PageDialog dlg;
	connect( &dlg, SIGNAL(textEntered(const QString &)), SLOT(pageActivated(const QString &)) );
	dlg.show();
}

void kdvi::toggleMakePK()
{
	makepk = !makepk;
	applyMakePK();
	message( makepk ? i18n("Missing PK-fonts will be generated"):
		i18n("Missing PK-fonts will be logged to 'missfont.log'") );
}

void kdvi::applyMakePK()
{
	if ( makepk == dviwin->makePK() )
		return;
	dviwin->setMakePK( makepk );
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_PK), makepk );
	config->writeEntry( "MakePK", makepk );
}

void kdvi::toggleShowPS()
{
	showPS = !showPS;
	applyShowPS();
	message( showPS ? i18n("Postcsript specials are rendered") :
		i18n("Postscript specials are not rendered") );
}

void kdvi::applyShowPS()
{
	if ( showPS == dviwin->showPS() )
		return;
	dviwin->setShowPS( showPS );
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_PS), showPS );
	config->writeEntry( "ShowPS", showPS );
}

void kdvi::toggleShowMenubar()
{
	hideMenubar = !hideMenubar;
	applyShowMenubar();
	message( hideMenubar ? i18n("No menu bar") : i18n("Menu bar is shown") );
}

void kdvi::applyShowMenubar()
{
	if ( hideMenubar )	
	  menuBar()->hide();
	else			
	  menuBar()->show();
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_MB), !hideMenubar );
	config->writeEntry( "HideMenubar", hideMenubar );

	// calling this will update the child geometries
	updateRects();
}

void kdvi::toggleShowButtons()
{
	hideButtons = !hideButtons;
	applyShowButtons();
	message( hideButtons ? i18n("No button bar") : i18n("Button bar is shown") );
}

void kdvi::applyShowButtons()
{
	enableToolBar( hideButtons ? KToolBar::Hide : KToolBar::Show );
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_BB), !hideButtons );
	config->writeEntry( "HideButtons", hideButtons );
}

void kdvi::toggleVertToolbar()
{
	vertToolbar = !vertToolbar;
	applyVertToolbar();
	message( vertToolbar ? i18n("Tool bar is shown") : i18n("No tool bar") );
}

void kdvi::applyVertToolbar()
{
	if (vertToolbar)
	{
	        toolBar2->show();
	}
	else
	{
                toolBar2->hide();
	}
        
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_TB), vertToolbar );
	config->writeEntry( "VertToolbar", vertToolbar );
}

void kdvi::toggleShowStatusbar()
{
	hideStatusbar = !hideStatusbar;
	applyShowStatusbar();
	message( hideStatusbar ? i18n("No status bar") : i18n("Status bar is shown") );
}

void kdvi::applyShowStatusbar()
{
	enableStatusBar( hideStatusbar ? KStatusBar::Hide : KStatusBar::Show );
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_SB), !hideStatusbar );
	config->writeEntry( "HideStatusbar", hideStatusbar );
}

void kdvi::toggleShowScrollbars()
{
	hideScrollbars = !hideScrollbars;
	applyShowScrollbars();
	message( !hideScrollbars ? i18n("Scroll bars are shown") : i18n("No scroll bars"));
}

void kdvi::applyShowScrollbars()
{
	dviwin->setShowScrollbars( !hideScrollbars );
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_SC), !hideScrollbars );
	config->writeEntry( "HideScrollbars", hideScrollbars );
}

void kdvi::helpContents()
{
	kapp->invokeHTMLHelp("","");
}

void kdvi::pageActivated( const QString & text)
{
	int pg = text.toInt();
	if (dviwin->page() != pg)
		dviwin->gotoPage( pg );
	dviwin->setFocus();
}

void kdvi::setPage( int pg )
{
	QString str;

	if (!pg) pg = dviwin->page();
	if (!pg) return;
	str.setNum( pg );
	str = i18n("Page: ") + str + i18n(" / ") + QString().setNum(dviwin->totalPages());
	statusBar()->changeItem( str, ID_STAT_PAGE );

	if ( marklist )
		marklist->select( pg - 1 );
}

void kdvi::selectShrink( QPoint p )
{
	sndr = sender()->name();
	if (!ssmenu)
	{
		ssmenu = new QPopupMenu;
		ssmenu->setMouseTracking( TRUE );
		connect( ssmenu, SIGNAL(activated(int)),
			 SLOT(selectShrink(int)) );
		QString s;
		for ( int i=1; i<=basedpi/20; i++ )
			ssmenu->insertItem( s.setNum( i ) );
	}
	ssmenu->popup( p - QPoint( 10, 10 ),
		(QString(sndr) == "largeButton" ? largeShrink : smallShrink) - 1 );
}

void kdvi::selectShrink( int id )
{
	if ( QString(sndr) == "largeButton" )
	{
		dviwin->setShrink( id + 1 );
		largeShrink = dviwin->shrink();
		config->writeEntry( "LargeShrink", largeShrink );
		message(i18n("Large text button set to shrink factor ") +
					QString().setNum(largeShrink) );
	}
	else
	{
		dviwin->setShrink( id + 1 );
		smallShrink = dviwin->shrink();
		config->writeEntry( "SmallShrink", smallShrink );
		message(i18n("Small text button set to shrink factor ") +
					QString().setNum(smallShrink) );
	}
}

void kdvi::selectLarge()
{
	dviwin->setShrink(largeShrink);
}

void kdvi::selectSmall()
{
	dviwin->setShrink(smallShrink);
}

void kdvi::shrinkChanged(int s)
{
	QString t = i18n("Shrink: %1").arg( s );
	statusBar()->changeItem( t, ID_STAT_SHRINK);
}

void kdvi::fileChanged()
{
	message( i18n("File reloaded.") );
	if ( dviwin->totalPages() != marklist->count() )
		updateMarklist();
	setPage();
}

void kdvi::showTip( const QString & tip)
{
	message( tip );
}

void kdvi::removeTip( )
{
	message( "" );
}

void kdvi::message( const QString &s )
{
	statusBar()->changeItem( s, ID_STAT_MSG );
}

void kdvi::showPoint( QPoint p )
{
	int x = p.x(), y = p.y();
	float f = dviwin->shrink() * 25.40 / dviwin->resolution();
	x = int(x * f), y = int(y * f);
	QString s;
	s.sprintf( "X:%4d, Y:%4d", x, y );
	statusBar()->changeItem( s, ID_STAT_XY );
}


void kdvi::readConfig()
{
	QString s;
	config = KApplication::kApplication()->config();
	config->setGroup( "kdvi" );

	pannerValue = config->readNumEntry( "Separator" );
	if (!pannerValue)
		config->writeEntry( "Separator", pannerValue = 80 );
	
	showPS = config->readNumEntry( "ShowPS" );

	hideMenubar = config->readNumEntry( "HideMenubar" );
	hideButtons = config->readNumEntry( "HideButtons" );
	vertToolbar = config->readNumEntry( "VertToolbar" );
	hideStatusbar = config->readNumEntry( "HideStatusbar" );
	hideScrollbars = config->readNumEntry( "HideScrollbars" );

	smallShrink = config->readNumEntry( "SmallShrink" );
	if (!smallShrink)
		config->writeEntry( "SmallShrink", smallShrink = 7 );

	largeShrink = config->readNumEntry( "LargeShrink" );
	if (!largeShrink)
		config->writeEntry( "LargeShrink", largeShrink = 3 );

	int width = config->readNumEntry( "Width" );
	if (!width)
		width = 500;

	int height = config->readNumEntry( "Height" );
	if (!height)
		height = 450;
	resize( width, height );
}


void kdvi::dropEvent( QDropEvent * event )
{
    QStrList list;
    QUriDrag::decode(event, list);
    
    const char *s;

    for ( s = list.first(); s != 0L; s = list.next() )
    {
	// Load the first file in this window
	if ( s == list.getFirst() )
	{
	    QString n = s;
	    if ( n.left(5) == "file:"  )
		openFile( n.mid( 5, n.length() ) );
	}
	else
	{
	    QString n = s;
	    if ( n.left(5) == "file:"  )
		newWindow( n.mid( 5, n.length() ) );
	}
    }
}

void kdvi::saveProperties(KConfig *config )
{
	config->writeEntry( "FileName", dviName );
	config->writeEntry( "Page", dviwin->page() );
	config->writeEntry( "Shrink", dviwin->shrink() );
	config->writeEntry( "Pos.x", dviwin->currentPos().x() );
	config->writeEntry( "Pos.y", dviwin->currentPos().y() );
}


void kdvi::readProperties(KConfig *config)
{
	QString file = config->readEntry("FileName");
	if ( file.isNull() )
		return;
	openFile( file );
	int page = config->readNumEntry( "Page", 1 );
	dviwin->gotoPage( page );
	setPage( page );
	dviwin->setShrink( config->readNumEntry( "Shrink" ) );
	dviwin->scroll( QPoint( config->readNumEntry( "Pos.x" ),
				config->readNumEntry( "Pos.y" ) ) );
}




