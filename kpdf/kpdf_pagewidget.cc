/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by James Ots <kde@jamesots.com>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qcursor.h>
#include <qpainter.h>
#include <qmutex.h>

#include <kglobalsettings.h> 
#include <kurldrag.h>

#include "PDFDoc.h"

#include "kpdf_pagewidget.h"
#include "kpdf_pagewidget.moc"
#include "QOutputDev.h"
#include <qpushbutton.h>
#include <kiconloader.h>

namespace KPDF
{
    PageWidget::PageWidget(QWidget* parent, const char* name, QMutex *docMutex)
        : QScrollView(parent, name, WRepaintNoErase),
          m_doc(0),
          m_docMutex(docMutex),
          m_zoomFactor( 1.0 ),
          m_currentPage( 1 ),
          m_pressedAction( 0 ),
          m_selection( false )
    {
        SplashColor paperColor;
        paperColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
        m_outputdev = new QOutputDev(paperColor);
        setFocusPolicy( QWidget::StrongFocus );
        viewport()->setFocusPolicy( QWidget::WheelFocus );

	QPushButton * w = new QPushButton( viewport() );
	w->setPixmap( SmallIcon("up") );
	setCornerWidget( w );
    }
    PageWidget::~PageWidget()
    {
        delete m_outputdev;
    }
    void
    PageWidget::setPDFDocument(PDFDoc* doc)
    {
        m_doc = doc;
        m_outputdev -> startDoc(doc->getXRef());
        m_currentPage = 1;
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
        QImage im;
        QColor bc(KGlobalSettings::calculateAlternateBackgroundColor(KGlobalSettings::baseColor()));
        if (m_outputdev)
            im = m_outputdev->getImage();
        if ( !im.isNull() )
        {
            p->drawImage ( clipx, clipy, im, clipx, clipy, clipw, cliph );
            if ((clipw + clipx) > im.width()) 
                p->fillRect ( im.width(), 
                        clipy, 
                        clipw - (im.width() - clipx), 
                        im.height() - clipy, 
                        bc );
            if ((cliph + clipy) > im.height())
                p->fillRect ( clipx, 
                        im.height(), 
                        clipw, 
                        cliph - (im.height() - clipy), 
                        bc );
            if (m_selection)
            {
                p->setBrush(Qt::SolidPattern);
                p->setPen(QPen(Qt::black, 1)); // should not be necessary bug a Qt bug makes it necessary
                p->setRasterOp(Qt::NotROP);
                p->drawRect(qRound(m_xMin*m_zoomFactor), qRound(m_yMin*m_zoomFactor), qRound((m_xMax- m_xMin)*m_zoomFactor), qRound((m_yMax- m_yMin)*m_zoomFactor));
            }
        }
        else
            p->fillRect ( clipx, clipy, clipw, cliph, bc );
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
        // any idea why this mutex is necessary?
        static QMutex mutex;
        
        m_selection = false;
        Q_ASSERT(mutex.locked() == false);
        mutex.lock();
        if (m_doc)
        {
            m_currentPage = page;
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
	    if ( atTop() )
		emit ReadUp();
	    else
                scrollUp();
            break;
        case Key_Down:
	    if ( atBottom() )
		emit ReadDown();
	    else
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
            // Pixels per point when the zoomFactor is 1.
            const float basePpp  = QPaintDevice::x11AppDpiX() / 72.0;

            const float ppp = basePpp * m_zoomFactor; // pixels per point

            m_docMutex->lock();
            m_doc->displayPage(m_outputdev, m_currentPage, ppp * 72.0, ppp * 72.0, 0, true, true);
            m_docMutex->unlock();

            resizeContents ( m_outputdev->getImage().width ( ), m_outputdev->getImage().height ( ));

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

    bool PageWidget::find(Unicode *u, int len, bool next)
    {
        bool b;
        if (!next)
        {
            // ensure we are searching the whole page
            m_xMin = 0;
            m_yMin = 0;
        }
        
        b = m_outputdev -> find(u, len, !next, true, next, false, &m_xMin, &m_yMin, &m_xMax, &m_yMax);
        m_xMin = m_xMin / m_zoomFactor;
        m_yMin = m_yMin / m_zoomFactor;
        m_xMax = m_xMax / m_zoomFactor;
        m_yMax = m_yMax / m_zoomFactor;
        m_selection = b;
        updateContents();
        return b;
    }

}

// vim:ts=2:sw=2:tw=78:et
