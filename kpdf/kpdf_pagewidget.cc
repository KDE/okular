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

PageWidget::PageWidget( QWidget *parent, KPDFDocument *document )
	: QScrollView( parent, "KPDF::pageWidget", WRepaintNoErase ),
	m_document( document ), m_page( 0 ),
	m_mouseMode( MouseNormal ), m_mouseOnLink( false ),
	m_zoomMode( FixedFactor ), m_zoomFactor( 1.0 ),
	m_delayTimer( 0 )
{
	// widget setup
	setAcceptDrops( true );
	setFocusPolicy( QWidget::StrongFocus );
	viewport()->setFocusPolicy( QWidget::WheelFocus );
	viewport()->setMouseTracking( true );

	// set a corner button to resize the view to the page size
	QPushButton * resizeButton = new QPushButton( viewport() );
	resizeButton->setPixmap( SmallIcon("crop" /*"top"*/) );
	setCornerWidget( resizeButton );
	resizeButton->setEnabled( false );
	// connect(...);
}

void PageWidget::setupActions( KActionCollection * ac, KConfigGroup * config )
{
	// Zoom actions ( higher scales consumes lots of memory! )
	const double zoomValue[10] = { 0.125, 0.25, 0.333, 0.5, 0.667, 0.75, 1, 1.25, 1.50, 2 };

	m_aZoom = new KSelectAction( i18n( "Zoom" ), "viewmag", 0, ac, "zoom_to" );
	connect( m_aZoom, SIGNAL( activated( const QString & ) ), this, SLOT( slotZoom( const QString& ) ) );
	m_aZoom->setEditable(  true );

	QStringList translated;
	translated << i18n("Fit Width") << i18n("Fit Page");
	QString localValue;
	QString double_oh( "00" );
	for ( int i = 0; i < 10; i++ )
	{
		localValue = KGlobal::locale()->formatNumber( zoomValue[i] * 100.0, 2 );
		localValue.remove( KGlobal::locale()->decimalSymbol() + double_oh );
		translated << QString( "%1%" ).arg( localValue );
	}
	m_aZoom->setItems( translated );
	m_aZoom->setCurrentItem( 8 );

	KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

	KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

	m_aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), "viewmagfit", 0, ac, "zoom_fit_width" );
	connect( m_aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

	m_aZoomFitPage = new KToggleAction( i18n("Fit to &Page"), "viewmagfit", 0, ac, "zoom_fit_page" );
	connect( m_aZoomFitPage, SIGNAL( toggled( bool ) ), SLOT( slotFitToPageToggled( bool ) ) );

	// Mouse-Mode actions
	KToggleAction * mn = new KToggleAction( i18n("Normal"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
	mn->setExclusiveGroup("MouseType");
	mn->setChecked( true );

	KToggleAction * ms = new KToggleAction( i18n("Select"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
	ms->setExclusiveGroup("MouseType");

	KToggleAction * md = new KToggleAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
	md->setExclusiveGroup("MouseType");

	// Other actions
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
}

void PageWidget::notifyPixmapChanged( int pageNumber )
{
	// check if it's the preview we're waiting for and update it
	if ( m_page && (int)m_page->number() == pageNumber )
		slotUpdateView();
}
//END KPDFDocumentObserver inherited methods

//BEGIN widget events 
void PageWidget::contentsMousePressEvent( QMouseEvent * e )
{
	switch ( m_mouseMode )
	{
	case MouseNormal:    // drag / click start
		if ( e->button() & LeftButton )
		{
			m_mouseGrabPos = e->globalPos();
			setCursor( sizeAllCursor );
		}
		else if ( e->button() & RightButton )
			emit rightClick();

		/* TODO Albert
			note: 'Page' is an 'information container' and has to deal with clean
			data (such as the '(int)page & (float)position' where link refers to,
			not a LinkAction struct.. better an own struct). 'Document' is the place
			to put xpdf/Splash dependant stuff and fill up pages with interpreted
			data. I think is a *clean* way to handle everything.
			m_pressedLink = *PAGE* or *DOCUMENT* ->findLink( normalizedX, normY );
		*/
		break;

	case MouseSelection: // ? set 1st corner of the selection rect ?

	case MouseEdit:      // ? place the beginning of [tool] ?
		break;
	}
}

void PageWidget::contentsMouseReleaseEvent( QMouseEvent * )
{
	switch ( m_mouseMode )
	{
	case MouseNormal:    // end drag / follow link
		setCursor( arrowCursor );

		/* TODO Albert
			PageLink * link = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
			if ( link == m_pressedLink )
				//go to link, use:
				document->slotSetCurrentPagePosition( (int)link->page(), (float)link->position() );
				//and all the views will update and display the right page at the right position
			m_pressedLink = 0;
		*/
		break;

	case MouseSelection: // ? m_page->setPixmapOverlaySelection( QRect ) ?

	case MouseEdit:      // ? apply [tool] ?
		break;
	}
}

void PageWidget::contentsMouseMoveEvent( QMouseEvent * e )
{
	switch ( m_mouseMode )
	{
	case MouseNormal:    // move page / change mouse cursor if over links
		if ( e->state() & LeftButton )
		{
			QPoint delta = m_mouseGrabPos - e->globalPos();
			scrollBy( delta.x(), delta.y() );
			m_mouseGrabPos = e->globalPos();
		}
		/* TODO Albert
			LinkAction* action = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
			setCursor(action != 0 ? );
			experimental version using Page->hasLink( int pageX, int pageY )
			and haslink has a fake true response on
		*/
		if ( m_page && e->state() == NoButton && m_pageRect.isValid() )
		{
			bool onLink = m_page->hasLink( e->x() - m_pageRect.left(), e->y() - m_pageRect.top() );
			// set cursor only when entering / leaving (setCursor has not an internal cache)
			if ( onLink != m_mouseOnLink )
			{
				m_mouseOnLink = onLink;
				setCursor( onLink ? pointingHandCursor : arrowCursor );
			}
		}
		break;

	case MouseSelection: // ? update selection contour ?

	case MouseEdit:      // ? update graphics ?
		break;
	}
}

void PageWidget::viewportResizeEvent( QResizeEvent * )
{
	// start a timer that will refresh the pixmap after 0.5s
	if ( !m_delayTimer )
	{
		m_delayTimer = new QTimer( this );
		connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotUpdateView() ) );
	}
	m_delayTimer->start( 400, true );
	// recalc coordinates
	slotUpdateView( false );
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
	QColor bc( paletteBackgroundColor() /*KGlobalSettings::calculateAlternateBackgroundColor( KGlobalSettings::baseColor() )*/ );
	if ( m_page )
	{
		QRect clip( clipx, clipy, clipw, cliph );
		QRect pageClip( clip.intersect( m_pageRect ) );

		// draw the page (and overlays such as hilighting and selection)
		if ( pageClip.isValid() )
		{
			p->save();
			p->translate( m_pageRect.left(), m_pageRect.top() );
			QRect translatedPageClip( pageClip );
			translatedPageClip.moveBy( -m_pageRect.left(), -m_pageRect.top() );
			m_page->drawPixmap( p, translatedPageClip, m_pageRect.width(), m_pageRect.height() );
			p->restore();
		}

		// if drawn region includes an edge of the page
		if ( pageClip != clip )
		{
			// draw the outline and adapt pageClip
			pageClip.moveBy( -1, -1 );
			pageClip.setWidth( pageClip.width() + 2 );
			pageClip.setHeight( pageClip.height() + 2 );
			p->setPen( Qt::black );
			p->drawRect( pageClip );
			p->setPen( Qt::gray );
			p->drawLine( pageClip.left(), pageClip.bottom() + 1,  pageClip.right() + 1, pageClip.bottom() + 1 );
			p->drawLine( pageClip.right() + 1, pageClip.top(), pageClip.right() + 1, pageClip.bottom() + 1 );
			p->setPen( Qt::lightGray );
			p->drawLine( pageClip.left(), pageClip.bottom() + 2,  pageClip.right() + 2, pageClip.bottom() + 2 );
			p->drawLine( pageClip.right() + 2, pageClip.top(), pageClip.right() + 2, pageClip.bottom() + 2 );
			p->setPen( bc );
			p->drawPoint( pageClip.left(), pageClip.bottom() + 2 );
			p->drawPoint( pageClip.right() + 2, pageClip.top() );
			pageClip.setWidth( pageClip.width() + 2 );
			pageClip.setHeight( pageClip.height() + 2 );

			// fill the remaining area (left side, right side, top crop, bottom crop)
			if ( clip.left() < pageClip.left() )
				p->fillRect( clip.left(), clip.top(), pageClip.left() - clip.left(), clip.height(), bc );
			if ( clip.right() > pageClip.right() )
				p->fillRect( pageClip.right() + 1, clip.top(), clip.right() - pageClip.right(), clip.height(), bc );
			if ( clip.top() < pageClip.top() )
				p->fillRect( pageClip.left(), clip.top(), pageClip.width(), pageClip.top() - clip.top(), bc );
			if ( clip.bottom() > pageClip.bottom() )
				p->fillRect( pageClip.left(), pageClip.bottom() + 1, pageClip.width(), clip.bottom() - pageClip.bottom(), bc );
		}
	}
	else
		p->fillRect ( clipx, clipy, clipw, cliph, bc );
}
//END widget events

//BEGIN internal SLOTS
void PageWidget::slotZoom( const QString & nz )
{
	if ( nz == i18n("Fit Width") )
	{
		m_aZoomFitWidth->setChecked( true );
		return slotFitToWidthToggled( true );
	}
	if ( nz == i18n("Fit Page") )
	{
		m_aZoomFitPage->setChecked( true );
		return slotFitToPageToggled( true );
	}

	QString z = nz;
	z.remove( z.find( '%' ), 1 );
	bool isNumber = true;
	double zoom = KGlobal::locale()->readNumber(  z, &isNumber ) / 100;
	
	if ( m_zoomFactor != zoom && zoom > 0.1 && zoom < 8.0 )
	{
		m_zoomMode = FixedFactor;
		m_zoomFactor = zoom;
		slotUpdateView();
		m_aZoomFitWidth->setChecked( false );
		m_aZoomFitPage->setChecked( false );
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
	m_aZoomFitPage->setChecked( false );
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
	m_aZoomFitPage->setChecked( false );
}

void PageWidget::slotFitToWidthToggled( bool on )
{
	m_zoomMode = on ? FitWidth : FixedFactor;
	slotUpdateView();
	m_aZoomFitPage->setChecked( false );
}

void PageWidget::slotFitToPageToggled( bool on )
{
	m_zoomMode = on ? FitPage : FixedFactor;
	slotUpdateView();
	m_aZoomFitWidth->setChecked( false );
}

void PageWidget::slotSetMouseNormal()
{
	m_mouseMode = MouseNormal;
}

void PageWidget::slotSetMouseSelect()
{
	m_mouseMode = MouseSelection;
}

void PageWidget::slotSetMouseDraw()
{
	m_mouseMode = MouseEdit;
}

void PageWidget::slotToggleScrollBars( bool on )
{
	setHScrollBarMode( on ? AlwaysOn : AlwaysOff );
	setVScrollBarMode( on ? AlwaysOn : AlwaysOff );
}

void PageWidget::slotUpdateView( bool repaint )
{	//TODO ASYNC autogeneration!
	if ( !m_page )
	{
		m_pageRect.setRect( 0, 0, 0, 0 );
		resizeContents( 0, 0 );
	}
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
		int pageW = (int)( scale * m_page->width() ),
		    pageH = (int)( scale * m_page->height() ),
		    viewW = QMAX( viewport()->width(), pageW ),
		    viewH = QMAX( viewport()->height(), pageH );
		m_pageRect.setRect( (viewW - pageW) / 2, (viewH - pageH) / 2, pageW, pageH );
		resizeContents( viewW, viewH );
		m_document->requestPixmap( m_page->number(), pageW, pageH, true );
	}
	if ( repaint )
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
