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

#include "kpdf_pagewidget.h"
#include "page.h"

namespace KPDF
{

PageWidget::PageWidget(QWidget *parent, KPDFDocument *document)
	: QScrollView(parent, "KPDF::pageWidget", WRepaintNoErase),
	m_document( document ),
	m_page( 0 ),
	m_mouseMode( MouseNormal ),
	m_zoomFactor( 1.0 )
{
	// widget setup
	setMouseTracking( true );
	setAcceptDrops( true );
	setFocusPolicy( QWidget::StrongFocus );
	viewport()->setFocusPolicy( QWidget::WheelFocus );

	//TODO following code will resize the view to page size.. cool!
	 //#include <qpushbutton.h> and <kiconloader.h>
	/*QPushButton * w = new QPushButton( viewport() );
	w->setPixmap( SmallIcon("up") );
	setCornerWidget( w );*/
}

PageWidget::~PageWidget()
{
	//TODO delete page(s) (sure ???)
}

void PageWidget::setupActions( KActionCollection * ac, KConfigGroup * config )
{
	// Zoom actions
	const double zoomValue[14] = {0.125,0.25,0.3333,0.5,0.6667,0.75,1,1.25,1.50,2,3,4,6,8 };

	m_aZoom = new KSelectAction(  i18n( "Zoom" ), "viewmag", 0, ac, "zoomTo" );
	connect( m_aZoom, SIGNAL(  activated(  const QString & ) ), this, SLOT(  slotZoom( const QString& ) ) );
	m_aZoom->setEditable(  true );
	m_aZoom->clear();

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
	m_aZoom->setItems( translated );
	m_aZoom->setCurrentItem( idx );

	KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

	KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

	m_aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), 0, ac, "fit_to_width" );
	connect( m_aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

	KToggleAction * ss = new KToggleAction( i18n( "Show &Scrollbars" ), 0, ac, "show_scrollbars" );
	ss->setCheckedState(i18n("Hide &Scrollbars"));
	connect( ss, SIGNAL( toggled( bool ) ), SLOT( slotToggleScrollBars( bool ) ) );

	ss->setChecked( config->readBoolEntry( "ShowScrollBars", true ) );
	slotToggleScrollBars( ss->isChecked() );
}

void PageWidget::saveSettings( KConfigGroup * config )
{
	config->writeEntry( "ShowScrollBars", hScrollBarMode() == AlwaysOn );
}


    bool PageWidget::find( Unicode * /*u*/, int /*len*/, bool /*next*/ )
    {return false; /* TODO !!restore!! Enrico
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
    */}


//BEGIN KPDFDocumentObserver inherited methods 
void PageWidget::pageSetup( const QValueList<int> & pages )
{
	m_pages.clear();
	m_page = 0;

	if ( pages.count() < 1 )
	{
		resizeContents( 0, 0 );
		return;
	}
	
	// populate internal vector with the list of pages and update
	QValueList<int>::const_iterator pageIt = pages.begin();
	QValueList<int>::const_iterator pageEnd = pages.end();
	for ( ; pageIt != pageEnd ; ++pageIt )
		m_pages.push_back( *pageIt );
}

void PageWidget::pageSetCurrent( int pageNumber, float position )
{
	// select next page
	m_vectorIndex = 0;
	m_page = 0;

	QValueVector<int>::iterator pagesIt = m_pages.begin();
	QValueVector<int>::iterator pagesEnd = m_pages.end();
	for ( ; pagesIt != pagesEnd; ++pagesIt )
	{
		if ( *pagesIt == pageNumber )
		{
			m_page = m_document->page( pageNumber );
			break;
		}
		m_vectorIndex++;
	}

	if ( !m_page || m_page->width() < 1 || m_page->height() < 1 )
		return;

	double pageWidth = m_zoomFactor * m_page->width();
	double pageHeight = m_zoomFactor * m_page->height();
	updatePixmap();
	resizeContents( (int)pageWidth, (int)pageHeight );
	verticalScrollBar()->setValue( (int)(position * verticalScrollBar()->maxValue()) );

	m_document->requestPixmap( pageNumber, (int)pageWidth, (int)pageHeight, true );
}

void PageWidget::notifyPixmapChanged( int pageNumber )
{
	// check if it's the preview we're waiting for
	if ( !m_page || (int)m_page->number() != pageNumber )
		return;

	printf("cel'ho\n");
	updatePixmap();
}
//END KPDFDocumentObserver inherited methods

//BEGIN widget events 
void PageWidget::contentsMousePressEvent(QMouseEvent* /*e*/)
{/* FIXME Albert
	if ( e->button() & LeftButton )
	{
		m_dragGrabPos = e -> globalPos();
		setCursor( sizeAllCursor );
	}
	else if ( e->button() & RightButton )
	{
		emit rightClick();
	}

	m_pressedAction = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
*/}

void PageWidget::contentsMouseReleaseEvent(QMouseEvent* /*e*/)
{/* FIXME Albert
	if ( e -> button() & LeftButton )
	{
		setCursor( arrowCursor );
	}
	else
	{
		LinkAction* action = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
		if (action == m_pressedAction)
			emit linkClicked(action);

		m_pressedAction = 0;
	}
*/}

void PageWidget::contentsMouseMoveEvent(QMouseEvent* /*e*/)
{/* FIXME Albert
	if ( e->state() & LeftButton )
	{
		QPoint delta = m_dragGrabPos - e->globalPos();
		scrollBy( delta.x(), delta.y() );
		m_dragGrabPos = e->globalPos();
	}
	else
	{
		LinkAction* action = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
		setCursor(action != 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
	}
*/}

void PageWidget::keyPressEvent( QKeyEvent* e )
{
	switch ( e->key() ) {
	case Key_Up:
		if ( atTop() )
			scrollUp(); //TODO direct document request to change page
		else
			verticalScrollBar()->subtractLine();
		break;
	case Key_Down:
		if ( atBottom() )
			scrollDown(); //TODO direct document request to change
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
			scrollDown(); //TODO direct document request to change
		break;
	default:
		e->ignore();
		return;
	}
	e->accept();
}

void PageWidget::wheelEvent( QWheelEvent *e )
{
	int delta = e->delta();
	e->accept();
	if ((e->state() & ControlButton) == ControlButton) {
		if ( e->delta() > 0 )
			emit slotZoomOut();
		else
			emit slotZoomIn();
	}
	else if ( delta <= -120 && atBottom() )
	{
		scrollDown(); //TODO direct document request to change
	}
	else if ( delta >= 120 && atTop())
	{
		scrollUp(); //TODO direct document request to change
	}

	else
		QScrollView::wheelEvent( e );
}

void PageWidget::dragEnterEvent( QDragEnterEvent * ev )
{
	ev->accept();
}

void PageWidget::dropEvent( QDropEvent* ev )
{
	KURL::List lst;
	if (  KURLDrag::decode(  ev, lst ) ) {
		emit urlDropped( lst.first() );
	}
}

void PageWidget::drawContents ( QPainter *p, int cx, int cy, int cw, int ch )
{
	if ( m_page )
		m_page->drawPixmap( p, QRect( cx,cy,cw,ch ), (int)m_page->width(), (int)m_page->height() );
/* TODO Enrico
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
*/
}
//END widget events

//BEGIN internal SLOTS
void PageWidget::slotZoom( const QString & nz )
{
	QString z = nz;
	z.remove( z.find( '%' ), 1 );
	bool isNumber = true;
	double zoom = KGlobal::locale()->readNumber(  z, &isNumber ) / 100;
	
	if ( m_zoomFactor != zoom )
	{
		m_zoomFactor = zoom;
		updatePixmap();
	}
}

void PageWidget::slotZoomIn()
{
	m_zoomFactor += 0.1;
	updatePixmap();
}

void PageWidget::slotZoomOut()
{
	if ( m_zoomFactor < 0.2 )
		return;
	m_zoomFactor -= 0.1;
	updatePixmap();
}

void PageWidget::slotFitToWidthToggled( bool /*on*/ )
{
	//m_zoomMode = on ? FitWidth : FixedFactor;
	updatePixmap();
}

void PageWidget::slotToggleScrollBars( bool on )
{
	setHScrollBarMode( on ? AlwaysOn : AlwaysOff );
	setVScrollBarMode( on ? AlwaysOn : AlwaysOff );
}
//END internal SLOTS

bool PageWidget::atTop() const
{
	return verticalScrollBar()->value() == verticalScrollBar()->minValue();
}

bool PageWidget::atBottom() const
{
	return verticalScrollBar()->value() == verticalScrollBar()->maxValue();
}

void PageWidget::scrollUp()
{
	if( atTop() )
	{
		if ( m_vectorIndex > 0 )
		{
			int nextPage = m_pages[ m_vectorIndex - 1 ];
			// go the previous page at bottom
			m_document->slotSetCurrentPagePosition( nextPage, 1.0 );
		}
	}
	else
	{
		int newValue = QMAX( verticalScrollBar()->value() - height() + 50,
		                     verticalScrollBar()->minValue() );
		verticalScrollBar()->setValue( newValue );
	}
}

void PageWidget::scrollDown()
{
	if( atBottom() )
	{
		if ( m_vectorIndex < (int)m_pages.count() - 1 )
		{
			int nextPage = m_pages[ m_vectorIndex + 1 ];
			// go the previous page at top
			m_document->slotSetCurrentPagePosition( nextPage, 0.0 );
		}
	} else {
		int newValue = QMIN( verticalScrollBar()->value() + height() - 50,
		                     verticalScrollBar()->maxValue() );
		verticalScrollBar()->setValue( newValue );
	}
}

void PageWidget::updatePixmap()
{
	if ( !m_page )
		resizeContents( 0, 0 );
	else
		resizeContents( (int)(m_page->width() * m_zoomFactor), (int)(m_page->height() * m_zoomFactor) );
	viewport()->update();
}

/** TO BE IMPORTED (Zoom code)

	const double pageWidth  = *PAGE* ->getPageWidth (pageNumber) * m_zoomFactor;
    const double pageHeight = *PAGE* ->getPageHeight(pageNumber) * m_zoomFactor;

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
*/
} //NAMESPACE END!

#include "kpdf_pagewidget.moc"
// vim:ts=2:sw=2:tw=78:et
