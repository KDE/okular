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

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kconfigbase.h>

#include "PDFDoc.h"

#include "kpdf_pagewidget.h"
#include "kpdf_pagewidget.moc"
#include "QOutputDev.h"
#include <qpushbutton.h>
#include <kiconloader.h>

namespace KPDF
{
    PageWidget::PageWidget(QWidget *parent, KPDFDocument *doc)
        : QScrollView(parent, "pageWidget", WRepaintNoErase),
          m_doc(0),
          m_zoomFactor( 1.0 ),
          m_document( doc ),
          m_currentPage( 1 ),
          m_pressedAction( 0 ),
          m_selection( false )
    {
        SplashColor paperColor;
        paperColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
        m_outputdev = new QOutputDev(paperColor);
        setFocusPolicy( QWidget::StrongFocus );
        setAcceptDrops( true );
        viewport()->setFocusPolicy( QWidget::WheelFocus );

        QPushButton * w = new QPushButton( viewport() );
        w->setPixmap( SmallIcon("up") );
        setCornerWidget( w );
    }

    PageWidget::~PageWidget()
    {
        delete m_outputdev;
    }

void PageWidget::pageSetup( const QValueList<int> & /*pages*/ )
{
/*
	m_doc = doc;
	m_outputdev -> startDoc(doc->getXRef());
	m_currentPage = 1;
	updatePixmap();
*/
}

void PageWidget::pageSetCurrent( int /*pageNumber*/, float /*position*/ )
{
/*
	// any idea why this mutex is necessary?
	static QMutex mutex;
	
	m_selection = false;
	Q_ASSERT(mutex.locked() == false);
	mutex.lock();
	if (m_doc)
	{
		m_currentPage = pageNumber;
	} else {
		m_currentPage = 0;
	}
	updatePixmap();
	mutex.unlock();
*/
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
    {return;
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

    void PageWidget::enableScrollBars( bool b )
    {
        setHScrollBarMode( b ? Auto : AlwaysOff );
        setVScrollBarMode( b ? Auto : AlwaysOff );
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
			verticalScrollBar()->subtractLine();
		break;
	case Key_Down:
		if ( atBottom() )
			emit ReadDown();
		else
			verticalScrollBar()->addLine();
		break;
	case Key_Left:
		horizontalScrollBar()->subtractLine();
		break;
	case Key_Right:
		horizontalScrollBar()->addLine();
		break;
	case Key_Space:
		if( e->state() != ShiftButton )
			emit spacePressed();
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
            //const float ppp = (float)QPaintDevice::x11AppDpiX() * m_zoomFactor; // pixels per point

            //m_docMutex->lock();
            //m_doc->displayPage(m_outputdev, m_currentPage, ppp, ppp, 0, true, true);
            //m_docMutex->unlock();

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

void PageWidget::setupActions( KActionCollection * ac, KConfigGroup * config )
{
	// Zoom actions
	const double zoomValue[14] = {0.125,0.25,0.3333,0.5,0.6667,0.75,1,1.25,1.50,2,3,4,6,8 };

	m_zoomTo = new KSelectAction(  i18n( "Zoom" ), "zoomTo", 0, ac, "zoomTo" );
	connect( m_zoomTo, SIGNAL(  activated(  const QString & ) ), this, SLOT(  slotZoom( const QString& ) ) );
	m_zoomTo->setEditable(  true );
	m_zoomTo->clear();

	QStringList translated;
	QString localValue;
	QString double_oh("00");
	int idx = 0;
	int cur = 0;
	for ( int i = 0; i < 10;i++)
	{
		localValue = KGlobal::locale()->formatNumber( zoomValue[i] * 100.0, 2 );
		localValue.remove( KGlobal::locale()->decimalSymbol()+double_oh );
	
		translated << QString( "%1%" ).arg( localValue );
		if ( zoomValue[i] == 1.0 )
			idx = cur;
		++cur;
	}
	m_zoomTo->setItems( translated );
	m_zoomTo->setCurrentItem( idx );

	KStdAction::zoomIn( this, SLOT(slotZoomIn()), ac, "zoom_in" );

	KStdAction::zoomOut( this, SLOT(slotZoomOut()), ac, "zoom_out" );

	m_fitToWidth = new KToggleAction( i18n("Fit to Page &Width"), 0, ac, "fit_to_width" );
	connect( m_fitToWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

	m_showScrollBars = new KToggleAction( i18n( "Show &Scrollbars" ), 0, ac, "show_scrollbars" );
	m_showScrollBars->setCheckedState(i18n("Hide &Scrollbars"));
	connect( m_showScrollBars, SIGNAL( toggled( bool ) ), SLOT( slotToggleScrollBars( bool ) ) );

	m_showScrollBars->setChecked( config->readBoolEntry( "ShowScrollBars", true ) );
	slotToggleScrollBars( m_showScrollBars->isChecked() );
}

void PageWidget::saveSettings( KConfigGroup * config )
{
	config->writeEntry( "ShowScrollBars", m_showScrollBars->isChecked() );
}

void PageWidget::slotZoom( const QString & nz )
{
	QString z = nz;
	z.remove( z.find( '%' ), 1 );
	bool isNumber = true;
	double zoom = KGlobal::locale()->readNumber(  z, &isNumber ) / 100;
zoom = 0;//CUT WARNINGS (but remove me:-)
/*	if ( isNumber )
		document->slotSetZoom( zoom );*/
}

void PageWidget::slotZoomIn()
{
// 	document->slotChangeZoom( 0.1 );
}

void PageWidget::slotZoomOut()
{
// 	document->slotChangeZoom( -0.1 );
}

void PageWidget::slotFitToWidthToggled( bool /*fit*/ )
{
/*
	m_zoomMode = m_fitToWidth->isChecked() ? FitWidth : FixedFactor;
	displayPage(m_currentPage);
*/
}

void PageWidget::slotToggleScrollBars( bool show )
{
	enableScrollBars( show );
}

void PageWidget::slotSetZoom( float /*zoom*/ )
{
}

void PageWidget::slotChangeZoom( float /*offset*/ )
{
}

}

/** TO BE IMPORTED 
void Part::displayPage( int pageNumber )
{
    if (pageNumber <= 0 || pageNumber > m_doc->getNumPages())
        return;
    updateActions();
    const double pageWidth  = m_doc->getPageWidth (pageNumber) * m_zoomFactor;
    const double pageHeight = m_doc->getPageHeight(pageNumber) * m_zoomFactor;

    // Pixels per point when the zoomFactor is 1.
    const float basePpp  = QPaintDevice::x11AppDpiX() / 72.0;

    switch (m_zoomMode)
    {
    case FitWidth:
    {
        const double pageAR = pageWidth/pageHeight; // Aspect ratio

        const int canvasWidth    = m_pageWidget->contentsRect().width();
        const int canvasHeight   = m_pageWidget->contentsRect().height();
        const int scrollBarWidth = m_pageWidget->verticalScrollBar()->width();

        // Calculate the height so that the page fits the viewport width
        // assuming that we need a vertical scrollbar.
        float height = float(canvasWidth - scrollBarWidth) / pageAR;

        // If the vertical scrollbar wasn't needed after all, calculate the page
        // size so that the page fits the viewport width without the scrollbar.
        if (ceil(height) <= canvasHeight)
        {
            height = float(canvasWidth) / pageAR;

            // Handle the rare case that enlarging the page resulted in the need of
            // a vertical scrollbar. We can fit the page to the viewport height in
            // this case.
            if (ceil(height) > canvasHeight)
                height = float(canvasHeight) * pageAR;
        }

        m_zoomFactor = (height / pageHeight) / basePpp;
        break;
    }
    case FixedFactor:
    default:
        break;
    }

//const float ppp = basePpp * m_zoomFactor; // pixels per point
//  m_doc->displayPage(m_pageWidget, pageNumber, int(m_zoomFactor * ppp * 72.0), 0, true);
//  m_pageWidget->show();
//  m_currentPage = pageNumber;
}
*/

// vim:ts=2:sw=2:tw=78:et
