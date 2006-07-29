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
#include "settings.h"
#include "pageview.h"
#include "pageviewutils.h"
#include "pageviewannotator.h"

/**
 * @short Engine: filter events to distill an Annotation.
 */
class AnnotatorEngine
{
    public:
        AnnotatorEngine( const QDomElement & engineElement )
            : m_engineElement( engineElement ), m_creationCompleted( false )
        {
            // parse common engine attributes
            if ( engineElement.hasAttribute( "color" ) )
                m_engineColor = QColor( engineElement.attribute( "color" ) );
        }
        virtual ~AnnotatorEngine() {};

        // enum definitions
        enum EventType { Press, Move, Release };
        enum Button { None, Left, Right };

        // perform operations
        virtual QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale ) = 0;
        virtual void paint( QPainter * painter, double xScale, double yScale, const QRect & clipRect ) = 0;
        virtual Annotation * end() = 0;

        // query creation state
        //PageViewItem * editingItem() const { return m_lockedItem; }
        bool creationCompleted() const { return m_creationCompleted; }

    protected:
        // common engine attributes (the element includes annotation desc)
        QDomElement m_engineElement;
        QColor m_engineColor;
        // other vars (remove this!)
        bool m_creationCompleted;
};

/** @short SmoothPathEngine */
class SmoothPathEngine : public AnnotatorEngine
{
    public:
        SmoothPathEngine( const QDomElement & engineElement )
            : AnnotatorEngine( engineElement )
        {
            // parse engine specific attributes
        }

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale )
        {
            // only proceed if pressing left button
            if ( button != Left )
                return QRect();

            // start operation
            if ( type == Press && points.isEmpty() )
            {
                lastPoint.x = nX;
                lastPoint.y = nY;
                totalRect.left = totalRect.right = lastPoint.x;
                totalRect.top = totalRect.bottom = lastPoint.y;
                points.append( lastPoint );
            }
            // add a point to the path
            else if ( type == Move && points.count() > 0 )
            {
                //double dist = hypot( nX - points.last().x, nY - points.last().y );
                //if ( dist > 0.0001 )
                //{
                    // append mouse position (as normalized point) to the list
                    NormalizedPoint nextPoint = NormalizedPoint( nX, nY );
                    points.append( nextPoint );
                    // update total rect
                    double dX = 2.0 / (double)xScale;
                    double dY = 2.0 / (double)yScale;
                    totalRect.left = qMin( totalRect.left, nX - dX );
                    totalRect.top = qMin( totalRect.top, nY - dY );
                    totalRect.right = qMax( nX + dX, totalRect.right );
                    totalRect.bottom = qMax( nY + dY, totalRect.bottom );
                    // paint the difference to previous full rect
                    NormalizedRect incrementalRect;
                    incrementalRect.left = qMin( nextPoint.x, lastPoint.x ) - dX;
                    incrementalRect.right = qMax( nextPoint.x, lastPoint.x ) + dX;
                    incrementalRect.top = qMin( nextPoint.y, lastPoint.y ) - dY;
                    incrementalRect.bottom = qMax( nextPoint.y, lastPoint.y ) + dY;
                    lastPoint = nextPoint;
                    return incrementalRect.geometry( (int)xScale, (int)yScale );
                //}
            }
            // terminate process
            else if ( type == Release && points.count() > 0 )
            {
                if ( points.count() < 2 )
                    points.clear();
                else
                    m_creationCompleted = true;
                return totalRect.geometry( (int)xScale, (int)yScale );
            }
            return QRect();
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            // draw SmoothPaths with at least 2 points
            if ( points.count() > 1 )
            {
                // use engine's color for painting
                painter->setPen( QPen( m_engineColor, 1 ) );

                QLinkedList<NormalizedPoint>::iterator pIt = points.begin(), pEnd = points.end();
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
        }

        Annotation * end()
        {
            // find out annotation's description node
            const QDomElement & annElement = m_engineElement.firstChild().toElement();
            if ( annElement.isNull() || annElement.tagName() != "annotation" )
                return 0;

            // find out annotation's type
            Annotation * ann = 0;
            QString typeString = annElement.attribute( "type" );

            // create HighlightAnnotation from path
            if ( typeString == "Highlight" )
            {
                HighlightAnnotation * ha = new HighlightAnnotation();
                ann = ha;
                // TODO
            }
            // create InkAnnotation from path
            else if ( typeString == "Ink" )
            {
                InkAnnotation * ia = new InkAnnotation();
                ann = ia;
                if ( annElement.hasAttribute( "width" ) )
                    ann->style.width = annElement.attribute( "width" ).toDouble();
                // fill points
                ia->inkPaths.append( points );
                // set boundaries
                ia->boundary = totalRect;
            }

            // safety check
            if ( !ann )
                return 0;

            // set common attributes
            ann->style.color = annElement.hasAttribute( "color" ) ?
                annElement.attribute( "color" ) : m_engineColor;
            if ( annElement.hasAttribute( "opacity" ) )
                ann->style.opacity = annElement.attribute( "opacity" ).toDouble();

            // return annotation
            return ann;
        }

    private:
        // data
        QLinkedList<NormalizedPoint> points;
        NormalizedRect totalRect;
        NormalizedPoint lastPoint;
};

/** @short PickPointEngine */
class PickPointEngine : public AnnotatorEngine
{
    public:
        PickPointEngine( const QDomElement & engineElement )
            : AnnotatorEngine( engineElement ), clicked( false )
        {
            // parse engine specific attributes
            QString pixmapName = engineElement.attribute( "hoverIcon" );
            if ( pixmapName.isNull() )
                pixmapName = "okular";

            // create engine objects
            pixmap = new QPixmap( DesktopIcon( "okular", 32 ) );
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

            // update variables and extents (zoom invariant rect)
            point.x = nX;
            point.y = nY;
            rect.left = nX - (16.0 / (double)xScale) ;
            rect.right = nX + (17.0 / (double)xScale) ;
            rect.top = nY - (16.0 / (double)yScale) ;
            rect.bottom = nY + (17.0 / (double)yScale) ;
            return rect.geometry( (int)xScale, (int)yScale );
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            if ( clicked && pixmap )
            {
                int pX = (int)(point.x * (double)xScale);
                int pY = (int)(point.y * (double)yScale);
                painter->drawPixmap( pX - 15, pY - 15, *pixmap );
            }
        }

        Annotation * end()
        {
            // find out annotation's description node
            const QDomElement & annElement = m_engineElement.firstChild().toElement();
            if ( annElement.isNull() || annElement.tagName() != "annotation" )
                return 0;

            // find out annotation's type
            Annotation * ann = 0;
            QString typeString = annElement.attribute( "type" );

            // create TextAnnotation from path
            if ( typeString == "Text")	//<annotation type="Text" 
            {
				//find if chlicked a text annoteation, if there is, load it, or create it.
				//note dialog
				QString prompt = i18n( "Please input the note:" ) ;
				bool resok;
				QString note ="";
				
				note= KInputDialog::getText( i18n("Note"), prompt, note,&resok );
				if(resok)
				{
					//add note
					TextAnnotation * ta = new TextAnnotation();
					ann = ta;
					ta->inplaceText=note;
					ta->textType = TextAnnotation::InPlace;
					ta->boundary=this->rect;
	
				}
            }
            // create StampAnnotation from path
            else if ( typeString == "Stamp" )
            {
                StampAnnotation * sa = new StampAnnotation();
                ann = sa;
				sa->stampIconName="okular";
            }

            // safety check
            if ( !ann )
                return 0;

            // set common attributes
            ann->style.color = annElement.hasAttribute( "color" ) ?
                annElement.attribute( "color" ) : m_engineColor;
            if ( annElement.hasAttribute( "opacity" ) )
                ann->style.opacity = annElement.attribute( "opacity" ).toDouble();

            // return annotation
            return ann;
        }

    private:
        bool clicked;
        NormalizedRect rect;
        NormalizedPoint point;
        QPixmap * pixmap;
};

/** @short TwoPointsEngine */
class TwoPointsEngine : public AnnotatorEngine
{
    public:
        TwoPointsEngine( const QDomElement & engineElement )
            : AnnotatorEngine( engineElement )
        {
            // parse engine specific attributes
            m_block = engineElement.attribute( "block" ) == "true";
        }

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale )
        {
            // only proceed if pressing left button
            if ( button != Left )
                return QRect();

            // start operation
            if ( type == Press && points.isEmpty() )
            {
                NormalizedPoint newPoint;
                newPoint.x = nX;
                newPoint.y = nY;
                rect.left = rect.right =newPoint.x;
                rect.top = rect.bottom =newPoint.y;
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
                rect.left = qMin( firstPoint.x, nX ) - 2.0 / (double)xScale;
                rect.right = qMax( firstPoint.x, nX ) + 2.0 / (double)xScale;
                rect.top = qMin( firstPoint.y, nY ) - 2.0 / (double)yScale;
                rect.bottom = qMax( firstPoint.y, nY ) + 2.0 / (double)yScale;
            }
            // end creation if we have 2 points
            else if ( type == Release && points.count() == 2 )
                m_creationCompleted = true;

            return rect.geometry( (int)xScale, (int)yScale );
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
                painter->setPen( m_engineColor );
                painter->setBrush( QBrush( m_engineColor.light(), Qt::Dense4Pattern ) );
                painter->drawRect( (int)(first.x * (double)xScale), (int)(first.y * (double)yScale),
                                   (int)((second.x - first.x) * (double)xScale), (int)((second.y - first.y) * (double)yScale) );
            }
            else
            {
                // draw a line that connects the 2 points
                painter->setPen( QPen( m_engineColor, 2 ) );
                painter->drawLine( (int)(first.x * (double)xScale), (int)(first.y * (double)yScale),
                                   (int)(second.x * (double)xScale), (int)(second.y * (double)yScale) );
            }
        }

        Annotation * end()
        {
			// find out annotation's description node
            const QDomElement & annElement = m_engineElement.firstChild().toElement();
            if ( annElement.isNull() || annElement.tagName() != "annotation" )
                return 0;

            // find out annotation's type
            Annotation * ann = 0;
            QString typeString = annElement.attribute( "type" );

            // create LineAnnotation from path
            if ( typeString == "Line")	//<annotation type="Text" 
            {
				
            if ( points.count() != 2 )
                return 0;
				//add note
				LineAnnotation * la = new LineAnnotation();
				ann = la;
				la->linePoints.append(points[0]);
				la->linePoints.append(points[1]);
				la->boundary=this->rect;
				
            }

            // safety check
            if ( !ann )
                return 0;

            // set common attributes
            ann->style.color = annElement.hasAttribute( "color" ) ?
                annElement.attribute( "color" ) : m_engineColor;
            if ( annElement.hasAttribute( "opacity" ) )
                ann->style.opacity = annElement.attribute( "opacity" ).toDouble();
            // return annotation
            return ann;
        }

    private:
        QList<NormalizedPoint> points;
        NormalizedRect rect;
        bool m_block;
};


/** PageViewAnnotator **/

PageViewAnnotator::PageViewAnnotator( PageView * parent, KPDFDocument * storage )
    : QObject( parent ), m_document( storage ), m_pageView( parent ),
    m_toolBar( 0 ), m_engine( 0 ), m_lastToolID( -1 ), m_lockedItem( 0 )
{
    // load the tools from the 'xml tools definition' file. store the tree internally.
    QFile infoFile( KStandardDirs::locate("data", "okular/tools.xml") );
    if ( infoFile.exists() && infoFile.open( QIODevice::ReadOnly ) )
    {
        QDomDocument doc( "annotatingTools" );
        if ( doc.setContent( &infoFile ) )
            m_toolsDefinition = doc.elementsByTagName("annotatingTools").item( 0 ).toElement();
        else
            kWarning() << "AnnotatingTools XML file seems to be damaged" << endl;
        infoFile.close();
    }
    else
        kWarning() << "Unable to open AnnotatingTools XML definition" << endl;
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
    QLinkedList<ToolBarItem> items;
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
    m_toolBar->showItems( (PageViewToolBar::Side)KpdfSettings::editToolBarPlacement(), items );

    // ask for Author's name if not already set
    if ( KpdfSettings::annotationsAuthor().isEmpty() )
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
        KpdfSettings::setAnnotationsAuthor( userName );
        KpdfSettings::writeConfig();
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
    const QRectF & itemRect = item->geometry();
    double itemWidth = itemRect.width();
    double itemHeight = itemRect.height();
    double nX = (double)(e->x() - itemRect.left()) / itemWidth;
    double nY = (double)(e->y() - itemRect.top()) / itemHeight;

    // 1. lock engine to current item
    if ( m_lockedItem && item != m_lockedItem )
        return;
    if ( !m_lockedItem && eventType == AnnotatorEngine::Press )
        m_lockedItem = item;

    // 2. use engine to perform operations
    QRect paintRect = m_engine->event( eventType, button, nX, nY, itemWidth, itemHeight );

    // 3. update absolute extents rect and send paint event(s)
    if ( paintRect.isValid() )
    {
        // 3.1. unite old and new painting regions
        QRegion compoundRegion( m_lastDrawnRect );
        m_lastDrawnRect = paintRect;
        m_lastDrawnRect.translate( itemRect.left(), itemRect.top() );
        // 3.2. decompose paint region in rects and send paint events
        QVector<QRect> rects = compoundRegion.unite( m_lastDrawnRect ).rects();
        for ( int i = 0; i < rects.count(); i++ )
            m_pageView->update( rects[i] );
    }

    // 4. if engine has finished, apply Annotation to the page
    if ( m_engine->creationCompleted() )
    {
        // apply engine data to Annotation and reset engine
        Annotation * annotation = m_engine->end();
        // attach the newly filled annotation to the page
        if ( annotation )
        {
            annotation->author = KpdfSettings::annotationsAuthor();
            m_document->addPageAnnotation( m_lockedItem->pageNumber(), annotation );
        }

        // go on creating annotations of the same type
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
    // [DEBUG] draw the paint region if enabled
    if ( KpdfSettings::debugDrawAnnotationRect() )
        painter->drawRect( paintRect );
#endif
    // move painter to current itemGeometry rect
    const QRectF & itemGeometry = m_lockedItem->geometry();
    painter->save();
    painter->translate( itemGeometry.left(), itemGeometry.top() );

    // transform cliprect from absolute to item relative coords
    QRect annotRect = paintRect.intersect( m_lastDrawnRect );
    annotRect.translate( itemGeometry.left(), itemGeometry.top() );

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
        m_pageView->update( m_lastDrawnRect );
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

    // for the selected tool create the Engine
    QDomNode toolNode = m_toolsDefinition.firstChild();
    while ( toolNode.isElement() )
    {
        QDomElement toolElement = toolNode.toElement();
        toolNode = toolNode.nextSibling();

        // only find out the element describing selected tool
        if ( toolElement.tagName() != "tool" || toolElement.attribute("id").toInt() != toolID )
            continue;

        // parse tool properties
        QDomNode toolSubNode = toolElement.firstChild();
        while ( toolSubNode.isElement() )
        {
            QDomElement toolSubElement = toolSubNode.toElement();
            toolSubNode = toolSubNode.nextSibling();

            // create the AnnotatorEngine
            if ( toolSubElement.tagName() == "engine" )
            {
                QString type = toolSubElement.attribute( "type" );
                if ( type == "SmoothLine" )
                    m_engine = new SmoothPathEngine( toolSubElement );
                else if ( type == "PickPoint" )
                    m_engine = new PickPointEngine( toolSubElement );
                else if ( type == "TwoPoints" )
                    m_engine = new TwoPointsEngine( toolSubElement );
                else
                    kWarning() << "tools.xml: engine type:'" << type << "' is not defined!" << endl;
            }
            // display the tooltip
            else if ( toolSubElement.tagName() == "tooltip" )
                m_pageView->displayMessage( toolSubElement.text() );
        }

        // consistancy warning
        if ( !m_engine )
            kWarning() << "tools.xml: couldn't find good engine description. check xml." << endl;

        // stop after parsing selected tool's node
        break;
    }
}

void PageViewAnnotator::slotSaveToolbarOrientation( int side )
{
    KpdfSettings::setEditToolBarPlacement( (int)side );
    KpdfSettings::writeConfig();
}

#include "pageviewannotator.moc"
