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
#include <qtimer.h>
#include <qpushbutton.h>

#include <kglobalsettings.h> 
#include <kiconloader.h>
#include <kurldrag.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kconfigbase.h>

#include "kpdf_pagewidget.h"
#include "page.h"

PageWidget::PageWidget(QWidget *parent, KPDFDocument *document)
	: QScrollView(parent, "KPDF::pageWidget", WRepaintNoErase),
	m_document( document ), m_page( 0 ),
	m_pageWidth( 0 ), m_pageHeight( 0 ),
	m_mouseMode( MouseNormal ),
	m_zoomMode( FixedFactor ), m_zoomFactor( 1.0 ),
	m_delayTimer( 0 )
{
	// widget setup
	setMouseTracking( true );
	setAcceptDrops( true );
	setFocusPolicy( QWidget::StrongFocus );
	viewport()->setFocusPolicy( QWidget::WheelFocus );

	// set a corner button to resize the view to the page size
	QPushButton * resizeButton = new QPushButton( viewport() );
	resizeButton->setPixmap( SmallIcon("crop" /*"top"*/) );
	setCornerWidget( resizeButton );
	// connect(...);
}

void PageWidget::setupActions( KActionCollection * ac, KConfigGroup * config )
{
	// Zoom actions ( higher scales consumes lots of memory! )
	const double zoomValue[10] = {0.125,0.25,0.3333,0.5,0.6667,0.75,1,1.25,1.50,2 };

	m_aZoom = new KSelectAction( i18n( "Zoom" ), "viewmag", 0, ac, "zoomTo" );
	connect( m_aZoom, SIGNAL( activated( const QString & ) ), this, SLOT( slotZoom( const QString& ) ) );
	m_aZoom->setEditable(  true );
	m_aZoom->clear();

	QStringList translated;
	QString localValue;
	QString double_oh( "00" );
	int idx = 0;
	for ( int i = 0; i < 10; i++ )
	{
		localValue = KGlobal::locale()->formatNumber( zoomValue[i] * 100.0, 2 );
		localValue.remove( KGlobal::locale()->decimalSymbol() + double_oh );
		translated << QString( "%1%" ).arg( localValue );
		if ( zoomValue[i] == 1.0 )
			idx = i;
	}
	m_aZoom->setItems( translated );
	m_aZoom->setCurrentItem( idx );

	KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

	KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

	m_aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), "viewmagfit", 0, ac, "fit_to_width" );
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


//BEGIN KPDFDocumentObserver inherited methods 
void PageWidget::pageSetup( const QValueList<int> & pages )
{
	m_pages.clear();
	m_page = 0;

	if ( pages.count() < 1 )
		return slotUpdateView();

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

	slotUpdateView();
	verticalScrollBar()->setValue( (int)(position * verticalScrollBar()->maxValue()) );
	// TODO : move request (Async or sync ?) to updateview.. check this
	m_document->requestPixmap( pageNumber, m_pageWidth, m_pageHeight, true );
}

void PageWidget::notifyPixmapChanged( int pageNumber )
{
	// check if it's the preview we're waiting for and update it
	if ( m_page && (int)m_page->number() == pageNumber )
		slotUpdateView();
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

void PageWidget::viewportResizeEvent( QResizeEvent * )
{
	if ( !m_delayTimer )
	{
		m_delayTimer = new QTimer( this );
		connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotUpdateView() ) );
	}
	m_delayTimer->start( 400, true );
}

void PageWidget::keyPressEvent( QKeyEvent * e )
{
	switch ( e->key() )
	{
	case Key_Up:
		if ( atTop() )
			scrollUp();
		else
			verticalScrollBar()->subtractLine();
		break;
	case Key_Down:
		if ( atBottom() )
			scrollDown();
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
			scrollDown();
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
	if ( (e->state() & ControlButton) == ControlButton ) {
		if ( e->delta() > 0 )
			slotZoomOut();
		else
			slotZoomIn();
	}
	else if ( delta <= -120 && atBottom() )
		scrollDown();
	else if ( delta >= 120 && atTop() )
		scrollUp();
	else
		QScrollView::wheelEvent( e );
}

void PageWidget::dragEnterEvent( QDragEnterEvent * ev )
{
	ev->accept();
}

void PageWidget::dropEvent( QDropEvent * ev )
{
	KURL::List lst;
	if (  KURLDrag::decode(  ev, lst ) )
		emit urlDropped( lst.first() );
}

void PageWidget::drawContents ( QPainter *p, int clipx, int clipy, int clipw, int cliph )
{
	QColor bc( Qt::lightGray /*KGlobalSettings::calculateAlternateBackgroundColor( KGlobalSettings::baseColor() )*/ );
	if ( m_page )
	{
		// draw the page (and overlays such as hilighting and selection)
		QRect clipRect( clipx, clipy, clipw, cliph );
		QRect pageRect( 0,0, m_pageWidth, m_pageHeight );
		p->save();
		m_page->drawPixmap( p, clipRect.intersect(pageRect), m_pageWidth, m_pageHeight );
		p->restore();

		// fill the remaining area
		if ( (clipx + clipw) > m_pageWidth )
			p->fillRect ( m_pageWidth, clipy, clipw - (m_pageWidth - clipx), m_pageHeight - clipy, bc );
		if ( (cliph + clipy) > m_pageHeight )
			p->fillRect ( clipx, m_pageHeight, clipw, cliph - (m_pageHeight - clipy), bc );
	}
	else
		p->fillRect ( clipx, clipy, clipw, cliph, bc );
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
		m_zoomMode = FixedFactor;
		m_zoomFactor = zoom;
		slotUpdateView();
		m_aZoomFitWidth->setChecked( false );
	}
}

void PageWidget::slotZoomIn()
{
	if ( m_zoomFactor >= 4.0 )
		return;
	m_zoomFactor += 0.1;
	if ( m_zoomFactor >= 4.0 )
		m_zoomFactor = 4.0;

	m_zoomMode = FixedFactor;
	slotUpdateView();
	m_aZoomFitWidth->setChecked( false );
}

void PageWidget::slotZoomOut()
{
	if ( m_zoomFactor <= 0.125 )
		return;
	m_zoomFactor -= 0.1;
	if ( m_zoomFactor <= 0.125 )
		m_zoomFactor = 0.125;

	m_zoomMode = FixedFactor;
	slotUpdateView();
	m_aZoomFitWidth->setChecked( false );
}

void PageWidget::slotFitToWidthToggled( bool on )
{
	m_zoomMode = on ? FitWidth : FixedFactor;
	slotUpdateView();
}

void PageWidget::slotToggleScrollBars( bool on )
{
	setHScrollBarMode( on ? AlwaysOn : AlwaysOff );
	setVScrollBarMode( on ? AlwaysOn : AlwaysOff );
}

void PageWidget::slotUpdateView( bool /*forceRepaint*/ )
{	//TODO ASYNC autogeneration!
	if ( !m_page )
		resizeContents( 0, 0 );
	else
	{
		// Zoom / AutoFit-Width / AutoFit-Page
		double scale = m_zoomFactor;
		if ( m_zoomMode == FitWidth || m_zoomMode == FitPage )
		{
			scale = (double)viewport()->width() / (double)m_page->width();
			if ( m_zoomMode == FitPage )
			{
				double scaleH = (double)viewport()->height() / (double)m_page->height();
				if ( scaleH < scale )
					scale = scaleH;
			}
		}
		m_pageWidth = (int)( scale * m_page->width() );
		m_pageHeight = (int)( scale * m_page->height() );
		resizeContents( m_pageWidth, m_pageHeight );
		m_document->requestPixmap( m_page->number(), m_pageWidth, m_pageHeight, true );
	}
	viewport()->update();
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
	if( atTop() && m_vectorIndex > 0 )
		// go to the bottom of previous page
		m_document->slotSetCurrentPagePosition( m_pages[ m_vectorIndex - 1 ], 1.0 );
	else
	{   // go towards the top of current page
		int newValue = QMAX( verticalScrollBar()->value() - height() + 50,
		                     verticalScrollBar()->minValue() );
		verticalScrollBar()->setValue( newValue );
	}
}

void PageWidget::scrollDown()
{
	if( atBottom() && m_vectorIndex < (int)m_pages.count() - 1 )
		// go to the top of previous page
		m_document->slotSetCurrentPagePosition( m_pages[ m_vectorIndex + 1 ], 0.0 );
	else
	{	// go towards the bottom of current page
		int newValue = QMIN( verticalScrollBar()->value() + height() - 50,
		                     verticalScrollBar()->maxValue() );
		verticalScrollBar()->setValue( newValue );
	}
}

#include "kpdf_pagewidget.moc"
