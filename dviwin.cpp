//
// Class: dviWindow
//
// Previewer for TeX DVI files.
//

#include "dviwin.h"
#include "prefs.h"
#include <qbitmap.h> 
#include <qkeycode.h>
#include <qpaintd.h>
#include <kapp.h>
#include <qmsgbox.h>
#include <qfileinf.h>
#include <kdebug.h>

#include <klocale.h>

#include <stdlib.h>
#include <unistd.h>

//------ some definitions from xdvi ----------


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

struct	WindowRec {
	Window		win;
	int		shrinkfactor;
	int		base_x, base_y;
	unsigned int	width, height;
	int	min_x, max_x, min_y, max_y;
};

extern	struct WindowRec mane, alt, currwin;

#include "c-openmx.h" // for OPEN_MAX

	float	_gamma;
	int	_pixels_per_inch;
	_Xconst char	*_paper;
	Pixel	_fore_Pixel;
	Pixel	_back_Pixel;
	Boolean	_postscript;
	Boolean	useGS;
	Boolean	_use_grey;

extern char		*prog;
extern char *		dvi_name;
extern FILE *		dvi_file;
extern int 		n_files_left;
extern int 		min_x;
extern int 		min_y;
extern int 		max_x;
extern int 		max_y;
extern int		offset_x, offset_y;
extern unsigned int	unshrunk_paper_w, unshrunk_paper_h;
extern unsigned int	unshrunk_page_w, unshrunk_page_h;
extern unsigned int	page_w, page_h;
extern int 		current_page;
extern int 		total_pages;
extern Display *	DISP;
extern Screen  *	SCRN;
Window mainwin;
int			useAlpha;

extern "C" void 	draw_page(void);
// extern "C" void 	kpse_set_progname(const char*);
// extern "C" void 	kpse_init_prog(const char*, unsigned, const char*, bool, const char*);
extern "C" Boolean 	check_dvi_file();
extern "C" void 	reset_fonts();
extern "C" void 	init_page();
extern "C" void 	init_pix(Boolean warn);
extern "C" void 	psp_destroy();
extern "C" void 	psp_toggle();
extern "C" void 	psp_interrupt();

//------ next the drawing functions called from C-code (dvi_draw.c) ----

QPainter *dcp;

extern "C" void qtPutRule(int x, int y, int w, int h)
{
	dcp->fillRect( x, y, w, h, black );
}

extern "C" void qtPutBorder(int x, int y, int w, int h)
{
	dcp->drawRect( x, y, w, h );
}

extern "C" void qtPutBitmap( int x, int y, int w, int h, uchar *bits )
{
	QBitmap bm( w, h, bits, TRUE );
	dcp->drawPixmap( x, y, bm );
}

extern "C" void qt_processEvents()
{
	qApp->processEvents();
}

//------ now comes the dviWindow class implementation ----------

dviWindow::dviWindow( int bdpi, const char *mfm, const char *ppr,
			int mkpk, QWidget *parent, const char *name )
	: QWidget( parent, name ), block( this )
{
	ChangesPossible = 1;
	showsbs = 1;
	FontPath = QString( 0 );
	setBackgroundColor( white );
	setUpdatesEnabled(FALSE);
	setFocusPolicy(QWidget::StrongFocus);
	setFocus();

	timer = new QTimer( this );
	connect( timer, SIGNAL(timeout()),
		 this, SLOT(timerEvent()) );
	checkinterval = 1000;

  // Create a Vertical scroll bar

	vsb = new QScrollBar( QScrollBar::Vertical,this,"scrollBar" );
	connect( vsb, SIGNAL(valueChanged(int)), SLOT(scrollVert(int)) );

  // Create a Horizontal scroll bar

	hsb = new QScrollBar( QScrollBar::Horizontal,this,"scrollBar" );
	connect( hsb, SIGNAL(valueChanged(int)), SLOT(scrollHorz(int)) );

	// initialize the dvi machinery

	setResolution( bdpi );
	setMakePK( mkpk );
	setMetafontMode( mfm );
	setPaper( ppr );

	DISP = x11Display();
	mainwin = handle();
	currwin.shrinkfactor = 6;
	mane = currwin;
	SCRN = DefaultScreenOfDisplay(DISP);
	_fore_Pixel = BlackPixelOfScreen(SCRN);
	_back_Pixel = WhitePixelOfScreen(SCRN);
	useGS = 1;
	_postscript = 0;
	_use_grey = 1;
	pixmap = NULL;
	init_pix(FALSE);
}

dviWindow::~dviWindow()
{
	psp_destroy();
}

void dviWindow::setChecking( int time )
{
	checkinterval = time;
	if ( timer->isActive() )
		timer->changeInterval( time );
}

int dviWindow::checking()
{
	return checkinterval;
}

void dviWindow::setShowScrollbars( int flag )
{
	if ( showsbs == flag )
		return;
	showsbs = flag;
	setChildrenGeometries();
}

int dviWindow::showScrollbars()
{
	return showsbs;
}

void dviWindow::setShowPS( int flag )
{
	if ( _postscript == flag )
		return;
	_postscript = flag;
	psp_toggle();
	drawPage();
}

int dviWindow::showPS()
{
	return _postscript;
}

void dviWindow::setAntiAlias( int flag )
{
	if ( !useAlpha == !flag )
		return;
	useAlpha = flag;
	psp_destroy();
	drawPage();
}

int dviWindow::antiAlias()
{
	return useAlpha;
}

void dviWindow::setMakePK( int flag )
{
	if (!ChangesPossible)
		QMessageBox::warning( this, i18n("Notice"),
			i18n("The change in font generation will be effective\n"
			"only after you start kdvi again!"), i18n( "OK" ) );
	makepk = flag;
}

int dviWindow::makePK()
{
	return makepk;	
}
	
// extern char * kpse_font_override_path;
extern "C" {
#include <kpathsea/kpathsea.h>
}

void dviWindow::setFontPath( const char *s )
{
	if (!ChangesPossible)
		QMessageBox::warning( this, i18n("Notice"),
			i18n("The change in font path will be effective\n"
			"only after you start kdvi again!"), i18n( "OK" ) );
	FontPath = s;
}

const char * dviWindow::fontPath()
{
	return FontPath;
}

void dviWindow::setMetafontMode( const char *mfm )
{
	if (!ChangesPossible)
		QMessageBox::warning( this, i18n("Notice"),
			i18n("The change in Metafont mode will be effective\n"
			"only after you start kdvi again!"), i18n( "OK" ) );
	MetafontMode = mfm;
}

const char * dviWindow::metafontMode()
{
	return MetafontMode;
}

void dviWindow::setPaper( const char *paper )
{
	if ( !paper )
		return;
	paper_type = paper;
	_paper = paper_type;
	float w, h;
	if (!kdviprefs::paperSizes( paper, w, h ))
	{
		KDEBUG( KDEBUG_WARN, 0, "Unknown paper type!");
		// A4 paper is used as default, if paper is unknown
		w = 21.0/2.54;
		h = 29.7/2.54;
	}
	unshrunk_paper_w = int( w * basedpi + 0.5 );
	unshrunk_paper_h = int( h * basedpi + 0.5 ); 
}

const char * dviWindow::paper()
{
	return paper_type;
}

void dviWindow::setResolution( int bdpi )
{
	if (!ChangesPossible)
		QMessageBox::warning( this, i18n("Notice"),
			i18n("The change in resolution will be effective\n"
			"only after you start kdvi again!"), i18n( "OK" ) );
	basedpi = bdpi;
	_pixels_per_inch = bdpi;
	offset_x = offset_y = bdpi;
}

int dviWindow::resolution()
{
	return basedpi;
}

void dviWindow::setGamma( float gamma )
{
	if (!ChangesPossible)
	{
		QMessageBox::warning( this, i18n("Notice"),
			i18n("The change in gamma will be effective\n"
			"only after you start kdvi again!"), i18n( "OK" ) );
		return;	// Qt will kill us otherwise ??
	}
	_gamma = gamma;
	init_pix(FALSE);
}

float dviWindow::gamma()
{
	return _gamma;
}


//------ reimplement virtual event handlers -------------

void dviWindow::mousePressEvent ( QMouseEvent *e)
{
	if (!(e->button()&LeftButton))
		return;
	mouse = e->pos();
	emit setPoint( mouse + base );
}

void dviWindow::mouseMoveEvent ( QMouseEvent *e)
{
	const int mouseSpeed = 1;

	if (!(e->state()&LeftButton))
		return;
	QPoint diff = mouse - e->pos();
	mouse = e->pos();
	scrollRelative(diff*mouseSpeed);
}

void dviWindow::keyPressEvent ( QKeyEvent *e)
{
	const int slowScrollSpeed = 1;
	const int fastScrollSpeed = 15;
	int speed = e->state() & ControlButton ?
			slowScrollSpeed : fastScrollSpeed;

	switch( e->key() ) {
	case Key_Next:	nextPage();				break;
	case Key_Prior:	prevPage();				break;
	case Key_Space:	goForward();				break;
	case Key_Plus:	prevShrink();				break;
	case Key_Minus:	nextShrink();				break;
	case Key_Down:	scrollRelative(QPoint(0,speed));	break;
	case Key_Up:	scrollRelative(QPoint(0,-speed));	break;
	case Key_Right:	scrollRelative(QPoint(speed,0));	break;
	case Key_Left:	scrollRelative(QPoint(-speed,0));	break;
	case Key_Home:	
		if (e->state() == ControlButton)
			firstPage();
		else
			scrollAbsolute(QPoint(base.x(),0));
		break;
	case Key_End:
		if (e->state() == ControlButton)
			lastPage();
		else
			scrollAbsolute(QPoint(base.x(),page_h));
		break;

	default:	e->ignore();				break;
	}
}

void dviWindow::paintEvent( QPaintEvent *p )
{
	if ( !pixmap )
		return;
	QRect r( 0, 0, hclip, vclip );
	r = r.intersect( p->rect() );

	bitBlt( this, r.x(), r.y(),
		pixmap, base.x() + r.x(), base.y() + r.y(),
		r.width(), r.height() );
}

void dviWindow::resizeEvent ( QResizeEvent *)
{
	if ( !pixmap || ( width() > pixmap->width() )
	     || ( height() > pixmap->height() ) )
		changePageSize();
	setChildrenGeometries( FALSE );
	// The following will ensure that view position is within limits
	scrollRelative( QPoint(0,0) );
}

void dviWindow::initDVI()
{
	prog = "kdvi";
	n_files_left = OPEN_MAX;
	kpse_set_progname ("xdvi");
	kpse_init_prog ("XDVI", basedpi, MetafontMode.data(), "cmr10");
	kpse_set_program_enabled (kpse_any_glyph_format,
				  makepk, kpse_src_client_cnf);
//	*(const_cast<char**>(&kpse_font_override_path)) = FontPath.data();
	kpse_format_info[kpse_pk_format].override_path
		= kpse_format_info[kpse_gf_format].override_path
                = kpse_format_info[kpse_any_glyph_format].override_path
                = kpse_format_info[kpse_tfm_format].override_path
		= FontPath.data();
	ChangesPossible = 0;
}

#include <setjmp.h>
extern	jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */
extern	char *dvi_oops_msg;
extern	time_t dvi_time;

//------ this function calls the dvi interpreter ----------

void dviWindow::drawDVI()
{
	psp_interrupt();
	if (filename.isEmpty())	// must call setFile first
		return;
	if (!dvi_name)
	{			//  dvi file not initialized yet
		QApplication::setOverrideCursor( waitCursor );
		dvi_name = filename.data();

		dvi_file = NULL;
		if (setjmp(dvi_env))
		{	// dvi_oops called
			dvi_time = 0; // force init_dvi_file
			QApplication::restoreOverrideCursor();
			QMessageBox::critical( this, i18n("HEY"),
				QString(i18n("What's this? DVI problem!\n"))
					+ dvi_oops_msg,
				i18n("OK"));
			return;
		}
		if ( !check_dvi_file() )
			emit fileChanged();

		QApplication::restoreOverrideCursor();
		gotoPage(1);
		setChildrenGeometries(FALSE);
		changePageSize();
		emit viewSizeChanged( QSize( hclip, vclip ) );
		timer->start( 1000 );
		return;
	}
	min_x = 0;
	min_y = 0;
	max_x = page_w;
	max_y = page_h;

	if ( !pixmap )	return;
	if ( !pixmap->paintingActive() )
	{
		QPainter paint;
		paint.begin( pixmap );
		QApplication::setOverrideCursor( waitCursor );
		dcp = &paint;
		if (setjmp(dvi_env))
		{	// dvi_oops called
			dvi_time = 0; // force init_dvi_file
			QApplication::restoreOverrideCursor();
			paint.end();
			QMessageBox::critical( this, i18n("HEY"),
				QString(i18n("What's this? DVI problem!\n"))
					+ dvi_oops_msg,
				i18n("OK"));
			return;
		}
		else
		{
			if ( !check_dvi_file() )
				emit fileChanged();
			pixmap->fill( white );
			draw_page();
		}
		QApplication::restoreOverrideCursor();
		paint.end();
	}
}

void dviWindow::drawPage()
{
	drawDVI();
	setUpdatesEnabled(TRUE);repaint(0);setUpdatesEnabled(FALSE);
}

bool dviWindow::changedDVI()
{
	return changetime != QFileInfo(filename).lastModified();
}

bool dviWindow::correctDVI()
{
	QFile f(filename);
	if (!f.open(IO_ReadOnly))
		return FALSE;
	int n = f.size();
	if ( n < 134 )	// Too short for a dvi file
		return FALSE;
	f.at( n-4 );
	char test[4], trailer[4] = { 0xdf,0xdf,0xdf,0xdf };
	if ( f.readBlock( test, 4 )<4 || strncmp( test, trailer, 4 ) )
		return FALSE;
	// We suppose now that the dvi file is complete	and OK
	return TRUE;
}
void dviWindow::timerEvent()
{
	static int changing = 0;
	
	if ( !changedDVI() )
		return;
	if ( !changing )
		emit statusChange( i18n("File status changed.") );
	changing = 1;
	if ( !correctDVI() )
		return;
	changing = 0;
	emit statusChange( i18n("File reloaded.") );
	changetime = QFileInfo(filename).lastModified();
	drawPage();
	emit fileChanged();
}

void dviWindow::changePageSize()
{
	if ( pixmap && pixmap->paintingActive() )
		return;
	psp_destroy();
	if (pixmap)
		delete pixmap;
	pixmap = new QPixmap( QMAX( width(), (int)page_w ),
				QMAX( height(), (int)page_h ) );
	emit pageSizeChanged( QSize( page_w, page_h ) );

	currwin.win = mane.win = pixmap->handle();

	pixmap->fill( white );
	drawPage();
}

//------ setup the dvi interpreter (should do more here ?) ----------

void dviWindow::setFile( const char *fname )
{
	if (ChangesPossible)
		initDVI();
	filename = fname;
	dvi_name = 0;
	changetime = QFileInfo(filename).lastModified();
	drawPage();
}

//------ class private stuff, scrolling----------

#define SBW 16

void dviWindow::setChildrenGeometries(int doupdate)
{
	int oldhc = hclip;
	int oldvc = vclip;

	// deside if scrollbars needed

	hscroll = vscroll = 0;
	if (showsbs)
	{
		if ( width()		< int(page_w) ) hscroll = SBW;
		if ( height() - hscroll < int(page_h) ) vscroll = SBW;
		if ( width()  - vscroll < int(page_w) ) hscroll = SBW;
	}

	// store clip sizes

	hclip = width() - vscroll;
	vclip = height() - hscroll;

	// reconfig scrollbars accordingly

	if (hscroll)
	{
		hsb->show();
		hsb->setGeometry( 0, vclip, hclip, hscroll );
		hsb->setRange( 0, page_w - hclip );
		hsb->setValue( base.x() );
		hsb->setSteps( 15, hclip );
	}
	else
		hsb->hide();

	if (vscroll)
	{
		vsb->show();
		vsb->setGeometry( hclip, 0, vscroll, vclip );
		vsb->setRange( 0, page_h - vclip );
		vsb->setValue( base.y() );
		vsb->setSteps( 15, vclip );
	}
	else
		vsb->hide();

	if (hscroll&&vscroll)
	{
		block.setGeometry( hclip, vclip, SBW, SBW );
		block.show();
	}
	else
		block.hide();
	if (doupdate)
		setUpdatesEnabled(TRUE);repaint(0);setUpdatesEnabled(FALSE);

	if ( oldhc != hclip || oldvc != vclip )
		emit viewSizeChanged( QSize( hclip, vclip ) );
}

void dviWindow::scrollVert( int value )
{
	scrollRelative( QPoint(0,value - base.y()) );
}

void dviWindow::scrollHorz( int value )
{
	scrollRelative( QPoint(value - base.x(),0) );
}

void dviWindow::scrollRelative(const QPoint r, int doupdate)
{
	int maxx = page_w - hclip;
	int maxy = page_h - vclip;

	QPoint old = base;
	base += r;

	if ( maxx < 0 ) maxx = 0;
	if ( maxy < 0 ) maxy = 0;
	if ( base.x() <= 0 )
		base.rx() = 0;
	else if ( base.x() > maxx )
		base.rx() = maxx;
	if ( base.y() <= 0)
		base.ry() = 0;
	else if ( base.y() > maxy )
		base.ry() = maxy;

	if ( (base-old).isNull() )
		return;

	setChildrenGeometries(FALSE);

	if ( doupdate )
	{
		setUpdatesEnabled(TRUE);repaint(0);setUpdatesEnabled(FALSE);
	}
	emit currentPosChanged( base );
}

void dviWindow::scrollAbsolute(const QPoint p)
{
	scrollRelative( p - base );
}

//------ following member functions are in the public interface ----------

QPoint dviWindow::currentPos()
{
	return base;
}

void dviWindow::scroll( QPoint to )
{
	scrollAbsolute( to );
}

QSize dviWindow::viewSize()
{
	return QSize( hclip, vclip );
}

QSize dviWindow::pageSize()
{
	return QSize( page_w, page_h );
}

//------ handling pages ----------

void dviWindow::goForward()
{
	if ( base.y() >= int(page_h) - vclip )
	{
		scrollRelative(QPoint (0,-base.y()), FALSE );
		nextPage();
	}
	else
		scrollRelative(QPoint(0,2*vclip/3));
}

void dviWindow::prevPage()
{
	gotoPage( page()-1 );
}

void dviWindow::nextPage()
{
	gotoPage( page()+1 );
}

void dviWindow::gotoPage(int new_page)
{
	if (new_page<1)
		new_page = 1;
	if (new_page>total_pages)
		new_page = total_pages;
	if (new_page-1==current_page)
		return;
	current_page = new_page-1;
	emit currentPage(new_page);
	drawPage();
}

void dviWindow::firstPage()
{
	gotoPage(1);
}

void dviWindow::lastPage()
{
	gotoPage(totalPages());
}

int dviWindow::page()
{
	return current_page+1;
}

int dviWindow::totalPages()
{
	return total_pages;
}

//------ handling shrink factor ----------

int dviWindow::shrink()
{
	return currwin.shrinkfactor;
}

void dviWindow::prevShrink()
{
	setShrink( shrink() - 1 );
}

void dviWindow::nextShrink()
{
	setShrink( shrink() + 1 );
}

void dviWindow::setShrink(int s)
{
	int olds = shrink();

	if (s<=0 || s>basedpi/20 || olds == s)
		return;
	mane.shrinkfactor = currwin.shrinkfactor = s;
	init_page();
	init_pix(FALSE);
	reset_fonts();
	QPoint newbase = ((base + mouse) * olds - s * mouse) / s;
	setChildrenGeometries();
	changePageSize();
	scrollAbsolute(newbase);
	emit shrinkChanged( shrink() );
}
