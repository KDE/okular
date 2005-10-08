// -*- C++ -*-
#ifndef KDVIMULTIPAGE_H
#define KDVIMULTIPAGE_H

#include "kmultipage.h"
#include "dviRenderer.h"

#include <qstringlist.h>

class KPrinter;

class KDVIMultiPage : public KMultiPage
{
  Q_OBJECT

public:
  KDVIMultiPage(QWidget *parentWidget, const char *widgetName, QObject *parent,
                const char *name, const QStringList& args = QStringList());
  virtual ~KDVIMultiPage();

// Interface definition start ------------------------------------------------

  /// returns the list of supported file formats
  virtual QStringList fileFormats() const;
  
  virtual void setFile(bool r);

  virtual void print();
  
  /// KDVI offers read- and write functionality must re-implement this
  /// method and return true here.
  virtual bool isReadWrite() {return true;}

  virtual void addConfigDialogs(KConfigDialog* configDialog);

  static KAboutData* createAboutData();

private:
  virtual DocumentWidget* createDocumentWidget();

  virtual void initializePageCache();

  /** Used to enable the export menu when a file is successfully
      loaded. */
  virtual void enableActions(bool);
  
public slots:
  /** Opens a file requestor and saves. This really saves the content
      of the DVI-file, and does not just start a copy job */
  virtual void slotSave();
 
  /** Similar to slotSave, but does not ask for a filename. */
  virtual void slotSave_defaultFilename();

  void setEmbedPostScriptAction();

  void slotEmbedPostScript();

  virtual void preferencesChanged();

  /** Shows the "text search" dialog, if text search is supported by
      the renderer. Otherwise, the method returns immediately.
      We reimplement this slot to show a warning message that informs the
      user about the currently limited search capabilities of KDVI. */
  virtual void showFindTextDialog();

protected slots:
  void doExportText();
  void doEnableWarnings();

  void showTip();
  void showTipOnStart();

private:
  // Points to the same object as renderer to avoid downcasting.
  // FIXME: Remove when the API of the Renderer-class is finished.
  dviRenderer     DVIRenderer;

  // Set to true if we used the search function atleast once.
  // It is used to remember if we already have show the warning message.
  bool searchUsed;

  /*************************************************************
   * Methods and classes concerned with the find functionality *
   *************************************************************/

  /** Pointers to several actions which are disabled if no file is
      loaded. */
  KAction      *docInfoAction;
  KAction      *embedPSAction;
  KAction      *exportPDFAction;
  KAction      *exportPSAction;
};


#endif
