//
// Class: dviWindow
//
// Previewer for TeX DVI files.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#include <qpainter.h>
#include <qbitmap.h> 
#include <qkeycode.h>
#include <qpaintdevice.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qurl.h>

#include <kapp.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>

#include "dviwin.h"
#include "fontpool.h"
#include "optiondialog.h"


//------ some definitions from xdvi ----------


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#define	MAXDIM		32767
#define    DVI_BUFFER_LEN  512     
extern struct frame        *current_frame;     
struct WindowRec mane	= {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
struct WindowRec currwin = {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
extern	struct WindowRec alt;
extern unsigned char       dvi_buffer[DVI_BUFFER_LEN];  
struct drawinf	currinf;
fontPool font_pool;

const char *dvi_oops_msg;	/* error message */
double	dimconv;
int	        n_files_left;    	/* for LRU closing of fonts */
unsigned short	current_timestamp = 0;	/* for LRU closing of fonts */
extern struct frame        frame0; /* dummy head of list */  

jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */


QIntDict<font> tn_table;


#include "c-openmx.h" // for OPEN_MAX

int	_pixels_per_inch;
_Xconst char	*_paper;



// The following are really used
long	        magnification;
unsigned int	page_w;
unsigned int	page_h;
// end of "really used"

extern int 		n_files_left;
extern unsigned int	page_w, page_h;
Window                  mainwin;

void 	draw_page(void);

void 	reset_fonts();
#include <setjmp.h>
extern	jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */
double xres;
QPainter foreGroundPaint; // QPainter used for text


//------ now comes the dviWindow class implementation ----------

dviWindow::dviWindow( int bdpi, double zoom, const QString & mfm, int mkpk, QWidget *parent, const char *name ) 
  : QWidget( parent, name )
{
#ifdef DEBUG
  kdDebug() << "dviWindow" << endl;
#endif

  setBackgroundMode(NoBackground);

  setFocusPolicy(QWidget::StrongFocus);
  setFocus();
  
  // initialize the dvi machinery

  dviFile                = NULL;
  setResolution( bdpi );
  setMakePK( mkpk );
  setMetafontMode( mfm );
  unshrunk_paper_w       = int( 21.0 * basedpi/2.54 + 0.5 ); // set A4 paper as default
  unshrunk_paper_h       = int( 27.9 * basedpi/2.54 + 0.5 ); 
  PostScriptOutPutString = NULL;
  HTML_href              = NULL;
  mainwin                = handle();
  mane                   = currwin;
  _postscript            = 0;
  pixmap                 = NULL;

  // Calculate the horizontal resolution of the display device.  @@@
  // We assume implicitly that the horizontal and vertical resolutions
  // agree. This is probably not a safe assumption.
  Display *DISP          = x11Display();
  xres                   = ((double)(DisplayWidth(DISP,(int)DefaultScreen(DISP)) *25.4) /
			    DisplayWidthMM(DISP,(int)DefaultScreen(DISP)) );
  // Just to make sure that we are never dividing by zero.
  if ((xres < 10)||(xres > 1000))
    xres = 75.0;

  // In principle, this method should never be called with illegal
  // values for zoom. In principle.
  if (zoom < KViewPart::minZoom/1000.0)
    zoom = KViewPart::minZoom/1000.0;
  if (zoom > KViewPart::maxZoom/1000.0)
    zoom = KViewPart::maxZoom/1000.0;
  mane.shrinkfactor      = currwin.shrinkfactor = (double)basedpi/(xres*zoom);
  _zoom                  = zoom;

  PS_interface           = new ghostscript_interface(0.0, 0, 0);
  is_current_page_drawn  = 0;  
  n_files_left           = OPEN_MAX;
  resize(0,0);
}

dviWindow::~dviWindow()
{
#ifdef DEBUG
  kdDebug() << "~dviWindow" << endl;
#endif

  delete PS_interface;
  delete dviFile;
}

void dviWindow::setShowPS( int flag )
{
#ifdef DEBUG
  kdDebug() << "setShowPS" << endl;
#endif

  if ( _postscript == flag )
    return;
  _postscript = flag;
  drawPage();
}

void dviWindow::setShowHyperLinks( int flag )
{
  if ( _showHyperLinks == flag )
    return;
  _showHyperLinks = flag;

  drawPage();
}

void dviWindow::setMakePK( int flag )
{
  if (dviFile != NULL)
  KMessageBox::sorry( this,
		      i18n("The change in font generation will be effective\n"
			   "only after you start kdvi again!") );
  makepk = flag;
}

void dviWindow::setMetafontMode( const QString & mfm )
{
  if (dviFile != NULL)
    KMessageBox::sorry( this,
			i18n("The change in Metafont mode will be effective\n"
			     "only after you start kdvi again!") );
  MetafontMode = mfm;
  font_pool.setMetafontMode(mfm);
}


void dviWindow::setPaper(double w, double h)
{
#ifdef DEBUG
  kdDebug() << "setPaper" << endl;
#endif

  unshrunk_paper_w = int( w * basedpi/2.54 + 0.5 );
  unshrunk_paper_h = int( h * basedpi/2.54 + 0.5 ); 
  unshrunk_page_w  = unshrunk_paper_w;
  unshrunk_page_h  = unshrunk_paper_h;
  page_w           = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h           = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;
  reset_fonts();
  changePageSize();
}


void dviWindow::setResolution( int bdpi )
{
  if (dviFile != NULL)
    KMessageBox::sorry( this,
			i18n("The change in resolution will be effective\n"
			     "only after you start kdvi again!") );
  basedpi          = bdpi;
  _pixels_per_inch = bdpi;
  font_pool.setResolution(bdpi);
}


//------ this function calls the dvi interpreter ----------

void dviWindow::drawPage()
{
#ifdef DEBUG
  kdDebug(4300) << "drawPage" << endl;
#endif

  if (dviFile == NULL) {
    resize(0, 0);
    return;
  }
  if ( dviFile->file == NULL) {
    resize(0, 0);
    return;
  }

  if ( !pixmap )
    return;

  if ( !pixmap->paintingActive() ) {

    foreGroundPaint.begin( pixmap );
    QApplication::setOverrideCursor( waitCursor );
    if (setjmp(dvi_env)) {	// dvi_oops called
      QApplication::restoreOverrideCursor();
      foreGroundPaint.end();
      KMessageBox::error( this,
			  i18n("File corruption!\n\n") +
			  QString::fromUtf8(dvi_oops_msg) +
			  i18n("\n\nMost likely this means that the DVI file\nis broken, or that it is not a DVI file."));
      return;
    } else {
      draw_page();
    }
    QApplication::restoreOverrideCursor();
    foreGroundPaint.end();
  }
  resize(pixmap->width(), pixmap->height());
  repaint();

}


bool dviWindow::correctDVI(QString filename)
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

  int old_width = 0;
  if (pixmap) {
    old_width = pixmap->width();
    delete pixmap;
  }
  pixmap = new QPixmap( (int)page_w, (int)page_h );
  pixmap->fill( white );

  resize( page_w, page_h );
  currwin.win = mane.win = pixmap->handle();

  PS_interface->setSize( basedpi/mane.shrinkfactor , page_w, page_h);
  drawPage();
}

//------ setup the dvi interpreter (should do more here ?) ----------

void dviWindow::setFile( const QString & fname )
{
  QFileInfo fi(fname);
  QString   filename = fi.absFilePath();

  // If fname is the empty string, then this means: "close". Delete
  // the dvifile and the pixmap.
  if (fname.isEmpty()) {
    if (dviFile)
      delete dviFile;
    dviFile = 0;
    if (pixmap)
      delete pixmap;
    pixmap = 0;
    resize(0, 0);
    return;
  }

  // Make sure the file actually exists.
  if (!fi.exists() || fi.isDir()) {
    KMessageBox::error( this,
			i18n("File error!\n\n") +
			i18n("The file does not exist\n") + 
			filename);
    return;
  }

  QApplication::setOverrideCursor( waitCursor );
  if (setjmp(dvi_env)) {	// dvi_oops called
    QApplication::restoreOverrideCursor();
    KMessageBox::error( this,
			i18n("File corruption!\n\n") +
			QString::fromUtf8(dvi_oops_msg) +
			i18n("\n\nMost likely this means that the DVI file\n") + 
			filename +
			i18n("\nis broken, or that it is not a DVI file."));
    return;
  }

  dvifile *dviFile_new = new dvifile(filename);
  if (dviFile_new->file == NULL) {
    delete dviFile_new;
    return;
  }

  if (dviFile)
    delete dviFile;
  dviFile = dviFile_new;
  
  page_w = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;

  // Extract PostScript from the DVI file, and store the PostScript
  // specials in PostScriptDirectory, and the headers in the
  // PostScriptHeaderString.
  PS_interface->clear();

  // We will also generate a list of hyperlink-anchors in the
  // document. So declare the existing list empty.
  numAnchors = 0;
  
  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    PostScriptOutPutString = new QString();
    
    (void) lseek(fileno(dviFile->file), dviFile->page_offset[current_page], SEEK_SET);
    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = tn_table;
    currinf.end       = dvi_buffer;
    currinf.pos       = dvi_buffer;
    currinf._virtual  = NULL;
    draw_part(current_frame = &frame0, dimconv);

    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;
  is_current_page_drawn  = 0;

  QApplication::restoreOverrideCursor();
  return;
}



//------ handling pages ----------


void dviWindow::gotoPage(int new_page)
{
  if (dviFile == NULL)
    return;

  if (new_page<1)
    new_page = 1;
  if (new_page > dviFile->total_pages)
    new_page = dviFile->total_pages;
  if ((new_page-1==current_page) &&  !is_current_page_drawn)
    return;
  current_page           = new_page-1;
  is_current_page_drawn  = 0;  
  drawPage();
}


int dviWindow::totalPages()
{
  if (dviFile != NULL)
    return dviFile->total_pages;
  else
    return 0;
}


double dviWindow::setZoom(double zoom)
{
  // In principle, this method should never be called with illegal
  // values. In principle.
  if (zoom < KViewPart::minZoom/1000.0)
    zoom = KViewPart::minZoom/1000.0;
  if (zoom > KViewPart::maxZoom/1000.0)
    zoom = KViewPart::maxZoom/1000.0;

  mane.shrinkfactor = currwin.shrinkfactor = basedpi/(xres*zoom);
  _zoom             = zoom;

  page_w = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;

  reset_fonts();
  changePageSize();
  return _zoom;
}


void dviWindow::paintEvent(QPaintEvent *ev)
{
  if (pixmap) {
    QPainter p(this);
    p.drawPixmap(QPoint(0, 0), *pixmap);
  }
}

void dviWindow::mousePressEvent ( QMouseEvent * e )
{
#ifdef DEBUG_SPECIAL
  kdDebug() << "mouse event" << endl;
#endif

  for(int i=0; i<num_of_used_hyperlinks; i++) {
    if (hyperLinkList[i].box.contains(e->pos())) {
      if (hyperLinkList[i].linkText[0] == '#' ) {
#ifdef DEBUG_SPECIAL
	kdDebug() << "hit: local link to " << hyperLinkList[i].linkText << endl;
#endif
	QString locallink = hyperLinkList[i].linkText.mid(1); // Drop the '#' at the beginning
	for(int j=0; j<numAnchors; j++) {
	  if (locallink.compare(AnchorList_String[j]) == 0) {
#ifdef DEBUG_SPECIAL
	    kdDebug() << "hit: local link to  y=" << AnchorList_Vert[j] << endl;
	    kdDebug() << "hit: local link to sf=" << mane.shrinkfactor << endl;
#endif
	    emit(request_goto_page(AnchorList_Page[j], (int)(AnchorList_Vert[j]/mane.shrinkfactor)));
	    break;
	  }
	}
      } else {
#ifdef DEBUG_SPECIAL
	kdDebug() << "hit: external link to " << hyperLinkList[i].linkText << endl;
#endif
	QUrl DVI_Url(dviFile->filename);
	QUrl Link_Url(DVI_Url, hyperLinkList[i].linkText, TRUE );
	
	KShellProcess proc;
	proc << "kfmclient openURL " << Link_Url.toString();
	proc.start(KProcess::Block);
	//@@@ Set up a warning requester if the command failed?
      }
      break;
    }
  }
}
#include "dviwin.moc"
