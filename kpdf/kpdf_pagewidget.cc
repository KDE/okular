#include <qcursor.h>

#include "PDFDoc.h"

#include "kpdf_pagewidget.h"
#include "kpdf_pagewidget.moc"

using namespace KPDF;

PageWidget::PageWidget(QWidget* parent, const char* name)
  : QWidget(parent, name), 
    m_doc(0),
    m_pressedAction( 0 )
{
  setMouseTracking(true);
}

  void 
PageWidget::setPixmap(const QPixmap& pm)
{
  m_pixmap = pm;
}

  void 
PageWidget::setPDFDocument(PDFDoc* doc)
{
  m_doc = doc;
}

  void 
PageWidget::setPixelsPerPoint(float ppp)
{
  m_ppp = ppp;
}

  void 
PageWidget::mousePressEvent(QMouseEvent* e)
{
  if (m_doc == 0)
    return;

  m_pressedAction = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
}

  void 
PageWidget::mouseReleaseEvent(QMouseEvent* e)
{
  if (m_doc == 0)
    return;

  LinkAction* action = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
  if (action == m_pressedAction)
    emit linkClicked(action);

  m_pressedAction = 0;
}

  void
PageWidget::mouseMoveEvent(QMouseEvent* e)
{
  if (m_doc == 0)
    return;
    
  LinkAction* action = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
  setCursor(action != 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

// vim:ts=2:sw=2:tw=78:et
