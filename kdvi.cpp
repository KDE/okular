//
// Application: kdvi
//
// Previewer for TeX DVI files.
//

#include <qlayout.h>
#include <qdir.h>
#include <qfiledlg.h>
#include <qpixmap.h>
#include <qpushbt.h>
#include <qtooltip.h>
#include <qapp.h>
#include <kmenubar.h>
#include <qmsgbox.h>
#include <qgrpbox.h>
#include <qfileinf.h>
#include <qkeycode.h>
#include <qlined.h>
#include <qframe.h>
#include <kapp.h>
#include <kpanner.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kprocess.h>

#include "kdvi.h"
#include "scrbox.h"
#include "print.h"
#include "pushbutton.h"
#include "prefs.h"
#include "version.h"

#include <unistd.h>
#include <signal.h>

#define translate klocale->translate

enum {ID_STAT_SHRINK, ID_STAT_PAGE, ID_STAT_MSG, ID_STAT_XY};
enum {ID_OPT_PK = 3, ID_OPT_PS, ID_OPT_MB, ID_OPT_BB, ID_OPT_TB, ID_OPT_SB, ID_OPT_SC };

kdvi::kdvi( char *fname, QWidget *, const char *name )
	: KTopLevelWidget( name )
{
	msg = NULL;
	ssmenu = NULL;
	hbl = NULL;
	prefs = NULL;

	readConfig();
	setMinimumSize( 400, 60 );
	setCaption( kapp->getCaption() );
	tipgroup = new QToolTipGroup( this, "TipGroup" );
	connect( tipgroup, SIGNAL(showTip(const char *)), SLOT(showTip(const char *)) );
	connect( tipgroup, SIGNAL(removeTip()), SLOT(removeTip()) );

  // Create KPanner for toolBar2 and dviwindow
  
  	kpan = new KPanner( this, "panner",
	  			KPanner::O_VERTICAL|KPanner::U_ABSOLUTE, 100);
	setView( kpan, TRUE );
	setFrameBorderWidth( 4 );
	kpan->setAbsSeparator( pannerValue );
	connect( kpan, SIGNAL(positionChanged()), SLOT(pannerChanged()) );

  // Create a dvi window

	dviwin = new dviWindow( basedpi, mfmode, paper, makepk,
				kpan->child1(), "dviWindow" );
	connect( dviwin, SIGNAL(currentPage(int)), SLOT(setPage(int)) );
	connect( dviwin, SIGNAL(shrinkChanged(int)), SLOT(shrinkChanged(int)) );
	connect( dviwin, SIGNAL(fileChanged()), SLOT(fileChanged()) );
	connect( dviwin, SIGNAL(statusChange(const char *)),
			 SLOT(showTip(const char *)) );
	connect( dviwin, SIGNAL(setPoint(QPoint)), SLOT(showPoint(QPoint)) );

  // Create a menubar

	menuBar = NULL;
	makeMenuBar();

  // Create toolbars

	toolBar = NULL;
	makeButtons();
	makeToolBar2( kpan->child0() );

  // Create a statusbar

	statusBar = NULL;
	makeStatusBar( translate("No document") );

  // Lay out widgets

	QBoxLayout *l;
	l = new QBoxLayout( kpan->child0(), QBoxLayout::LeftToRight );
	l->addWidget( toolBar2 );
	l->activate();
	l = new QBoxLayout( kpan->child1(), QBoxLayout::LeftToRight );
	l->addWidget( dviwin );
	l->activate();

  // Create RMB menu

	rmbmenu = new QPopupMenu;
	rmbmenu->setMouseTracking( TRUE );
	rmbmenu->connectItem( rmbmenu->insertItem("Toggle Menubar"),
		      this, SLOT(toggleShowMenubar()) );
	rmbmenu->connectItem( rmbmenu->insertItem("Mark page"),
			marklist, SLOT(markSelected()) );
	rmbmenu->connectItem( rmbmenu->insertItem("Redraw"),
			dviwin, SLOT(drawPage()) );
	rmbmenu->connectItem( rmbmenu->insertItem("Preferences ..."),
			this, SLOT(optionsPreferences()) );

  // Bind keys

	bindKeys();
	updateMenuAccel();

 // Drag and drop

	KDNDDropZone * dropZone = new KDNDDropZone( this , DndURL);
	connect( dropZone, SIGNAL( dropAction( KDNDDropZone *) ),
		 SLOT( dropEvent( KDNDDropZone *) ) );

  // Read config options

	applyPreferences();

	selectSmall();
	dviwin->installEventFilter( this );

	message( "" );
	openFile(QString(fname));
}

kdvi::~kdvi()
{
 	if ( toolBar ) delete toolBar;
 	if ( statusBar ) delete statusBar;
 	if ( menuBar ) delete menuBar;
}

// This avoids seg fault at destructor:

void kdvi::closeEvent( QCloseEvent *e )
{
 	if ( toolBar ) delete toolBar;
 	toolBar = NULL;
 	if ( statusBar ) delete statusBar;
 	statusBar = NULL;
 	if ( menuBar ) delete menuBar;
 	menuBar = NULL;
	e->accept();
}

static QPopupMenu *m_f, *m_v, *m_p, *m_o, *m_h;
static int m_fn, m_fo, m_fr, m_fp, m_fx, m_vi, m_vo, m_vf, m_vw, m_vr, m_pp, m_pn, m_pf, m_pl, m_pg,
	m_op, m_ok, m_of, m_o0, m_om, m_ob, m_ot, m_os, m_ol,  m_hc; // , m_ha, m_hq;
	
void kdvi::makeMenuBar()
{
	if (menuBar) delete menuBar;
	menuBar = new KMenuBar( this );
	CHECK_PTR( menuBar );

	QPopupMenu *p = new QPopupMenu;
	CHECK_PTR( p );

	m_fn = p->insertItem( translate("&New"),	this, SLOT(fileNew()) );
	m_fo = p->insertItem( translate("&Open ..."),	this, SLOT(fileOpen()) );

	recentmenu = new QPopupMenu;
	CHECK_PTR( recentmenu );

	connect( recentmenu, SIGNAL(activated(int)), SLOT(openRecent(int)) );

	m_fr = p->insertItem( translate("Open &recent"),	recentmenu );
	
	m_fp = p->insertItem( translate("&Print ..."),	this, SLOT(filePrint()));
	m_fx = p->insertItem( translate("E&xit"), this, SLOT(fileExit()));

	m_f = p;
	menuBar->insertItem( translate("&File"), p, -2 );

	p = new QPopupMenu;
	CHECK_PTR( p );
	m_vi = p->insertItem( translate("Zoom &in"),	dviwin, SLOT(prevShrink()) );
	m_vo = p->insertItem( translate("Zoom &out"),	dviwin, SLOT(nextShrink()) );
	m_vf = p->insertItem( translate("&Fit to page"),	this, SLOT(viewFitPage()) );
	m_vw = p->insertItem( translate("Fit to page &width"),	this, SLOT(viewFitPageWidth()));
	p->insertSeparator();
	m_vr = p->insertItem( translate("&Redraw page"),	dviwin, SLOT(drawPage()) );

	m_v = p;
	menuBar->insertItem( translate("&View"), p, -2 );

	p = new QPopupMenu;
	CHECK_PTR( p );
	m_pp = p->insertItem( translate("&Previous"),	dviwin, SLOT(prevPage()) );
	m_pn = p->insertItem( translate("&Next"),		dviwin, SLOT(nextPage()) );
	m_pf = p->insertItem( translate("&First"),	dviwin, SLOT(firstPage()) );
	m_pl = p->insertItem( translate("&Last"),		dviwin, SLOT(lastPage()) );
	m_pg = p->insertItem( translate("&Go to ..."),	this,   SLOT(pageGoto()) );

	m_p = p;
	menuBar->insertItem( translate("&Page"), p, -2 );

	p = new QPopupMenu;
	CHECK_PTR( p );
	p->setCheckable( TRUE );
	m_op = p->insertItem( translate("&Preferences ..."), this, SLOT(optionsPreferences()));
	m_ok = p->insertItem( translate("&Keys ..."), this, SLOT(configKeys()));
	p->insertSeparator();
	m_of = p->insertItem( translate("Make PK-&fonts"), this, SLOT(toggleMakePK()) );
	p->setItemChecked( m_of, makepk );
	m_o0 = p->insertItem( translate("Show PS"), this, SLOT(toggleShowPS()));
	p->setItemChecked( m_o0, showPS );
	m_om = p->insertItem( translate("Show &Menubar"), this, SLOT(toggleShowMenubar()) );
	p->setItemChecked( m_om, !hideMenubar );
	m_ob = p->insertItem( translate("Show &Buttons"), this, SLOT(toggleShowButtons()) );
	p->setItemChecked( m_ob, !hideButtons );
	m_ot = p->insertItem( translate("Show Page Lis&t"), this, SLOT(toggleVertToolbar()) );
	p->setItemChecked( m_ol, vertToolbar );
	m_os = p->insertItem( translate("Show &Statusbar"), this, SLOT(toggleShowStatusbar()) );
	p->setItemChecked( m_os, !hideStatusbar );
	m_ol = p->insertItem( translate("Show Scro&llbars"), this, SLOT(toggleShowScrollbars()) );
	p->setItemChecked( m_ol, !hideScrollbars );

	m_o = p;
	menuBar->insertItem( translate("&Options"), p, -2 );
	optionsmenu = p;

	menuBar->insertSeparator();

	QPopupMenu *help = kapp->getHelpMenu(true, QString("DVI Viewer")
                                         + " " + KDVI_VERSION
                                         + "\n\nby Markku Hihnala"
                                         + " (mah@ee.oulu.fi)");

	m_h = p;
	menuBar->insertItem( translate("&Help"), help );
	if ( hideMenubar )	menuBar->hide();
	setMenu( menuBar );
}

void kdvi::makeButtons()
{
	QPixmap pm;

	if ( toolBar ) delete toolBar;

	toolBar = new KToolBar( this );
  
#define I(f,o,s,h) toolBar->insertButton( kapp->getIconLoader()->loadIcon(f),\
	 0, SIGNAL(clicked()), o, SLOT(s()), TRUE, h);

	I( "fileopen.xpm",	this,	fileOpen,	translate("Open document ...") )
	I( "reload.xpm",	dviwin,	drawPage,	translate("Reload document") )
	I( "fileprint.xpm",	this,	filePrint,	translate("Print ...") )
	toolBar->insertSeparator();
	I( "start.xpm",		dviwin,	firstPage, 	translate("Go to first page") )
	I( "back.xpm",		dviwin,	prevPage,	translate("Go to previous page") )
	I( "forwpage.xpm",	dviwin,	goForward,	translate("Go down then top of next page") )
	I( "forward.xpm",	dviwin,	nextPage,	translate("Go to next page") )
	I( "finish.xpm",	dviwin,	lastPage,	translate("Go to last page") )
	toolBar->insertSeparator();
	I( "viewmag-.xpm",	dviwin,	nextShrink,	translate("Decrease magnification") )
	I( "smalltext.xpm",	this,	selectSmall,	translate("Small text") )
	I( "largetext.xpm",	this,	selectLarge,	translate("Large text") )
	I( "viewmag+.xpm",	dviwin,	prevShrink,	translate("Increase magnification") )
#undef	I

	addToolBar( toolBar );
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
	QToolTip::add( sbox, 0, tipgroup, translate("Scroll window and switch the page") );
	sbox->setFixedSize(70,80);
	gl->addWidget( sbox );

  // Create a MarkList

	marklist = new MarkList( toolBar2 );
	connect( marklist, SIGNAL(selected(const char *)),
		SLOT(pageActivated(const char *)) );
	QToolTip::add( marklist, 0, tipgroup, translate("Select page and mark pages for printing") );

	gl->addWidget( marklist );
	gl->activate();

	sbox->setPageSize( dviwin->pageSize() );
}

void kdvi::makeStatusBar( QString )
{
	QPixmap pm;

	if ( statusBar ) delete statusBar;
	statusBar = new KStatusBar( this );
	statusBar->setInsertOrder( KStatusBar::RightToLeft );
	statusBar->insertItem((char*)translate("X:0000, Y:0000 "), ID_STAT_XY);
	statusBar->changeItem("", ID_STAT_XY);
	statusBar->insertItem((char*)translate("Shrink: xx"), ID_STAT_SHRINK);
	statusBar->changeItem("", ID_STAT_SHRINK);
	statusBar->insertItem((char*)translate("Page: xxxx / xxxx"), ID_STAT_PAGE);
	statusBar->changeItem("", ID_STAT_PAGE);
	statusBar->insertItem("", ID_STAT_MSG);

	setStatusBar( statusBar );
}

#include <kkeyconf.h>
#include <kstdaccel.h>

void kdvi::bindKeys()
{
	kKeys->registerWidget("kdvi", this);

#define AKCF(f,k,o,s)	kKeys->addKey(f, k);\
			kKeys->connectFunction( "kdvi", f, o, SLOT(s()));

	AKCF(translate("New window"),	"CTRL+N",	this,	fileNew		)
	AKCF(translate("Open file"),	"CTRL+O",	this,	fileOpen	)
	AKCF(translate("Print dialog"),	"CTRL+P",	this,	filePrint	)
	AKCF(translate("Quit"),		"CTRL+Q",	qApp,	quit		)
	AKCF(translate("Zoom in"),	"Plus",		dviwin,	prevShrink	)
	AKCF(translate("Zoom out"),	"Minus",	dviwin,	nextShrink	)
	AKCF(translate("Fit window"),	"Asterisk",	this,	viewFitPage	)
	AKCF(translate("Fit width"),	"Slash",	this,	viewFitPageWidth)
	AKCF(translate("Redraw page"),	"CTRL+R",	dviwin,	drawPage	)
	AKCF(translate("Next page"),	"Next",		dviwin,	nextPage	)
	AKCF(translate("Previous page"),"Prior",	dviwin,	prevPage	)
	AKCF(translate("Last page"),	"CTRL+Next",	dviwin,	lastPage	)
	AKCF(translate("First page"),	"CTRL+Prior",	dviwin,	firstPage	)
	AKCF(translate("Goto page"),	"CTRL+G",	this,	pageGoto	)
	AKCF(translate("Configure keys"),"CTRL+K",	this,	configKeys	)
	AKCF(translate("Toggle show PS"),"CTRL+V",	this,	toggleShowPS	)
	AKCF(translate("Toggle menu bar"),"CTRL+M",	this,	toggleShowMenubar)
	AKCF(translate("Toggle button bar"),"CTRL+B",	this,	toggleShowButtons)
	AKCF(translate("Toggle page list"),"CTRL+T",	this,	toggleVertToolbar)
	AKCF(translate("Toggle status bar"),"CTRL+S",	this,	toggleShowStatusbar)
	AKCF(translate("Toggle scroll bars"),"CTRL+L",	this,	toggleShowScrollbars)
	AKCF(translate("Help"),		"F1",		this,	helpContents	)

#undef AKCF

	config->setGroup( "kdvi" );
}

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


void kdvi::updateMenuAccel()
{
	changeMenuAccel( m_f, m_fn, translate("New window") );
	changeMenuAccel( m_f, m_fo, translate("Open file") );
	changeMenuAccel( m_f, m_fp, translate("Print dialog") );
	changeMenuAccel( m_f, m_fx, translate("Quit") );
	changeMenuAccel( m_v, m_vi, translate("Zoom in") );
	changeMenuAccel( m_v, m_vo, translate("Zoom out") );
	changeMenuAccel( m_v, m_vf, translate("Fit window") );
	changeMenuAccel( m_v, m_vw, translate("Fit width") );
	changeMenuAccel( m_v, m_vr, translate("Redraw page") );
	changeMenuAccel( m_p, m_pp, translate("Previous page") );
	changeMenuAccel( m_p, m_pn, translate("Next page") );
	changeMenuAccel( m_p, m_pf, translate("First page") );
	changeMenuAccel( m_p, m_pl, translate("Last page") );
	changeMenuAccel( m_p, m_pg, translate("Goto page") );
	changeMenuAccel( m_o, m_o0, translate("Toggle show PS") );
	changeMenuAccel( m_o, m_om, translate("Toggle menu bar") );
	changeMenuAccel( m_o, m_ob, translate("Toggle button bar") );
	changeMenuAccel( m_o, m_ot, translate("Toggle page list") );
	changeMenuAccel( m_o, m_os, translate("Toggle status bar") );
	changeMenuAccel( m_o, m_ol, translate("Toggle scroll bars") );
	changeMenuAccel( m_h, m_hc, translate("Help") );
}

void kdvi::configKeys()
{
	kKeys->configureKeys(this);
	updateMenuAccel();
	config->setGroup( "kdvi" );
}

void kdvi::resizeEvent( QResizeEvent* e )
{
	KTopLevelWidget::resizeEvent( e );
	
	config->setGroup( "kdvi" );
	config->writeEntry( "Width", width() );
	config->writeEntry( "Height", height() );
}

bool kdvi::eventFilter( QObject *obj , QEvent *e )
{
	if ( obj != dviwin || e->type() != Event_MouseButtonPress )
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

	message( translate("File open dialog is open") );
	QString f = QFileDialog::getOpenFileName( dir, "*.dvi", this );
	message( "" );

	openFile( f );
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
	QString oname( name );
	name.detach();
	if ( ! QFileInfo( name ).isReadable() )
		name.append( ".dvi" );
	if ( ! QFileInfo( name ).isReadable() )
	{
		QMessageBox::information( this, translate("Notice"),
				QString(translate("Can't read file:\n")) +
				oname, translate("OK"));
		return;
	}
	QDir::setCurrent( QFileInfo( name ).dirPath() );
	dviName = name.copy();
	message( translate("Opening ") + name + " ...");

	makeStatusBar( dviName );
	applyShowStatusbar();
	dviwin->setFile( name );
	dviwin->repaint();
	setCaption( QString()+kapp->getCaption()+": "+name );
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
	message( translate("Opened ") + name );
}

void kdvi::filePrint()
{
	if (!dviName)
		return;

	print * printdlg = new print( this, "printdlg" );

	message( translate("Print dialog is open") );
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

	message( translate("View size fits page") );
}

void kdvi::viewFitPageWidth()
{
	QSize s = rect().size();
	resize( s.width() + dviwin->pageSize().width() - dviwin->viewSize().width(),
		s.height() );

	message( translate("View width fits page") );
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

	message(translate("Preferences applied"));
}

PageDialog::PageDialog() : QDialog( 0, 0, 1 ),
	gb(translate(" Page "),this), ed(&gb), ok(&gb), cancel(&gb)
{
	setCaption( translate("Go to page") );
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
	ok.setText( translate("Go to") );
	connect( &ok, SIGNAL(clicked()), SLOT(go()) );
	cancel.setText( translate("Cancel") );
	resize( 300, 150 );
	l1.activate();
	l2.activate();
	setFixedSize(size());
	connect( &cancel, SIGNAL(clicked()), SLOT(reject()) );
}

void kdvi::pageGoto()
{
	PageDialog dlg;
	connect( &dlg, SIGNAL(textEntered(const char *)), SLOT(pageActivated(const char *)) );
	dlg.show();
}

void kdvi::toggleMakePK()
{
	makepk = !makepk;
	applyMakePK();
	message( makepk ? translate("Missing PK-fonts will be generated"):
		translate("Missing PK-fonts will be logged to 'missfont.log'") );
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
	message( showPS ? translate("Postcsript specials are rendered") :
		translate("Postscript specials are not rendered") );
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
	message( hideMenubar ? translate("No menu bar") : translate("Menu bar is shown") );
}

void kdvi::applyShowMenubar()
{
	if ( hideMenubar )	menuBar->hide();
	else			menuBar->show();
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_MB), !hideMenubar );
	config->writeEntry( "HideMenubar", hideMenubar );

	// calling this will update the child geometries
	updateRects();
}

void kdvi::toggleShowButtons()
{
	hideButtons = !hideButtons;
	applyShowButtons();
	message( hideButtons ? translate("No button bar") : translate("Button bar is shown") );
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
	message( vertToolbar ? translate("Tool bar is shown") : translate("No tool bar") );
}

void kdvi::applyVertToolbar()
{
	if (vertToolbar)
	{
		kpan->setLimits(0,0);
		kpan->setAbsSeparator(pannerValue);
	}
	else
	{
		if (kpan->getAbsSeparator())
			kpan->setAbsSeparator(0);
		kpan->setLimits(0,1);
	}
	optionsmenu->setItemChecked( optionsmenu->idAt(ID_OPT_TB), vertToolbar );
	config->writeEntry( "VertToolbar", vertToolbar );
}

void kdvi::toggleShowStatusbar()
{
	hideStatusbar = !hideStatusbar;
	applyShowStatusbar();
	message( hideStatusbar ? translate("No status bar") : translate("Status bar is shown") );
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
	message( !hideScrollbars ? translate("Scroll bars are shown") : translate("No scroll bars"));
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
/*
void kdvi::helpAbout()
{
	QMessageBox::information( this, translate("About Kdvi"),
		translate("kdvi - TeX DVI viewer\nVersion ")
			+ QString( KDVI_VERSION )
			+ translate("\n\nMarkku Hihnala <mah@ee.oulu.fi>"),
		translate("OK") );
}

void kdvi::helpAboutQt()
{
	QMessageBox::aboutQt( this, "About Qt" );
}
*/

void kdvi::pannerChanged()
{
	int i = kpan->getAbsSeparator();
	if ( i>1 )
		config->writeEntry( "Separator", pannerValue = i );
	else	
	{
		vertToolbar = 0;
		applyVertToolbar();
	}
}

void kdvi::pageActivated( const char * text)
{
	int pg = QString(text).toInt();
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
	str = translate("Page: ") + str + translate(" / ") + QString().setNum(dviwin->totalPages());
	statusBar->changeItem( str, ID_STAT_PAGE );

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
		message(translate("Large text button set to shrink factor ") +
					QString().setNum(largeShrink) );
	}
	else
	{
		dviwin->setShrink( id + 1 );
		smallShrink = dviwin->shrink();
		config->writeEntry( "SmallShrink", smallShrink );
		message(translate("Small text button set to shrink factor ") +
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
	QString t;
	t = "Shrink: " + t.setNum( s );
	statusBar->changeItem( t, ID_STAT_SHRINK);
}

void kdvi::fileChanged()
{
	message( translate("File reloaded") );
	if ( dviwin->totalPages() != marklist->count() )
		updateMarklist();
	setPage();
}

void kdvi::showTip( const char * tip)
{
	message( tip );
}

void kdvi::removeTip( )
{
	message( "" );
}

void kdvi::message( const QString &s )
{
	statusBar->changeItem( s, ID_STAT_MSG );
}

void kdvi::showPoint( QPoint p )
{
	int x = p.x(), y = p.y();
	float f = dviwin->shrink() * 25.40 / dviwin->resolution();
	x = int(x * f), y = int(y * f);
	QString s;
	s.sprintf( "X:%4d, Y:%4d", x, y );
	statusBar->changeItem( s, ID_STAT_XY );
}


void kdvi::readConfig()
{
	QString s;
	kdedir = KApplication::kdedir();
	config = KApplication::getKApplication()->getConfig();
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


void kdvi::dropEvent( KDNDDropZone * dropZone )
{
    QStrList & list = dropZone->getURLList();
    
    char *s;

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

static void getOptions( int& ac, char** av )
{
	if ( ac > 2 && av[1] == QString("-paper") )
		av[1] = av[3], ac -= 2;
}


int main( int argc, char **argv )
{
	KApplication a( argc, argv, "kdvi" );
	kdvi *k;

	getOptions( argc, argv );
	
	if ( a.isRestored() && KTopLevelWidget::canBeRestored( 1 ) )
	{
		k = new kdvi();
		k->restore( 1 );
	}
	else
		k = argc==2 ? new kdvi(argv[1]) : new kdvi();

	a.setMainWidget( k );
	k->show();
	
	kapp->getConfig()->setGroup("kdvi");

	return a.exec();
}
