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
#include "documentPage.h"
#include "dviFile.h"
#include "fontpool.h"
#include "infodialog.h"
#include "psgs.h"


class documentWidget;
class dviWindow;
class fontProgressDialog;
class infoDialog;
class KAction;
class KDVIMultiPage;
class KPrinter;
class KProcess;
class KProgressDialog;
class KShellProcess;
class TeXFontDefinition;

extern const int MFResolutions[];

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
typedef void    (dviWindow::*parseSpecials)(char *, Q_UINT8 *);

struct drawinf {
  struct framedata            data;
  TeXFontDefinition          *fontp;
  set_char_proc	              set_char_p;

  QIntDict<TeXFontDefinition> *fonttable;
  TeXFontDefinition	      *_virtual;
};



class dviWindow : public QObject, bigEndianByteReader
{
  Q_OBJECT

public:
  dviWindow(QWidget *parent);
  ~dviWindow();

  documentPage  *currentlyDrawnPage;

  QSize         sizeOfPage(Q_UINT16 page) {return currentlyDrawnPixmap.size();};

  class dvifile *dviFile;

  void          setPrefs(bool flag_showPS, bool flag_showHyperLinks, const QString &editorCommand, 
			 unsigned int MetaFontMode, bool makePK, bool useType1Fonts, bool useFontHints );

  bool          supportsTextSearch(void) {return true;};

  void          exportPS(QString fname = QString::null, QString options = QString::null, KPrinter *printer = 0);
  void          exportPDF();

  void          changePageSize(void);
  int		totalPages(void);
  bool		showPS(void) { return _postscript; };
  int		curr_page(void) { return current_page+1; };
  bool		showHyperLinks(void) { return _showHyperLinks; };
  void		setPaper(double w, double h);
  static bool   correctDVI(const QString &filename);
  
  // for the preview
  QPixmap      *pix() { if (currentlyDrawnPage) return currentlyDrawnPage->getPixmap(); return 0; };

  // These should not be public... only for the moment
  void          read_postamble(void);
  void          draw_part(double current_dimconv, bool is_vfmacro);
  void          set_vf_char(unsigned int cmd, unsigned int ch);
  void          set_char(unsigned int cmd, unsigned int ch);
  void          set_empty_char(unsigned int cmd, unsigned int ch);
  void          set_no_char(unsigned int cmd, unsigned int ch);
  void          applicationDoSpecial(char * cp);

  void          special(long nbytes);
  void          printErrorMsgForSpecials(QString msg);
  void          color_special(QString cp);
  void          html_href_special(QString cp);
  void          html_anchor_end(void);
  void          draw_page(void);

  double        xres;         // horizontal resolution of the display device in dots per inch.
  double        paper_width_in_cm;  // paper width in centimeters
  double        paper_height_in_cm; // paper height in centimeters


 /** Reference part of the URL which describes the filename. */
 QString        reference;


public slots:
  void          showInfo(void);
  void          handleLocalLink(const QString &linkText);
  void          handleSRCLink(const QString &linkText, QMouseEvent *e, documentWidget *widget);

  void          embedPostScript(void);
  void          abortExternalProgramm(void);
  bool		setFile(const QString &fname, const QString &ref = QString::null, bool sourceMarker=true);

  /** simply emits "setStatusBarText( QString::null )". This is used
      in dviWindow::mouseMoveEvent(), see the explanation there. */
  void          clearStatusBar(void);

  double	setZoom(double zoom);
  double        zoom() { return _zoom; };
  void		drawPage(documentPage *page);
 
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
  /** Emitted if the status of this class changed internally so that
      all associated widgets should be repainted, e.g. if a lengthy
      network download finished. */
  void          needsRepainting();

  /** Emitted to indicate that the prescan phase has ended */
  void          prescanDone();

  /** Emitted to indicate that a hyperlink has been clicked on, and
      that the widget requests that the controlling program goes to the
      page and the coordinates specified. */
  void          request_goto_page(int page, int y);

  /** Passed through to the top-level kpart. */
  void setStatusBarText( const QString& );

  /** To be passed through to the kmultipage */
  void documentSpecifiedPageSize(const pageSize &size);


private:
  fontPool      font_pool;
  infoDialog    *info;

  QWidget       *parentWidget;


  // @@@ explanation
  void          prescan(parseSpecials specialParser);
  void          prescan_embedPS(char *cp, Q_UINT8 *);
  void          prescan_removePageSizeInfo(char *cp, Q_UINT8 *);
  void          prescan_parseSpecials(char *cp, Q_UINT8 *);
  void          prescan_ParsePapersizeSpecial(QString cp);
  void          prescan_ParseBackgroundSpecial(QString cp);
  void          prescan_ParseHTMLAnchorSpecial(QString cp);
  void          prescan_ParsePSHeaderSpecial(QString cp);
  void          prescan_ParsePSBangSpecial(QString cp);
  void          prescan_ParsePSQuoteSpecial(QString cp);
  void          prescan_ParsePSSpecial(QString cp);
  void          prescan_ParsePSFileSpecial(QString cp);
  void          prescan_ParseSourceSpecial(QString cp);
  void          prescan_setChar(unsigned int ch);
  
  /** Utility fields used by the embedPostScript method*/
  KProgressDialog *embedPS_progress;
  Q_UINT16         embedPS_numOfProgressedFiles;


  /** Shrink factor. Units are not quite clear */
  double	shrinkfactor;
  
  QString       errorMsg;
  
  /** Methods which handle certain special commands. */
  void epsf_special(QString cp);
  void source_special(QString cp);
  
  /** TPIC specials */
  void TPIC_setPen_special(QString cp);
  void TPIC_addPath_special(QString cp);
  void TPIC_flushPath_special(void);
  
  /* This timer is used to delay clearing of the statusbar. Clearing
     the statusbar is delayed to avoid awful flickering when the mouse
     moves over a block of text that contains source hyperlinks. The
     signal timeout() is connected to the method clearStatusBar() of
     *this. */
  QTimer        clearStatusBarTimer;
  
  QPixmap       currentlyDrawnPixmap;
  
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
  
  unsigned int	   current_page;
  
  // Zoom factor. 1.0 means "100%"
  double            _zoom;

  /** Used to run and to show the progress of dvips and friends. */
  fontProgressDialog *progress;
  KShellProcess      *proc;
  KPrinter           *export_printer;
  QString             export_fileName;
  QString             export_tmpFileName;
  QString             export_errorString;
  
  /** Data required for handling TPIC specials */ 
  float       penWidth_in_mInch;
  QPointArray TPIC_path;
  Q_UINT16    number_of_elements_in_path;
  
  struct drawinf	currinf;
};



#endif
