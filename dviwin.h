//
// Class: dviWindow
//
// Widget for displaying TeX DVI files.
//

#ifndef _dviwin_h_
#define _dviwin_h_


#include <stdio.h>

#include "../config.h"
#include <qpainter.h> 
#include <qevent.h>
#include <qwidget.h> 
#include <qintdict.h>
#include <qvaluestack.h>
#include <qvector.h>

#include <kviewpart.h>

#include "fontpool.h"
#include "psgs.h"
#include "dvi_init.h"

class fontProgressDialog;
class infoDialog;
class KPrinter;
class KShellProcess;


// max. number of hyperlinks per page. This should later be replaced by
// a dynamic allocation scheme.
#define MAX_HYPERLINKS 400 
// max. number of anchors per document. This should later be replaced by
// a dynamic allocation scheme.
#define MAX_ANCHORS    1000


class DVI_Hyperlink {
 public:
  unsigned int baseline;
  QRect        box;
  QString      linkText;
};


/** Compound of registers, as defined in section 2.6.2 of the DVI
    driver standard, Level 0, published by the TUG DVI driver
    standards committee. */

struct framedata {
  long dvi_h;
  long dvi_v;
  long w;
  long x;
  long y;
  long z;
  int pxl_v;
};


class dviWindow : public QWidget
{
  Q_OBJECT

public:
  dviWindow( double zoom, int makepk, QWidget *parent=0, const char *name=0 );
  ~dviWindow();

  class dvifile *dviFile;

  void          showInfo();
  void          exportPS(QString fname = QString::null, QString options = QString::null, KPrinter *printer = 0);
  void          exportPDF();
  void          changePageSize();
  int		totalPages();
  void		setShowPS( int flag );
  int		showPS() { return _postscript; };
  int		curr_page() { return current_page+1; };
  void		setShowHyperLinks( int flag );
  int		showHyperLinks() { return _showHyperLinks; };
  void		setEditorCommand( QString command )  { editorCommand = command; };
  void		setMakePK( int flag );
  int		makePK() { return makepk; };
  void		setMetafontMode( unsigned int );
  void		setPaper(double w, double h);
  bool          correctDVI(QString filename);
  
  // for the preview
  QPixmap      *pix() { return pixmap; };

  // These should not be public... only for the moment
  void          mousePressEvent ( QMouseEvent * e );
  void          mouseMoveEvent ( QMouseEvent * e );
  void          read_postamble(void);
  void          draw_part(double current_dimconv, bool is_vfmacro);
  void          set_vf_char(unsigned int cmd, unsigned int ch);
  void          set_char(unsigned int cmd, unsigned int ch);
  void          set_empty_char(unsigned int cmd, unsigned int ch);
  void          set_no_char(unsigned int cmd, unsigned int ch);
  void          applicationDoSpecial(char * cp);


  void          special(long nbytes);
  void          html_href_special(QString cp);
  void          html_anchor_end(void);
  void          html_anchor_special(QString cp);
  void          draw_page(void);

  class fontPool  *font_pool;

  double       xres;         // horizontal resolution of the display device in dots per inch.
  double       paper_width;  // paper width in centimeters
  double       paper_height; // paper height in centimeters



public slots:
  void          abortExternalProgramm(void);
  bool		setFile(const QString & fname);

  /** Displays the page of the first argument */
  void		gotoPage(unsigned int page);

  /** Displays the page of the first argument, and blinks the display
      at the vertical offset vflashOffset. This is used when the user
      clicks on a hyperlink: the target of the jump flashes so that
      the user can locate it more easily. */
  void		gotoPage(int page, int vflashOffset);
  double	setZoom(double zoom);
  double        zoom() { return _zoom; };
  void		drawPage();
 
  /** Slots used in conjunction with external programs */
  void          dvips_output_receiver(KProcess *, char *buffer, int buflen);
  void          dvips_terminated(KProcess *);
  void          editorCommand_terminated(KProcess *);

signals:
  /** Emitted to indicate that a hyperlink has been clicked on, and
      that the widget requests that the controlling program goes to the
      page and the coordinates specified. */
  void          request_goto_page(int page, int y);

  /** Emitted to indicate the the contents of the widget has changed
      and that the tumbnail image should be updated. */
  void          contents_changed(void);

  /** Passed through to the top-level kpart. */
  void setStatusBarText( const QString& );

protected:
 void paintEvent(QPaintEvent *ev);

private:
 /** Methods which handle certain special commands. */
 void          bang_special(QString cp);
 void          quote_special(QString cp);
 void          ps_special(QString cp);
 void          epsf_special(QString cp);
 void          header_special(QString cp);
 void          source_special(QString cp);

 /** List of source-hyperlinks, for source-specials */
 DVI_Hyperlink     sourceHyperLinkList[MAX_HYPERLINKS];
 int               num_of_used_source_hyperlinks;
 QString          *source_href; // If not NULL, the text currently
				// drawn represents a source hyperlink
				// to the (relative) URL given in the
				// string;
 QString           editorCommand;

 /** List of ordinary-hyperlinks */
 DVI_Hyperlink     hyperLinkList[MAX_HYPERLINKS];
 int               num_of_used_hyperlinks;
 QString          *HTML_href; // If not NULL, the text currently drawn
			      // represents a hyperlink to the
			      // (relative) URL given in the string;

 /** Stack for register compounds, used for the DVI-commands PUSH/POP
     as explained in section 2.5 and 2.6.2 of the DVI driver standard,
     Level 0, published by the TUG DVI driver standards committee. */
 QValueStack<struct framedata> stack;

 /** Methods and counters used for the animation to mark the target of
     an hyperlink. */
 int               timerIdent;
 void              timerEvent( QTimerEvent *e );
 int               animationCounter;
 int               flashOffset;

 /** These fields contain information about the geometry of the
     page. */
 unsigned int	   unshrunk_page_w; // basedpi * width(in inch)
 unsigned int	   unshrunk_page_h; // basedpi * height(in inch)

 infoDialog       *info;

 /** If PostScriptOutPutFile is non-zero, then no rendering takes
     place. Instead, the PostScript code which is generated by the
     \special-commands is written to the PostScriptString */
 QString          *PostScriptOutPutString;

 ghostscript_interface *PS_interface;

 /** TRUE, if gs should be used, otherwise, only bounding boxes are
     drawn. */
 unsigned char	   _postscript;

 /** TRUE, if Hyperlinks should be shown. */
 unsigned char	   _showHyperLinks;

 /** This flag is used when rendering a dvi-page. It is set to "true"
     when any dvi-command other than "set" or "put" series of commands
     is encountered. This is considered to mark the end of a word. */
 bool              word_boundary_encountered;

 /** List of anchors in a document */
 QString           AnchorList_String[MAX_ANCHORS];
 unsigned int      AnchorList_Page[MAX_ANCHORS];
 double            AnchorList_Vert[MAX_ANCHORS];
 int               numAnchors;

 int		   basedpi;
 int		   makepk;
 QPixmap          *pixmap;
 unsigned int	   MetafontMode;
 QString	   paper_type;
 int		   ChangesPossible;
 unsigned int	   current_page;

 /** Indicates if the current page is already drawn (=1) or not
     (=0). */
 char              is_current_page_drawn;
 double            _zoom;

 /** Used to run and to show the progress of dvips and friends. */
 fontProgressDialog *progress;
 KShellProcess      *proc;
 KPrinter           *export_printer;
 QString             export_fileName;
 QString             export_tmpFileName;
 QString             export_errorString;
};


#include <X11/Xlib.h>

struct	WindowRec {
  Window	win;
  double	shrinkfactor;
  int		base_x;
  int           base_y;
  unsigned int	width;
  unsigned int	height;
  int	        min_x;	/* for pending expose events */
  int	        max_x;	/* for pending expose events */
  int	        min_y;	/* for pending expose events */
  int	        max_y;	/* for pending expose events */
};






typedef	void	(dviWindow::*set_char_proc)(unsigned int, unsigned int);

#include "font.h"

/* this information is saved when using virtual fonts */
struct drawinf {	
  struct framedata      data;
  struct font          *fontp;
  set_char_proc	        set_char_p;

  QIntDict<struct font> fonttable;
  struct font	       *_virtual;
};

#undef Unsorted

#endif
