#ifndef _KPDF_PART_H_
#define _KPDF_PART_H_

#include <qpainter.h>
#include <qpixmap.h>
#include <qwidget.h>

#include <kparts/part.h>

class QPainter;
class QPixmap;
class QWidget;

class KAboutData;
class KAction;
class KURL;
class KToggleAction;

class LinkAction;
class LinkDest;
class PDFDoc;
class XOutputDev;

namespace KPDF
{
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

    void displayPage(int pageNumber, float zoomFactor = 1.0);
    void displayDestination(LinkDest*);

  protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();

  protected slots:
    void fileOpen();
    void fileSaveAs();
    void filePrint();

    void displayNextPage();
    void displayPreviousPage();

    void executeAction(LinkAction*);

  private:
    Canvas*     m_canvas;
    QPixmap     m_pagePixmap;
    PageWidget* m_pageWidget;
    PDFDoc*     m_doc;
    XOutputDev* m_outputDev;

    KToggleAction* m_fitWidth;

    int   m_currentPage;

    ZoomMode m_zoomMode;
    float    m_zoomFactor;

  private slots:
    void fitWidthToggled();
  };
}

#endif

// vim:ts=2:sw=2:tw=78:et
