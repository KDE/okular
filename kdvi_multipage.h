#ifndef __KDVIMULTIPAGE_H
#define __KDVIMULTIPAGE_H


#include <qstringlist.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/factory.h>

class KPrinter;

#include "../kviewshell/kmultipage.h"
#include "dviwin.h"


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

  virtual void setPaperSize(double, double);

  virtual bool print(const QStringList &pages, int current);

  /// KDVI offers read- and write functionality must re-implement this
  /// method and return true here.
  virtual bool isReadWrite() {return true;};

  /// multipage implementations that offer read- and write
  /// functionality should re-implement this method.
  virtual bool isModified();

  virtual void addConfigDialogs(KConfigDialog* configDialog);

private:
  virtual documentWidget* createDocumentWidget();

  /** Used to enable the export menu when a file is successfully
      loaded. */
  virtual void enableActions(bool);

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

protected:
  /// For internal use only. See the comments in kdvi_multipage.cpp, right
  //before the timerEvent function.
  int  timer_id;
  /// For internal use only. See the comments in kdvi_multipage.cpp, right
  //before the timerEvent function.
  void timerEvent( QTimerEvent *e );

  virtual void reload();

protected slots:
  void doExportPS();
  void doExportPDF();
  void doExportText();
  void doSelectAll();
  void doEnableWarnings();
  void preferencesChanged();

  /** Makes page # pageNr visible, selects the text Elements
      beginSelection-endSelection, and draws the users attention to
      this place with an animated frame  */
  void gotoPage(int pageNr, int beginSelection, int endSelection );
  void gotoPage(const anchor &a);

  void showTip(void);
  void showTipOnStart(void);

private:
  // Points to the same object as renderer to avoid downcasting.
  // FIXME: Remove when the API of the Renderer-class is finished.
  dviRenderer       *window;
  KPrinter        *printer;

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
  KAction      *embedPSAction;
  KAction      *copyTextAction;
  KAction      *selectAllAction;
  KAction      *deselectAction;
  KAction      *findTextAction;
  KAction      *findNextTextAction;
  KAction      *findPrevAction;
  KAction      *exportPDFAction;
  KAction      *exportTextAction;
  KAction      *findNextAction;
  KAction      *exportPSAction;
};


#endif
