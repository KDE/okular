//
// Class: dviWindow
//
// Previewer for TeX DVI files.
//

#include <stdlib.h>
#include <unistd.h>

#include <qpainter.h>
#include <qbitmap.h> 
#include <qkeycode.h>
#include <qpaintdevice.h>
#include <qfileinfo.h>

#include <kapp.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>

#include "dviwin.h"
#include "optiondialog.h"


//------ some definitions from xdvi ----------


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

struct	WindowRec {
	Window		win;
	double		shrinkfactor;
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


extern char *           prog;
extern char *	        dvi_name;
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

void 	draw_page(void);
extern "C" void 	kpse_set_progname(const char*);
extern Boolean check_dvi_file(void);
void 	reset_fonts();
void 	init_page();
void 	psp_destroy();
void 	psp_toggle();
void 	psp_interrupt();
extern "C" {
#undef PACKAGE // defined by both c-auto.h and config.h
#undef VERSION
#include <kpathsea/c-auto.h>
#include <kpathsea/paths.h>
#include <kpathsea/proginit.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/tex-glyph.h>
}

//------ next the drawing functions called from C-code (dvi_draw.c) ----

QPainter *dcp;

extern  void qt_processEvents(void)
{
	qApp->processEvents();
}

//------ now comes the dviWindow class implementation ----------

dviWindow::dviWindow( int bdpi, int zoom, const char *mfm, const char *ppr, int mkpk, QWidget *parent, const char *name ) : QScrollView( parent, name )
{
	ChangesPossible = 1;
	FontPath = QString::null;
	viewport()->setBackgroundColor( white );
	setFocusPolicy(QWidget::StrongFocus);
	setFocus();

	setHScrollBarMode(QScrollView::AlwaysOff);
	setVScrollBarMode(QScrollView::AlwaysOff);

        connect(this, SIGNAL(contentsMoving(int, int)),
                this, SLOT(contentsMoving(int, int)));
        
	// initialize the dvi machinery

	setResolution( bdpi );
	setMakePK( mkpk );
	setMetafontMode( mfm );
	setPaper( ppr );

	DISP = x11Display();
	mainwin = handle();
	mane = currwin;
	SCRN = DefaultScreenOfDisplay(DISP);
	_fore_Pixel = BlackPixelOfScreen(SCRN);
	_back_Pixel = WhitePixelOfScreen(SCRN);
	useGS = 1;
	_postscript = 0;
	pixmap = NULL;

	double xres = ((double)(DisplayWidth(DISP,(int)DefaultScreen(DISP)) *25.4)/DisplayWidthMM(DISP,(int)DefaultScreen(DISP)) ); //@@@
	double s    = (basedpi * 100)/(xres*(double)zoom);
	mane.shrinkfactor = currwin.shrinkfactor = s;
}

dviWindow::~dviWindow()
{
	psp_destroy();
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
		KMessageBox::sorry( this,
			i18n("The change in font generation will be effective\n"
			"only after you start kdvi again!") );
	makepk = flag;
}

int dviWindow::makePK()
{
	return makepk;	
}
	
void dviWindow::setFontPath( const char *s )
{
	if (!ChangesPossible)
		KMessageBox::sorry( this,
			i18n("The change in font path will be effective\n"
			"only after you start kdvi again!"));
	FontPath = s;
}

const char * dviWindow::fontPath()
{
	return FontPath;
}

void dviWindow::setMetafontMode( const char *mfm )
{
	if (!ChangesPossible)
		KMessageBox::sorry( this,
			i18n("The change in Metafont mode will be effective\n"
			"only after you start kdvi again!") );
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
	if (!OptionDialog::paperSizes( paper, w, h ))
	{
		kDebugWarning( 4300, "Unknown paper type!");
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
		KMessageBox::sorry( this,
			i18n("The change in resolution will be effective\n"
			"only after you start kdvi again!") );
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
		KMessageBox::sorry( this,
			i18n("The change in gamma will be effective\n"
			"only after you start kdvi again!"), i18n( "OK" ) );
		return;	// Qt will kill us otherwise ??
	}
	_gamma = gamma;
}

float dviWindow::gamma()
{
	return _gamma;
}


//------ reimplement virtual event handlers -------------
void dviWindow::viewportMousePressEvent ( QMouseEvent *e)
{
        if (!(e->button()&LeftButton))
	{
	  QScrollView::viewportMousePressEvent(e);
	  return;
	}
	mouse = e->pos();
        emit setPoint( viewportToContents(mouse) );
}


void dviWindow::viewportMouseMoveEvent ( QMouseEvent *e)
{
	if (!(e->state()&LeftButton))
		return;
        QPoint diff = mouse - e->pos();
	mouse = e->pos();
        scrollBy(diff.x(), diff.y());
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
	case Key_Plus:	zoomIn();				break;
	case Key_Minus:	zoomOut();				break;
        case Key_Down:	scrollBy(0,speed);              	break;
	case Key_Up:	scrollBy(0,-speed);             	break;
	case Key_Right:	scrollBy(speed,0);              	break;
	case Key_Left:	scrollBy(-speed,0);                     break;
	case Key_Home:	
		if (e->state() == ControlButton)
			firstPage();
		else
			setContentsPos(0,0);
		break;
	case Key_End:
		if (e->state() == ControlButton)
			lastPage();
		else
			setContentsPos(0, contentsHeight()-visibleHeight());
		break;
	default:	e->ignore();				break;
	}
}

void dviWindow::initDVI()
{
        prog = const_cast<char*>("kdvi");
	n_files_left = OPEN_MAX;
	kpse_set_progname ("xdvi");
	kpse_init_prog ("XDVI", basedpi, MetafontMode.data(), "cmr10");
	kpse_set_program_enabled(kpse_any_glyph_format,
				 makepk, kpse_src_client_cnf);
	kpse_format_info[kpse_pk_format].override_path
		= kpse_format_info[kpse_gf_format].override_path
		= kpse_format_info[kpse_any_glyph_format].override_path
		= kpse_format_info[kpse_tfm_format].override_path
		= FontPath.ascii();
	ChangesPossible = 0;
}

#include <setjmp.h>
extern	jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */
extern	char *dvi_oops_msg;
extern	QDateTime dvi_time;

//------ this function calls the dvi interpreter ----------


void dviWindow::drawPage()
{
  psp_interrupt();
  if (filename.isEmpty())	// must call setFile first
    return;
  if (!dvi_name) {			//  dvi file not initialized yet
    QApplication::setOverrideCursor( waitCursor );
    dvi_name = const_cast<char*>(filename.ascii());

    dvi_file = NULL;
    if (setjmp(dvi_env)) {	// dvi_oops called
      dvi_time.setTime_t(0); // force init_dvi_file
      QApplication::restoreOverrideCursor();
      KMessageBox::error( this,
			  i18n("What's this? DVI problem!\n")
			  + dvi_oops_msg);
      return;
    }
    if ( !check_dvi_file() )
      emit fileChanged();
    QApplication::restoreOverrideCursor();
    gotoPage(1);
    changePageSize();
    emit viewSizeChanged( QSize( visibleWidth(),visibleHeight() ));
    return;
  }
  min_x = 0;
  min_y = 0;
  max_x = page_w;
  max_y = page_h;

  if ( !pixmap )
    return;
  if ( !pixmap->paintingActive() ) {
    QPainter paint;
    paint.begin( pixmap );
    QApplication::setOverrideCursor( waitCursor );
    dcp = &paint;
    if (setjmp(dvi_env)) {	// dvi_oops called
      dvi_time.setTime_t(0); // force init_dvi_file
      QApplication::restoreOverrideCursor();
      paint.end();
      KMessageBox::error( this,
			  i18n("What's this? DVI problem!\n") 
			  + dvi_oops_msg);
      return;
    } else {
      if ( !check_dvi_file() )
	emit fileChanged();
      pixmap->fill( white );
      draw_page();
    }
    QApplication::restoreOverrideCursor();
    paint.end();
  }
  resize(pixmap->width(), pixmap->height());
  repaintContents(contentsX(), contentsY(), visibleWidth(), visibleHeight(), FALSE);
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
  char test[4];
  unsigned char trailer[4] = { 0xdf,0xdf,0xdf,0xdf };
  if ( f.readBlock( test, 4 )<4 || strncmp( test, (char *) trailer, 4 ) )
    return FALSE;
  // We suppose now that the dvi file is complete	and OK
  return TRUE;
}


void dviWindow::changePageSize()
{
  if ( pixmap && pixmap->paintingActive() )
    return;
  psp_destroy();
  int old_width = 0;
  if (pixmap) {
    old_width = pixmap->width();
    delete pixmap;
  }
  pixmap = new QPixmap( (int)page_w, (int)page_h );
  pixmap->fill( white );
  emit pageSizeChanged( QSize( page_w, page_h ) );

  // Resize the QScrollview. Be careful to maintain the locical positon 
  // on the ScrollView. This looks kind of complicated, but my experience
  // shows that this is what most users intuitively expect.
  // -- Stefan Kebekus.
  int new_x = 0;
  if ( page_w > visibleWidth() ) {
    if ( old_width < visibleWidth() ) {// First time the Pixmap is wider than the ScrollView?
      // Yes. We center horizonzally by default.
      // Don't ask me why I need to subtract 30. It just works better (with qt 2.1 b4, that is).
      if (contentsWidth() < 10)
	new_x = ( page_w - visibleWidth() ) / 2 - 30;
      else
	new_x = ( page_w - visibleWidth() ) / 2;
    } else
      // Otherwise maintain the logical horizontal position
      // -- that is, the horizontal center of the screen stays where it is.
      new_x = ((contentsX()+visibleWidth()/2)*page_w)/contentsWidth() - visibleWidth()/2;
  }
  // We are not that particular about the vertical position 
  // -- just keep the upper corner where it is.
  int new_y = (contentsY()*page_h) / contentsHeight();

  resizeContents( page_w, page_h );
  setContentsPos( new_x > 0 ? new_x : 0, new_y > 0 ? new_y : 0 );
  currwin.win = mane.win = pixmap->handle();
  drawPage();
}

//------ setup the dvi interpreter (should do more here ?) ----------

void dviWindow::setFile( const char *fname )
{
        if (ChangesPossible){
            initDVI();
        }
        filename = fname;
        dvi_name = 0;
        changetime = QFileInfo(filename).lastModified();
        drawPage();
}

//------ following member functions are in the public interface ----------

QPoint dviWindow::currentPos()
{
	return QPoint(contentsX(), contentsY());
}

void dviWindow::scroll( QPoint to )
{
	setContentsPos(to.x(), to.y());
}

QSize dviWindow::viewSize()
{
	return QSize( visibleWidth(), visibleHeight() );
}

QSize dviWindow::pageSize()
{
	return QSize( page_w, page_h );
}

//------ handling pages ----------

void dviWindow::goForward()
{
  if(contentsY() >= contentsHeight()-visibleHeight()) {
    nextPage();
    // Go to the top of the page, but keep the horizontal position
    setContentsPos(contentsX(), 0);
  }
  else
    scrollBy(0, 2*visibleWidth()/3);
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

// Return the current zoom in percent.
// Zoom = 100% means that the image on the screen has the same
//             size as a printout.

int dviWindow::zoom()
{
  // @@@ We assume that pixels are square, i.e. that vertical and horizontal resolutions
  // agree. Is that a safe assumption?
  double xres = ((double)(DisplayWidth(DISP,(int)DefaultScreen(DISP)) *25.4)/DisplayWidthMM(DISP,(int)DefaultScreen(DISP)) );

  return (int)( (basedpi * 100.0)/(currwin.shrinkfactor*xres) + 0.5);
}

double dviWindow::shrink()
{
  return currwin.shrinkfactor;
}

void dviWindow::zoomOut()
{
  //@@@	setShrink( shrink() - 1 );
  setZoom(zoom()-10);
}

void dviWindow::zoomIn()
{
  //@@@	setShrink( shrink() + 1 );
  setZoom(zoom()+10);
}

void dviWindow::setZoom(int zoom)
{
  if ((zoom < 5) || (zoom > 500))
    zoom = 100;

  double xres = ((double)(DisplayWidth(DISP,(int)DefaultScreen(DISP)) *25.4)/DisplayWidthMM(DISP,(int)DefaultScreen(DISP)) ); //@@@
  double s    = (basedpi * 100)/(xres*(double)zoom);
  mane.shrinkfactor = currwin.shrinkfactor = s;
  init_page();
  reset_fonts();
  changePageSize();
  emit zoomChanged( zoom );
}

void dviWindow::resizeEvent(QResizeEvent *e)
{
  QScrollView::resizeEvent(e);
  emit viewSizeChanged( QSize( visibleWidth(),visibleHeight() ));
}	

void dviWindow::contentsMoving( int x, int y ) 
{
  emit currentPosChanged( QPoint(x, y) );
}


void dviWindow::drawContents(QPainter *p, int clipx, int clipy, int clipw, int cliph ) 
{
  if ( pixmap )
    p->drawPixmap(clipx, clipy, *pixmap, clipx, clipy, clipw, cliph);
}
