//
// Class: dviWindow
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2001-2003 Stefan Kebekus. Distributed under the GPL.


#ifndef _dviwin_h_
#define _dviwin_h_


#include <qevent.h>
#include <qintdict.h>
#include <qpainter.h> 
#include <qptrvector.h>
#include <qvaluestack.h>
#include <qvaluevector.h>
#include <qwidget.h> 
#include <kviewpart.h>

#include "bigEndianByteReader.h"
#include "dviFile.h"
#include "fontpool.h"
#include "psgs.h"
#include "selection.h"

class dviWindow;
class fontProgressDialog;
class infoDialog;
class KAction;
class KEdFind;
class KPrinter;
class KProcess;
class KShellProcess;
class TeXFontDefinition;

extern const int MFResolutions[];

class DVI_Hyperlink {
 public:
  DVI_Hyperlink() {}
  DVI_Hyperlink(Q_UINT32 bl, QRect re, QString lT): baseline(bl), box(re), linkText(lT) {}

  Q_UINT32 baseline;
  QRect    box;
  QString  linkText;
};

class DVI_Anchor {
 public:
  DVI_Anchor() {}
  DVI_Anchor(Q_UINT32 pg, double vc): page(pg), vertical_coordinate(vc) {}

  Q_UINT32   page;
  double     vertical_coordinate;
};

class DVI_SourceFileAnchor {
 public:
  DVI_SourceFileAnchor() {}
  DVI_SourceFileAnchor(QString &name, Q_UINT32 ln, Q_UINT32 pg, double vc): fileName(name), line(ln), page(pg), vertical_coordinate(vc) {}

  QString    fileName;
  Q_UINT32   line;
  Q_UINT32   page;
  double     vertical_coordinate;
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


/* this information is saved when using virtual fonts */

typedef	void	(dviWindow::*set_char_proc)(unsigned int, unsigned int);
struct drawinf {
  struct framedata            data;
  TeXFontDefinition          *fontp;
  set_char_proc	              set_char_p;

  QIntDict<TeXFontDefinition> *fonttable;
  TeXFontDefinition	      *_virtual;
};


// This class contains everything one needs dviwin needs to know about
// a certain page. 

class pageData
{
 public:
  Q_UINT16   pageNumber;
  
  QPixmap    *pixmap;
  
  // List of source-hyperlinks in the current page. This vector is
  // generated when the current page is drawn.
  QValueVector<DVI_Hyperlink> sourceHyperLinkList;

  QValueVector<DVI_Hyperlink> textLinkList; // List of text in the window
  QValueVector<DVI_Hyperlink> hyperLinkList; // List of ordinary hyperlinks
    
};


class dviWindow : public QWidget, bigEndianByteReader
{
  Q_OBJECT

public:
  dviWindow( double zoom, QWidget *parent=0, const char *name=0 );
  ~dviWindow();

  class dvifile *dviFile;

  void          showInfo();
  void          exportPS(QString fname = QString::null, QString options = QString::null, KPrinter *printer = 0);
  void          exportPDF();
  void          exportText();

  void          changePageSize(void);
  int		totalPages(void);
  void		setShowPS( bool flag );
  bool		showPS(void) { return _postscript; };
  int		curr_page(void) { return current_page+1; };
  void		setShowHyperLinks( bool flag );
  bool		showHyperLinks(void) { return _showHyperLinks; };
  void		setEditorCommand( QString command )  { editorCommand = command; };
  void		setPaper(double w, double h);
  bool          correctDVI(QString filename);
  
  // for the preview
  QPixmap      *pix() { return currentlyDrawnPage.pixmap; };

  // These should not be public... only for the moment
  void          mousePressEvent ( QMouseEvent * e );
  void          mouseMoveEvent ( QMouseEvent * e );
  void          mouseReleaseEvent ( QMouseEvent * e );
  void          read_postamble(void);
  void          draw_part(double current_dimconv, bool is_vfmacro);
  void          set_vf_char(unsigned int cmd, unsigned int ch);
  void          set_char(unsigned int cmd, unsigned int ch);
  void          set_empty_char(unsigned int cmd, unsigned int ch);
  void          set_no_char(unsigned int cmd, unsigned int ch);
  void          applicationDoSpecial(char * cp);


  void          special(long nbytes);
  void          printErrorMsgForSpecials(QString msg);
  void          background_special(QString cp);
  void          color_special(QString cp);
  void          html_href_special(QString cp);
  void          html_anchor_end(void);
  void          html_anchor_special(QString cp);
  void          draw_page(void);

  fontPool  *font_pool;

  double       xres;         // horizontal resolution of the display device in dots per inch.
  double       paper_width_in_cm;  // paper width in centimeters
  double       paper_height_in_cm; // paper height in centimeters

  selection    DVIselection;

 /** Reference part of the URL which describes the filename. */
 QString        reference;

 QString        searchText;
 KAction        *findNextAction;
 KAction        *findPrevAction;

public slots:
  void          selectAll(void);
  void          copyText(void);
  void          showFindTextDialog(void);
  void          findText(void);
  void          findNextText(void);
  void          findPrevText(void);

  void          abortExternalProgramm(void);
  bool		setFile(QString fname, QString ref = QString::null, bool sourceMarker=true);

  /** simply emits "setStatusBarText( QString::null )". This is used
      in dviWindow::mouseMoveEvent(), see the explanation there. */;
  void          clearStatusBar(void);

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


  /** This slot is usually called by the fontpool if all fonts are
      loaded. The method will try to parse the reference part of the
      DVI file's URL, e.g. src:<line><filename> and see if a
      corresponding section of the DVI file can be found. If so, it
      will emit a "requestGotoPage", otherwise it will just call
      drawpage */
  void          all_fonts_loaded(fontPool *);

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
 void          paintEvent(QPaintEvent *ev);

private:
 /** Shrink factor. Units are not quite clear */
 double	shrinkfactor;

 QString       errorMsg;

 /** Methods which handle certain special commands. */
 void          bang_special(QString cp);
 void          quote_special(QString cp);
 void          ps_special(QString cp);
 void          epsf_special(QString cp);
 void          header_special(QString cp);
 void          source_special(QString cp);

 /* This timer is used to delay clearing of the statusbar. Clearing
    the statusbar is delayed to avoid awful flickering when the mouse
    moves over a block of text that contains source hyperlinks. The
    signal timeout() is connected to the method clearStatusBar() of
    *this. */
 QTimer        clearStatusBarTimer;

 pageData      currentlyDrawnPage;


 // List of source-hyperlinks on all pages. This vector is generated
 // when the DVI-file is first loaded, i.e. when draw_part is called
 // with PostScriptOutPutString != NULL
 QValueVector<DVI_SourceFileAnchor>  sourceHyperLinkAnchors;

 // If not NULL, the text currently drawn represents a source
 // hyperlink to the (relative) URL given in the string;
 QString          *source_href;

 // If not NULL, the text currently drawn represents a hyperlink to
 // the (relative) URL given in the string;
 QString          *HTML_href;
 
 QString           editorCommand;
 
 /** Stack for register compounds, used for the DVI-commands PUSH/POP
     as explained in section 2.5 and 2.6.2 of the DVI driver standard,
     Level 0, published by the TUG DVI driver standards committee. */
 QValueStack<struct framedata> stack;

 /** A stack where color are stored, according to the documentation of
     DVIPS */
 QValueStack<QColor> colorStack;

 /** The global color is to be used when the color stack is empty */
 QColor              globalColor;

 /** Methods and counters used for the animation to mark the target of
     an hyperlink. */
 int               timerIdent;
 void              timerEvent( QTimerEvent *e );
 int               animationCounter;
 int               flashOffset;

 /** Methods and classes concerned with the find functionality and
     with selecting text */
 class KEdFind    *findDialog;
 QPoint            firstSelectedPoint;
 QRect             selectedRectangle;

 infoDialog       *info;

 /** If PostScriptOutPutFile is non-zero, then no rendering takes
     place. Instead, the PostScript code which is generated by the
     \special-commands is written to the PostScriptString */
 QString          *PostScriptOutPutString;

 ghostscript_interface *PS_interface;

 /** TRUE, if gs should be used, otherwise, only bounding boxes are
     drawn. */
 bool    	   _postscript;

 /** TRUE, if Hyperlinks should be shown. */
 bool     	   _showHyperLinks;

 /** This flag is used when rendering a dvi-page. It is set to "true"
     when any dvi-command other than "set" or "put" series of commands
     is encountered. This is considered to mark the end of a word. */
 bool              line_boundary_encountered;
 bool              word_boundary_encountered;

 /** List of anchors in a document */
 QMap<QString, DVI_Anchor> anchorList;

 double            fontPixelPerDVIunit() {return dviFile->cmPerDVIunit * MFResolutions[font_pool->getMetafontMode()]/2.54;};

 int		   ChangesPossible;
 unsigned int	   current_page;

 /** Indicates if the current page is already drawn (=1) or not
     (=0). */
 char              is_current_page_drawn;

 // Zoom factor. 1.0 means "100%"
 double            _zoom;

 /** Used to run and to show the progress of dvips and friends. */
 fontProgressDialog *progress;
 KShellProcess      *proc;
 KPrinter           *export_printer;
 QString             export_fileName;
 QString             export_tmpFileName;
 QString             export_errorString;

 struct drawinf	currinf;
};



#endif
