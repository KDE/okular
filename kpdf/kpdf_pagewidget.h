#ifndef _KPDF_PAGEWIDGET_H_
#define _KPDF_PAGEWIDGET_H_

#include <qpixmap.h>
#include <qwidget.h>

class LinkAction;
class PDFDoc;

namespace KPDF
{
  /**
   * Widget displaying a pixmap containing a PDF page and Links.
   */
  class PageWidget : public QWidget
  {
    Q_OBJECT

  public:
    PageWidget(QWidget* parent = 0, const char* name = 0);

    void setPixmap(const QPixmap&);
    void setPDFDocument(PDFDoc*);
    void setPixelsPerPoint(float);
      // void setLinks();

  signals:
    void linkClicked(LinkAction*);

  protected:
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);

  private:
    QPixmap m_pixmap;
    PDFDoc* m_doc;
    float   m_ppp; // Pixels per point

    LinkAction* m_pressedAction;
  };
}

#endif

// vim:ts=2:sw=2:tw=78:et
