// -*- C++ -*-
//
// Class: dviRenderer
//
// Class for rendering TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2001-2005 Stefan Kebekus. Distributed under the GPL.

#ifndef _dvirenderer_h_
#define _dvirenderer_h_

#include "bigEndianByteReader.h"
#include "documentRenderer.h"
#include "fontpool.h"

#include <kurl.h>
#include <qintdict.h>
#include <qpointarray.h>
#include <qtimer.h>
#include <qvaluestack.h>
#include <qvaluevector.h>

class Anchor;
class DocumentWidget;
class dviRenderer;
class fontProgressDialog;
class ghostscript_interface;
class infoDialog;
class KAction;
class KDVIMultiPage;
class KPrinter;
class KProcess;
class KProgressDialog;
class KShellProcess;
class PreBookmark;
class TeXFontDefinition;

extern const int MFResolutions[];

class DVI_SourceFileAnchor {
 public:
  DVI_SourceFileAnchor() {}
  DVI_SourceFileAnchor(const QString& name, Q_UINT32 ln, Q_UINT32 pg, const Length& _distance_from_top)
    : fileName(name), line(ln), page(pg),
      distance_from_top(_distance_from_top) {}

  QString    fileName;
  Q_UINT32   line;
  Q_UINT32   page;
  Length     distance_from_top;
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

typedef	void	(dviRenderer::*set_char_proc)(unsigned int, unsigned int);
typedef void    (dviRenderer::*parseSpecials)(char *, Q_UINT8 *);

struct drawinf {
  struct framedata            data;
  TeXFontDefinition          *fontp;
  set_char_proc	              set_char_p;

  QIntDict<TeXFontDefinition> *fonttable;
  TeXFontDefinition	      *_virtual;
};



class dviRenderer : public DocumentRenderer, bigEndianByteReader
{
  Q_OBJECT

public:
  dviRenderer(QWidget *parent);
  ~dviRenderer();

  virtual bool	setFile(const QString &fname, const KURL &base);

  class dvifile *dviFile;

  void          setPrefs(bool flag_showPS, const QString &editorCommand, bool useFontHints );

  virtual bool  supportsTextSearch() const {return true;}

  bool		showPS() { return _postscript; }
  int		curr_page() { return current_page+1; }
  virtual bool  isValidFile(const QString& fileName) const;


  /** This method will try to parse the reference part of the DVI
      file's URL, (either a number, which is supposed to be a page
      number, or src:<line><filename>) and see if a corresponding
      section of the DVI file can be found. If so, it returns an
      anchor to that section. If not, it returns an invalid anchor. */
  virtual Anchor        parseReference(const QString &reference);
  
  // These should not be public... only for the moment
  void          read_postamble();
  void          draw_part(double current_dimconv, bool is_vfmacro);
  void          set_vf_char(unsigned int cmd, unsigned int ch);
  void          set_char(unsigned int cmd, unsigned int ch);
  void          set_empty_char(unsigned int cmd, unsigned int ch);
  void          set_no_char(unsigned int cmd, unsigned int ch);
  void          applicationDoSpecial(char * cp);

  void          special(long nbytes);
  void          printErrorMsgForSpecials(const QString& msg);
  void          color_special(const QString& cp);
  void          html_href_special(const QString& cp);
  void          html_anchor_end();
  void          draw_page();

public slots:
  void          exportPS(const QString& fname = QString::null, const QString& options = QString::null, KPrinter* printer = 0);
  void          exportPDF();

  void          showInfo();
  void          handleSRCLink(const QString &linkText, QMouseEvent *e, DocumentWidget *widget);

  void          embedPostScript();
  void          abortExternalProgramm();

  /** simply emits "setStatusBarText( QString::null )". This is used
      in dviRenderer::mouseMoveEvent(), see the explanation there. */
  void          clearStatusBar();

  virtual void  drawPage(double res, RenderedDocumentPage *page);
  virtual void  getText(RenderedDocumentPage* page);

  /** Slots used in conjunction with external programs */
  void          dvips_output_receiver(KProcess *, char *buffer, int buflen);
  void          dvips_terminated(KProcess *);
  void          editorCommand_terminated(KProcess *);

signals:
  /** Passed through to the top-level kpart. */
  //  void setStatusBarText( const QString& );

private slots:
  /** This method shows a dialog that tells the user that source
      information is present, and gives the opportunity to open the
      manual and learn more about forward and inverse search */
  void          showThatSourceInformationIsPresent();

private:
  /** URL to the DVI file

  This field is initialized by the setFile() method. See the
  explanation there.  */
  KURL baseURL;


 /** This method parses a color specification of type "gray 0.5", "rgb
     0.5 0.7 1.0", "hsb ...", "cmyk .." or "PineGreen". See the source
     code for details. */
 QColor parseColorSpecification(const QString& colorSpec);
 
 /** This map contains the colors which are known by name. This field
     is initialized in the method parseColorSpecification() as soon as
     it is needed. */
 QMap<QString, QColor> namedColors;

  /* This method locates special PDF characters in a string and
     replaces them by UTF8. See Section 3.2.3 of the PDF reference
     guide for information */
  QString PDFencodingToQString(const QString& pdfstring);

  void  setResolution(double resolution_in_DPI);

  fontPool      font_pool;
  infoDialog    *info;

  double        resolutionInDPI;

  // @@@ explanation
  void          prescan(parseSpecials specialParser);
  void          prescan_embedPS(char *cp, Q_UINT8 *);
  void          prescan_removePageSizeInfo(char *cp, Q_UINT8 *);
  void          prescan_parseSpecials(char *cp, Q_UINT8 *);
  void          prescan_ParsePapersizeSpecial(const QString& cp);
  void          prescan_ParseBackgroundSpecial(const QString& cp);
  void          prescan_ParseHTMLAnchorSpecial(const QString& cp);
  void          prescan_ParsePSHeaderSpecial(const QString& cp);
  void          prescan_ParsePSBangSpecial(const QString& cp);
  void          prescan_ParsePSQuoteSpecial(const QString& cp);
  void          prescan_ParsePSSpecial(const QString& cp);
  void          prescan_ParsePSFileSpecial(const QString& cp);
  void          prescan_ParseSourceSpecial(const QString& cp);
  void          prescan_setChar(unsigned int ch);

  /* */
  QValueVector<PreBookmark> prebookmarks;



  /** Utility fields used by the embedPostScript method*/
  KProgressDialog *embedPS_progress;
  Q_UINT16         embedPS_numOfProgressedFiles;

  /** Shrink factor. Units are not quite clear */
  double	shrinkfactor;
  
  QString       errorMsg;
  
  /** Methods which handle certain special commands. */
  void epsf_special(const QString& cp);
  void source_special(const QString& cp);
  
  /** TPIC specials */
  void TPIC_setPen_special(const QString& cp);
  void TPIC_addPath_special(const QString& cp);
  void TPIC_flushPath_special();
  
  /** This timer is used to delay clearing of the statusbar. Clearing
      the statusbar is delayed to avoid awful flickering when the
      mouse moves over a block of text that contains source
      hyperlinks. The signal timeout() is connected to the method
      clearStatusBar() of *this. */
  QTimer        clearStatusBarTimer;
  
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
  
  /** true, if gs should be used, otherwise, only bounding boxes are
      drawn. */
  bool    	   _postscript;
  
  /** This flag is used when rendering a dvi-page. It is set to "true"
      when any dvi-command other than "set" or "put" series of commands
      is encountered. This is considered to mark the end of a word. */
  bool              line_boundary_encountered;
  bool              word_boundary_encountered;
  
  unsigned int	   current_page;
  
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
  RenderedDocumentPage* currentlyDrawnPage;
};

#endif
