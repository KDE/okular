#ifndef __KDVIMULTIPAGE_H
#define __KDVIMULTIPAGE_H


#include <qstringlist.h>


#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/factory.h>

class documentWidget;
class KPrinter;
class OptionDialog;
class QLabel;
class QPainter;


#include "../kviewshell/kmultipage.h"
#include "documentPageCache.h"
#include "dviwin.h"
#include "history.h"


class KDVIMultiPageFactory : public KParts::Factory
{
  Q_OBJECT

public:
  KDVIMultiPageFactory();
  virtual ~KDVIMultiPageFactory();

  virtual KParts::Part *createPartObject( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *, const QStringList & );

  static KInstance *instance();

private:

  static KInstance *s_instance;
};


class KDVIMultiPage : public KMultiPage
{
  Q_OBJECT

public:
  enum viewModes {KVS_SinglePage = 0, KVS_Continuous = 1};

  documentPageCache  currentPage;

  KDVIMultiPage(QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name);
  virtual ~KDVIMultiPage();
  // Methods which are associated with the DCOP functionality of the
  // kmultipage. This method can be implemented by the multipage,
  // e.g. to jump to a certain location.
  void  jumpToReference(QString reference);

// Interface definition start ------------------------------------------------

  /// returns the list of supported file formats
  virtual QStringList fileFormats();

  /// opens a file
  virtual bool openFile();

  /// close a file
  virtual bool closeURL();

  /// displays the given page
  virtual bool gotoPage(int page);

  /// sets a zoom factor
  virtual double setZoom(double z);

  /// calculates the zoom needed to fit into a given width
  virtual double zoomForWidth(int width);

  /// calculates the zoom needed to fit into a given height
  virtual double zoomForHeight(int height);

  virtual void setPaperSize(double, double);

  virtual bool print(const QStringList &pages, int current);

  /// KDVI offers read- and write functionality must re-implement this
  /// method and return true here.
  virtual bool isReadWrite() {return true;};

  /// multipage implementations that offer read- and write
  /// functionality should re-implement this method.
  virtual bool isModified();

  /** Returns the number of the first (i.e. top-left) page in the
      display. Such a number cannot be reasonably assigned
      (e.g. because the current document is empty, or because no
      document has been loaded yet), the method returns "invalid
      page", i.e. 0. */
  Q_UINT16 getCurrentPageNumber();


public slots:
  /** Opens a file requestor and saves. This really saves the content
      of the DVI-file, and does not just start a copy job */
  virtual void slotSave();

  /** Similar to slotSave, but does not ask for a filename. */
  virtual void slotSave_defaultFilename();

 void setEmbedPostScriptAction(void);

 void slotEmbedPostScript(void);

 /** Shows the "text search" dialog, if text search is supported by
     the renderer. Otherwise, the method returns immediately. */
 void showFindTextDialog(void);
 
 /** Starts the text search functionality. The text to search, and the
     direction in which to search is read off the 'findDialog'
     member, which must not be NULL. */
  void  findText(void);

  /** This method may be called after the text search dialog
      'findDialog' has been set up, and the user has entered a search
      phrase. The method searches for the next occurence of the text,
      starting from the beginning of the current page, or after the
      currently selected text, if there is any. */
  void findNextText(void);

  /** This method may be called after the text search dialog
      'findDialog' has been set up, and the user has entered a search
      phrase. The method searches for the next occurence of the text,
      starting from the end of the current page, or before the
      currently selected text, if there is any. */
  void findPrevText(void);

  void setViewMode(int);

protected:
  history document_history;

  /// For internal use only. See the comments in kdvi_multipage.cpp, right
  //before the timerEvent function.
  int  timer_id;
  /// For internal use only. See the comments in kdvi_multipage.cpp, right
  //before the timerEvent function.
  void timerEvent( QTimerEvent *e );

  // Set initial window caption
  void guiActivateEvent( KParts::GUIActivateEvent * event );

  virtual void reload();

signals:
  /** Emitted to indicate the number of pages in the file and the
      current page. The receiver will not change or update the
      display, nor call the gotoPage()-method. */
  void pageInfo(int nr, int currpg);

protected slots:
  void doSettings();
  void doExportPS();
  void doExportPDF();
  void doExportText();
  void doSelectAll();
  void doGoBack();
  void doGoForward();
  void doEnableWarnings();
  void about();
  void helpme();
  void bugform();
  void preferencesChanged();
  void goto_page(int page, int y);

  void generateDocumentWidgets(void);

  void contentsMovingInScrollView(int x, int y);

  /** Makes page # pageNr visible, selects the text Elements
      beginSelection-endSelection, and draws the users attention to
      this place with an animated frame  */
  void gotoPage(int pageNr, int beginSelection, int endSelection );
  void showTip(void);
  void showTipOnStart(void);

private:
  dviWindow    *window;
  OptionDialog *options;
  KPrinter     *printer;

  QPtrVector<QWidget> widgetList; 
  /*************************************************************
   * Methods and classes concerned with the find functionality *
   *************************************************************/

  /** Pointer to the text search dialog. This dialog is generally set
      up when the method 'showFindTextDialog(void)' is first
      called. */
  class KEdFind    *findDialog;


  /** Pointers to several actions which are disabled if no file is
      loaded. */
  KAction      *docInfoAction;
  KAction      *backAction;
  KAction      *forwardAction;
  KAction      *embedPSAction;
  KAction      *copyTextAction;
  KAction      *selectAllAction;
  KAction      *findTextAction;
  KAction      *findNextTextAction;
  KAction      *findPrevAction;
  KAction      *exportPDFAction;
  KAction      *exportTextAction;
  KAction      *findNextAction;
  KAction      *exportPSAction;
  KSelectAction *viewModeAction;


  /** Used to enable the export menu when a file is successfully
      loaded. */
  void enableActions(bool);
};


#endif
