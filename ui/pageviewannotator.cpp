/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt / kde includes
#include <qfile.h>
#include <qcolor.h>
#include <qevent.h>
#include <qpainter.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kinputdialog.h>
#include <kuser.h>
#include <kdebug.h>

// system includes
#include <math.h>

// local includes
#include "core/document.h"
#include "core/page.h"
#include "core/annotations.h"
#include "conf/settings.h"
#include "pageview.h"
#include "pageviewutils.h"
#include "pageviewannotator.h"

/**
 * @short Engine: filter events to distill an Annotation.
 */
class AnnotatorEngine
{
    public:
        AnnotatorEngine( Annotation * annotation )
            : m_annotation( annotation ), m_creationCompleted( false ) {}
        virtual ~AnnotatorEngine() {};

        // enum definitions
        enum EventType { Press, Move, Release };
        enum Button { None, Left, Right };

        // perform operations
        virtual QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale ) = 0;
        virtual void paint( QPainter * painter, double xScale, double yScale, const QRect & clipRect ) = 0;
        virtual Annotation * end() { return m_annotation; }

        // query creation state
        //PageViewItem * editingItem() const { return m_lockedItem; }
        bool creationCompleted() const { return m_creationCompleted; }

    protected:
        Annotation * m_annotation;
        bool m_creationCompleted;
};

/** @short SmoothPathEngine */
class SmoothPathEngine : public AnnotatorEngine
{
    public:
        SmoothPathEngine( Annotation * annotation )
            : AnnotatorEngine( annotation ) {}

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale )
        {
            // only proceed if pressing left button
            if ( button != Left )
                return QRect();

            // start operation
            if ( type == Press && points.isEmpty() )
            {
                NormalizedPoint newPoint;
                m_annotation->r.left = m_annotation->r.right = newPoint.x = nX;
                m_annotation->r.top = m_annotation->r.bottom = newPoint.y = nY;
                points.append( newPoint );
                return QRect();
            }
            // add a point to the path
            else if ( type == Move && points.count() > 0 )
            {
                double dist = hypot( nX - points.last().x, nY - points.last().y );
                if ( dist < 0.01 )
                    return QRect();

                double dX = 2.0 / (double)xScale;
                double dY = 2.0 / (double)yScale;
                m_annotation->r.left = QMIN( m_annotation->r.left, nX - dX );
                m_annotation->r.top = QMIN( m_annotation->r.top, nY - dY );
                m_annotation->r.right = QMAX( nX + dX, m_annotation->r.right );
                m_annotation->r.bottom = QMAX( nY + dY, m_annotation->r.bottom );
                NormalizedPoint newPoint;
                newPoint.x = nX;
                newPoint.y = nY;
                points.append( newPoint );
            }
            // terminate process
            else if ( type == Release && points.count() > 0 )
            {
                if ( points.count() < 2 )
                    points.clear();
                else
                    m_creationCompleted = true;
            }
            return m_annotation->r.geometry( (int)xScale, (int)yScale );
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            // draw SmoothPaths whith at least 2 points
            if ( points.count() < 2 )
                return;

            // use annotation color for painting
            painter->setPen( QPen(m_annotation->color, 3) );

            QValueList<NormalizedPoint>::iterator pIt = points.begin(), pEnd = points.end();
            NormalizedPoint pA = *pIt;
            ++pIt;
            for ( ; pIt != pEnd; ++pIt )
            {
                NormalizedPoint pB = *pIt;
                painter->drawLine( (int)(pA.x * (double)xScale), (int)(pA.y * (double)yScale),
                                (int)(pB.x * (double)xScale), (int)(pB.y * (double)yScale) );
                pA = pB;
            }
        }

        Annotation * end()
        {
            /*switch ( annotation->annotationType() )
                case Annotation::AHighlight: */
            return m_annotation;
        }

    private:
        QValueList<NormalizedPoint> points;
};

/** @short PickPointEngine */
class PickPointEngine : public AnnotatorEngine
{
    public:
        PickPointEngine( Annotation * annotation, QString pixmapName )
            : AnnotatorEngine( annotation ), clicked( false )
        {
            pixmap = new QPixmap( DesktopIcon( pixmapName, 32 ) );
        }
        ~PickPointEngine()
        {
            delete pixmap;
        }

        QRect event( EventType type, Button /*button*/, double nX, double nY, double xScale, double yScale )
        {
            // only proceed if pressing left button
            //if ( button != Left )
            //    return QRect();

            // start operation on click
            if ( type == Press && clicked == false )
            {
                clicked = true;
            }
            // repaint if moving while pressing
            else if ( type == Move && clicked == true )
            {
            }
            // operation finished on release
            else if ( type == Release && clicked == true )
            {
                m_creationCompleted = true;
            }
            else
                return QRect();
            // update variable and annotation geometry (zoom invariant rect)
            point.x = nX;
            point.y = nY;
            m_annotation->r.left = nX - (16.0 / (double)xScale);
            m_annotation->r.right = nX + (17.0 / (double)xScale);
            m_annotation->r.top = nY - (16.0 / (double)yScale);
            m_annotation->r.bottom = nY + (17.0 / (double)yScale);

            return m_annotation->r.geometry( (int)xScale, (int)yScale );
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            // paint a dot if clicking on the page
            if ( !clicked )
                return;

            painter->setPen( QPen(m_annotation->color, 3) );
            int pX = (int)(point.x * (double)xScale);
            int pY = (int)(point.y * (double)yScale);
            if ( pixmap )
                painter->drawPixmap( pX - 15, pY - 15, *pixmap );
        }

    private:
        bool clicked;
        NormalizedPoint point;
        QPixmap * pixmap;
};

/** @short TwoPointsEngine */
class TwoPointsEngine : public AnnotatorEngine
{
    public:
        TwoPointsEngine( Annotation * annotation, bool block = false )
            : AnnotatorEngine( annotation ), m_block( block ) {}

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale )
        {
            // only proceed if pressing left button
            if ( button != Left )
                return QRect();

            // start operation
            if ( type == Press && points.isEmpty() )
            {
                NormalizedPoint newPoint;
                m_annotation->r.left = m_annotation->r.right = newPoint.x = nX;
                m_annotation->r.top = m_annotation->r.bottom = newPoint.y = nY;
                points.append( newPoint );
                return QRect();
            }
            // move the second point
            else if ( type == Move && !points.isEmpty() )
            {
                if ( points.count() == 2 )
                    points.pop_back();
                NormalizedPoint newPoint;
                newPoint.x = nX;
                newPoint.y = nY;
                points.append( newPoint );
                NormalizedPoint firstPoint = points.front();
                m_annotation->r.left = QMIN( firstPoint.x, nX ) - 2.0 / (double)xScale;
                m_annotation->r.right = QMAX( firstPoint.x, nX ) + 2.0 / (double)xScale;
                m_annotation->r.top = QMIN( firstPoint.y, nY ) - 2.0 / (double)yScale;
                m_annotation->r.bottom = QMAX( firstPoint.y, nY ) + 2.0 / (double)yScale;
            }
            // end creation if we have 2 points
            else if ( type == Release && points.count() == 2 )
                m_creationCompleted = true;

            return m_annotation->r.geometry( (int)xScale, (int)yScale );
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            if ( points.count() != 2 )
                return;

            NormalizedPoint first = points[0];
            NormalizedPoint second = points[1];
            if ( m_block )
            {
                // draw a semitransparent block around the 2 points
                painter->setPen( m_annotation->color );
                painter->setBrush( QBrush( m_annotation->color.light(), Qt::Dense4Pattern ) );
                painter->drawRect( (int)(first.x * (double)xScale), (int)(first.y * (double)yScale),
                                   (int)((second.x - first.x) * (double)xScale), (int)((second.y - first.y) * (double)yScale) );
            }
            else
            {
                // draw a line that connects the 2 points
                painter->setPen( QPen(m_annotation->color, 2) );
                painter->drawLine( (int)(first.x * (double)xScale), (int)(first.y * (double)yScale),
                                   (int)(second.x * (double)xScale), (int)(second.y * (double)yScale) );
            }
        }

        Annotation * end()
        {
            return m_annotation;
        }

    private:
        QValueList<NormalizedPoint> points;
        bool m_block;
};


/** PageViewAnnotator **/

PageViewAnnotator::PageViewAnnotator( PageView * parent, KPDFDocument * storage )
    : QObject( parent ), m_document( storage ), m_pageView( parent ),
    m_toolBar( 0 ), m_engine( 0 ), m_lastToolID( -1 ), m_lockedItem( 0 )
{
    // load the tools from the 'xml tools definition' file. store the tree internally.
    QFile infoFile( locate("data", "kpdf/tools.xml") );
    if ( infoFile.exists() && infoFile.open( IO_ReadOnly ) )
    {
        QDomDocument doc( "annotationTools" );
        if ( doc.setContent( &infoFile ) )
            m_toolsDefinition = doc.elementsByTagName("annotationTools").item( 0 ).toElement();
        else
            kdWarning() << "Annotations XML file seems to be damaged" << endl;
        infoFile.close();
    }
    else
        kdWarning() << "Unable to open Annotation Tools XML definition" << endl;
}

PageViewAnnotator::~PageViewAnnotator()
{
    delete m_engine;
}

void PageViewAnnotator::setEnabled( bool on )
{
    if ( !on )
    {
        // remove toolBar
        if ( m_toolBar )
            m_toolBar->hideAndDestroy();
        m_toolBar = 0;
        return;
    }

    // if no tools are defined, don't show the toolbar
    if ( !m_toolsDefinition.hasChildNodes() )
        return;

    // create toolBar
    if ( !m_toolBar )
    {
        m_toolBar = new PageViewToolBar( m_pageView, m_pageView->viewport() );
        connect( m_toolBar, SIGNAL( toolSelected(int) ),
                this, SLOT( slotToolSelected(int) ) );
        connect( m_toolBar, SIGNAL( orientationChanged(int) ),
                this, SLOT( slotSaveToolbarOrientation(int) ) );
    }

    // create the ToolBarItems from the XML dom tree
    QValueList<ToolBarItem> items;
    QDomNode toolDescription = m_toolsDefinition.firstChild();
    while ( toolDescription.isElement() )
    {
        QDomElement toolElement = toolDescription.toElement();
        if ( toolElement.tagName() == "tool" )
        {
            ToolBarItem item;
            item.id = toolElement.attribute("id").toInt();
            item.text = toolElement.attribute("name");
            item.pixmap = toolElement.attribute("pixmap");
            QDomNode shortcutNode = toolElement.elementsByTagName( "shortcut" ).item( 0 );
            if ( shortcutNode.isElement() )
                item.shortcut = shortcutNode.toElement().text();
            items.push_back( item );
        }
        toolDescription = toolDescription.nextSibling();
    }

    // show the toolBar
    m_toolBar->showItems( (PageViewToolBar::Side)Settings::editToolBarPlacement(), items );

    // ask for Author's name if not already set
    if ( Settings::annotationsAuthor().isEmpty() )
    {
        // get default username from the kdelibs/kdecore/KUser
        KUser currentUser;
        QString userName = currentUser.fullName();
        // ask the user for confirmation/change
        bool firstTry = true;
        while ( firstTry || userName.isEmpty()  )
        {
            QString prompt = firstTry ? i18n( "Please insert your name or initials:" ) :
                i18n( "You must set this name:" );
            userName = KInputDialog::getText( i18n("Annotations author"), prompt, userName );
            firstTry = false;
        }
        // save the name
        Settings::setAnnotationsAuthor( userName );
    }
}

bool PageViewAnnotator::routeEvents() const
{
    return m_engine && m_toolBar;
}

void PageViewAnnotator::routeEvent( QMouseEvent * e, PageViewItem * item )
{
if ( !item ) return; //STRAPAAAATCH !!! FIXME

    // find out mouse event type
    AnnotatorEngine::EventType eventType = AnnotatorEngine::Press;
    if ( e->type() == QEvent::MouseMove )
        eventType = AnnotatorEngine::Move;
    else if ( e->type() == QEvent::MouseButtonRelease )
        eventType = AnnotatorEngine::Release;

    // find out the pressed button
    AnnotatorEngine::Button button = AnnotatorEngine::None;
    Qt::ButtonState buttonState = ( eventType == AnnotatorEngine::Move ) ? e->state() : e->button();
    if ( buttonState == Qt::LeftButton )
        button = AnnotatorEngine::Left;
    else if ( buttonState == Qt::RightButton )
        button = AnnotatorEngine::Right;

    // find out normalized mouse coords inside current item
    const QRect & itemRect = item->geometry();
    double itemWidth = (double)itemRect.width();
    double itemHeight = (double)itemRect.height();
    double nX = (double)(e->x() - itemRect.left()) / itemWidth;
    double nY = (double)(e->y() - itemRect.top()) / itemHeight;

    // 1. lock annotation to current item
    if ( m_lockedItem && item != m_lockedItem )
        return;
    if ( !m_lockedItem && eventType == AnnotatorEngine::Press )
        m_lockedItem = item;

    // 2. use engine for performing operations on annotation
    QRect paintRect = m_engine->event( eventType, button, nX, nY, itemWidth, itemHeight );

    // 3. update absolute annotation rect and send paint event(s)
    if ( paintRect.isValid() )
    {
        // 3.1. unite old and new painting regions
        QRegion compoundRegion( m_lastDrawnRect );
        m_lastDrawnRect = paintRect;
        m_lastDrawnRect.moveBy( itemRect.left(), itemRect.top() );
        // 3.2. decompose paint region in rects and send paint events
        QMemArray<QRect> rects = compoundRegion.unite( m_lastDrawnRect ).rects();
        for ( uint i = 0; i < rects.count(); i++ )
            m_pageView->updateContents( rects[i] );
    }

    // 4. if engine has finished, apply annotations to the page
    if ( m_engine->creationCompleted() )
    {
        // apply engine data to annotation and reset engine data
        Annotation * annotation = m_engine->end();
        // attach the newly filled annotation to the page
        if ( annotation )
            m_document->addPageAnnotation( m_lockedItem->pageNumber(), annotation );

        // go on creating annotation of the same type
        slotToolSelected( m_lastToolID );
    }
}

bool PageViewAnnotator::routePaints( const QRect & wantedRect ) const
{
    return m_engine && m_toolBar && wantedRect.intersects( m_lastDrawnRect );
}

void PageViewAnnotator::routePaint( QPainter * painter, const QRect & paintRect )
{
#ifndef NDEBUG
    // [DEBUG] draw the annotation extension if enabled
    if ( Settings::debugDrawAnnotationRect() )
        painter->drawRect( paintRect );
#endif
    // move painter to current itemGeometry rect
    const QRect & itemGeometry = m_lockedItem->geometry();
    painter->save();
    painter->translate( itemGeometry.left(), itemGeometry.top() );

    // transform cliprect from absolute to item relative coords
    QRect annotRect = paintRect.intersect( m_lastDrawnRect );
    annotRect.moveBy( itemGeometry.left(), itemGeometry.top() );

    // use current engine for painting
    m_engine->paint( painter, m_lockedItem->width(), m_lockedItem->height(), annotRect );
    painter->restore();
}

void PageViewAnnotator::slotToolSelected( int toolID )
{
    // terminate any previous operation
    if ( m_engine )
    {
        delete m_engine;
        m_engine = 0;
    }
    m_lockedItem = 0;
    if ( m_lastDrawnRect.isValid() )
    {
        m_pageView->updateContents( m_lastDrawnRect );
        m_lastDrawnRect = QRect();
    }

    // store current tool for later usage
    m_lastToolID = toolID;

    // handle tool deselection
    if ( toolID == -1 )
    {
        m_pageView->displayMessage( QString() );
        return;
    }

    // for the selected tool initialize the Annotation and create the Engine
    QDomNode toolDescription = m_toolsDefinition.firstChild();
    while ( toolDescription.isElement() )
    {
        QDomElement toolElement = toolDescription.toElement();
        if ( toolElement.tagName() == "tool" &&
             toolElement.attribute("id").toInt() == toolID )
        {
            // create Annotation
            Annotation * annotation = 0;
            QDomElement a = toolElement.elementsByTagName( "annotation" ).item( 0 ).toElement();
            if ( !a.isNull() )
            {
                // create annotation and apply custom attributes
                QString aType = a.attribute( "type" );
                if ( aType == "Text" )
                    annotation = new TextAnnotation();
                else if ( aType == "Line" )
                {
                    LineAnnotation * l = new LineAnnotation();
                    if ( a.hasAttribute( "width" ) )
                        l->border.width = a.attribute( "width" ).toInt();
                    annotation = l;
                }
                else if ( aType == "Geom" )
                    annotation = new GeomAnnotation();
                else if ( aType == "Highlight" )
                    annotation = new HighlightAnnotation();
                else if ( aType == "Stamp" )
                {
                    StampAnnotation * s = new StampAnnotation();
                    s->stampIconName = a.attribute( "icon" );
                    annotation = s;
                }
                else if ( aType == "Ink" )
                    annotation = new InkAnnotation();
                else
                    kdWarning() << "tools: annotation '" << aType << "' is not defined!" << endl;

                // parse generic annotation attributes
                if ( annotation )
                {
                    if ( a.hasAttribute( "color" ) )
                        annotation->color = QColor( a.attribute( "color" ) );
                }
            }
            if ( !annotation )
            {
                kdWarning() << "tools: can't create annotation for toolID: " << toolID << endl;
                break;
            }

            // create Engine
            QDomElement e = toolElement.elementsByTagName( "engine" ).item( 0 ).toElement();
            if ( e.isNull() )
                kdWarning() << "tools: engine not defined" << endl;
            else
            {
                QString eType = e.attribute( "type" );
                if ( eType == "SmoothLine" )
                    m_engine = new SmoothPathEngine( annotation );
                else if ( eType == "PickPoint" )
                {
                    QString pixName = e.attribute( "hoverIcon" );
                    if ( pixName.isNull() )
                        kdWarning() << "tools: engine PickPoint needs 'hoverIcon' attribute!" << endl;
                    else
                        m_engine = new PickPointEngine( annotation, pixName );
                }
                else if ( eType == "TwoPoints" )
                {
                    bool block = e.attribute( "block" ) == "true";
                    m_engine = new TwoPointsEngine( annotation, block );
                }
                else
                    kdWarning() << "tools: engine '" << eType << "' is not defined!" << endl;
            }
            if ( !m_engine )
            {
                delete annotation;
                break;
            }

            // show ToolTip if present
            QDomNode tNode = toolElement.elementsByTagName( "tooltip" ).item( 0 );
            if ( tNode.isElement() )
                m_pageView->displayMessage( tNode.toElement().text() );
            break;
        }
        toolDescription = toolDescription.nextSibling();
    }
}

void PageViewAnnotator::slotSaveToolbarOrientation( int side )
{
    Settings::setEditToolBarPlacement( (int)side );
}

#include "pageviewannotator.moc"
