#ifndef _KPDF_PART_H_
#define _KPDF_PART_H_

#include <qpainter.h>
#include <qpixmap.h>
#include <qwidget.h>

#include <kparts/browserextension.h>
#include <kparts/part.h>

#include "QOutputDev.h"

class QPainter;
class QPixmap;
class QWidget;
class QListBoxItem;

class KAboutData;
class KAction;
class KURL;
class KToggleAction;

class LinkAction;
class LinkDest;
class PDFDoc;
class XOutputDev;

class PDFPartView;

namespace KPDF
{
  class BrowserExtension;
  class Canvas;
  class PageWidget;

  /**
   * This is a "Part".  It that does all the real work in a KPart
   * application.
   *
   * @short Main Part
   * @author Wilco Greven <greven@kde.org>
   * @version 0.1
   */
  class Part : public KParts::ReadOnlyPart
  {
    Q_OBJECT

  public:
     // Do with these first. We can always add the other zoommodes which can
     // be specified in a Destination later.
     enum ZoomMode { FitInWindow, FitWidth, FitVisible, FixedFactor };

    /**
     * Default constructor
     */
    Part(QWidget* parentWidget, const char* widgetName,
         QObject* parent, const char* name, const QStringList& args);

    /**
     * Destructor
     */
    virtual ~Part();

    static KAboutData* createAboutData();

    bool closeURL();

    void print();

    void displayPage(int pageNumber, float zoomFactor = 1.0);
    void displayDestination(LinkDest*);

  protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();

		void update();

  protected slots:
    void find()     { /* stub */ };
    void findNext() { /* stub */ };
    void zoomIn()   { m_zoomFactor += 0.1; update(); };
    void zoomOut()  { m_zoomFactor -= 0.1; update(); };
    void back()     { /* stub */ };
    void forward()  { /* stub */ };

    void displayNextPage();
    void displayPreviousPage();

    void executeAction(LinkAction*);

  private:
    PDFDoc*     m_doc;
    PageWidget* m_outputDev;
		PDFPartView * pdfpartview;

    KToggleAction* m_fitToWidth;

    int   m_currentPage;

    ZoomMode m_zoomMode;
    float    m_zoomFactor;

  private slots:
    void slotFitToWidthToggled();
		void redrawPage();
		void pageClicked ( QListBoxItem * );
  };

  class BrowserExtension : public KParts::BrowserExtension
  {
    Q_OBJECT
      
  public:
    BrowserExtension(Part*);

  public slots:
    // Automatically detected by the host.
    void print();
  };
  
}

#endif

// vim:ts=2:sw=2:tw=78:et
