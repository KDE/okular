// include xpdf config file
#include "config.h"

#include "QOutputDevPixmap.h"

#include <kglobal.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qmutex.h>

#include <kdebug.h>
#include <kurldrag.h>
#include <kglobalsettings.h> 

#include "PDFDoc.h"

#include "kpdf_pagewidget.h"
#include "kpdf_pagewidget.moc"

namespace KPDF
{
    PageWidget::PageWidget(QWidget* parent, const char* name)
        : QScrollView(parent, name, WRepaintNoErase),
          m_doc(0),
          m_zoomFactor( 1.0 ),
          m_currentPage( 1 ),
          m_pressedAction( 0 )
    {
        m_outputdev = new QOutputDevPixmap();
        setFocusPolicy( QWidget::StrongFocus );
        viewport()->setFocusPolicy( QWidget::WheelFocus );
    }
    PageWidget::~PageWidget()
    {
        delete m_outputdev;
    }
    void
    PageWidget::setPDFDocument(PDFDoc* doc)
    {
        m_doc = doc;
        updatePixmap();
    }

    void
    PageWidget::setPixelsPerPoint(float ppp)
    {
        m_ppp = ppp;
    }

    void
    PageWidget::contentsMousePressEvent(QMouseEvent* e)
    {
        if (m_doc == 0)
            return;
        if ( e->button() & LeftButton )
        {
            m_dragGrabPos = e -> globalPos();
            setCursor( sizeAllCursor );
        }
        else if ( e->button() & RightButton )
        {
            emit rightClick();
        }

        m_pressedAction = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
    }

    void
    PageWidget::contentsMouseReleaseEvent(QMouseEvent* e)
    {
        if (m_doc == 0)
            return;

        if ( e -> button() & LeftButton )
        {
            setCursor( arrowCursor );
        }
        else
        {
            LinkAction* action = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
            if (action == m_pressedAction)
                emit linkClicked(action);

            m_pressedAction = 0;
        }
    }

    void
    PageWidget::contentsMouseMoveEvent(QMouseEvent* e)
    {
        if (m_doc == 0)
            return;
        if ( e->state() & LeftButton )
        {
            QPoint delta = m_dragGrabPos - e->globalPos();
            scrollBy( delta.x(), delta.y() );
            m_dragGrabPos = e->globalPos();
        }
        else
        {
            LinkAction* action = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
            setCursor(action != 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        }
    }

    void PageWidget::drawContents ( QPainter *p, int clipx, int clipy, int clipw, int cliph )
    {
        QPixmap * m_pixmap = NULL;
				QColor bc(KGlobalSettings::calculateAlternateBackgroundColor(KGlobalSettings::baseColor()));
        if (m_outputdev)
            m_pixmap = m_outputdev->getPixmap();
        if ( m_pixmap != NULL && ! m_pixmap->isNull() )
				{
            p->drawPixmap ( clipx, clipy, *m_pixmap, clipx, clipy, clipw, cliph );
            if (clipw > m_pixmap->width()) 
							p->fillRect ( m_pixmap->width(), clipy, clipw, cliph, bc );
						if (cliph > m_pixmap->height())
							p->fillRect ( clipx, m_pixmap->height() - clipy, clipw, cliph, bc );
				}
        else
            p->fillRect ( clipx, clipy, clipw, cliph, bc );
    }

    void PageWidget::nextPage()
    {
        setPage( getPage() + 1);
    }

    void PageWidget::previousPage()
    {
        setPage( getPage() - 1 );
    }

    void PageWidget::zoomTo( double _value )
    {
        if ( m_zoomFactor != _value)
        {
            m_zoomFactor = _value;
            updatePixmap();
        }
    }

    void PageWidget::zoomIn()
    {
        m_zoomFactor += 0.1;
        updatePixmap();
    }

    void PageWidget::zoomOut()
    {
        if ( (m_zoomFactor-0.1)<0.1 )
            return;
        m_zoomFactor -= 0.1;
        updatePixmap();
    }

    void PageWidget::setPage(int page)
    {
        static QMutex mutex;

        Q_ASSERT(mutex.locked() == false);
        mutex.lock();
        if (m_doc)
        {
            m_currentPage = KMAX(0, KMIN( page, m_doc->getNumPages()));
        } else {
            m_currentPage = 0;
        }
        updatePixmap();
        mutex.unlock();
    }

    void PageWidget::enableScrollBars( bool b )
    {
        setHScrollBarMode( b ? Auto : AlwaysOff );
        setVScrollBarMode( b ? Auto : AlwaysOff );
    }

    void PageWidget::scrollRight()
    {
        horizontalScrollBar()->addLine();
    }

    void PageWidget::scrollLeft()
    {
        horizontalScrollBar()->subtractLine();
    }

    void PageWidget::scrollDown()
    {
        verticalScrollBar()->addLine();
    }

    void PageWidget::scrollUp()
    {
        verticalScrollBar()->subtractLine();
    }

    void PageWidget::scrollBottom()
    {
        verticalScrollBar()->setValue( verticalScrollBar()->maxValue() );
    }

    void PageWidget::scrollTop()
    {
        verticalScrollBar()->setValue( verticalScrollBar()->minValue() );
    }

    void PageWidget::keyPressEvent( QKeyEvent* e )
    {
        switch ( e->key() ) {
        case Key_Up:
            scrollUp();
            break;
        case Key_Down:
            scrollDown();
            break;
        case Key_Left:
            scrollLeft();
            break;
        case Key_Right:
            scrollRight();
            break;
        case Key_Space:
        {
            if( e->state() != ShiftButton ) {
                emit spacePressed();
            }
        }
        default:
            e->ignore();
            return;
        }
        e->accept();
    }

    bool PageWidget::atTop() const
    {
        return verticalScrollBar()->value() == verticalScrollBar()->minValue();
    }

    bool PageWidget::atBottom() const
    {
        return verticalScrollBar()->value() == verticalScrollBar()->maxValue();
    }

    void PageWidget::wheelEvent( QWheelEvent *e )
    {
        int delta = e->delta();
        e->accept();
        if ((e->state() & ControlButton) == ControlButton) {
            if ( e->delta() > 0 )
                emit ZoomOut();
            else
                emit ZoomIn();
        }
        else if ( delta <= -120 && atBottom() )
        {
            emit ReadDown();
        }
        else if ( delta >= 120 && atTop())
        {
            emit ReadUp();
        }

        else
            QScrollView::wheelEvent( e );
    }

    void PageWidget::updatePixmap()
    {
        if ( m_doc )
        {
            const double pageWidth  = m_doc->getPageWidth (m_currentPage) * m_zoomFactor;
            const double pageHeight = m_doc->getPageHeight(m_currentPage) * m_zoomFactor;

            // Pixels per point when the zoomFactor is 1.
            const float basePpp  = QPaintDevice::x11AppDpiX() / 72.0;

            const float ppp = basePpp * m_zoomFactor; // pixels per point

            m_doc->displayPage(m_outputdev, m_currentPage, int(ppp * 72.0), 0, true);

            resizeContents ( m_outputdev->getPixmap()->width ( ), m_outputdev->getPixmap()->height ( ));

            viewport()->update();
        }
    }

    bool PageWidget::readUp()
    {
        if( atTop() )
            return false;
        else {
            int newValue = QMAX( verticalScrollBar()->value() - height() + 50,
                                 verticalScrollBar()->minValue() );

            /*
              int step = 10;
              int value = verticalScrollBar()->value();
              while( value > newValue - step ) {
              verticalScrollBar()->setValue( value );
              value -= step;
              }
            */

            verticalScrollBar()->setValue( newValue );
            return true;
        }
    }

    void PageWidget::dropEvent( QDropEvent* ev )
    {
        KURL::List lst;
        if (  KURLDrag::decode(  ev, lst ) ) {
            emit urlDropped( lst.first() );
        }
    }

    void PageWidget::dragEnterEvent( QDragEnterEvent * ev )
    {
        ev->accept();
    }

    bool PageWidget::readDown()
    {
        if( atBottom() )
            return false;
        else {
            int newValue = QMIN( verticalScrollBar()->value() + height() - 50,
                                 verticalScrollBar()->maxValue() );

            /*
              int step = 10;
              int value = verticalScrollBar()->value();
              while( value < newValue + step ) {
              verticalScrollBar()->setValue( value );
              value += step;
              }
            */

            verticalScrollBar()->setValue( newValue );
            return true;
        }
    }
}

// vim:ts=2:sw=2:tw=78:et
