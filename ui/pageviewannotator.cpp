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
#include <qlist.h>
#include <qpainter.h>
#include <qset.h>
#include <qvariant.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kinputdialog.h>
#include <kuser.h>
#include <kdebug.h>
#include <kmenu.h>

// system includes
#include <math.h>

// local includes
#include "core/area.h"
#include "core/document.h"
#include "core/page.h"
#include "core/annotations.h"
#include "settings.h"
#include "annotationtools.h"
#include "pageview.h"
#include "pageviewutils.h"
#include "pageviewannotator.h"

/** @short PickPointEngine */
class PickPointEngine : public AnnotatorEngine
{
    public:
        PickPointEngine( const QDomElement & engineElement )
            : AnnotatorEngine( engineElement ), clicked( false ), pixmap( 0 ),
              xscale( 1.0 ), yscale( 1.0 )
        {
            // parse engine specific attributes
            pixmapName = engineElement.attribute( "hoverIcon" );
            QString stampname = m_annotElement.attribute( "icon" );
            if ( m_annotElement.attribute( "type" ) == "Stamp" && !stampname.simplified().isEmpty() )
                pixmapName = stampname;
            center = QVariant( engineElement.attribute( "center" ) ).toBool();
            bool ok = true;
            size = engineElement.attribute( "size", "32" ).toInt( &ok );
            if ( !ok )
                size = 32;
            m_block = QVariant( engineElement.attribute( "block" ) ).toBool();

            // create engine objects
            if ( !pixmapName.simplified().isEmpty() )
                pixmap = new QPixmap( DesktopIcon( pixmapName, size ) );
        }

        ~PickPointEngine()
        {
            delete pixmap;
        }

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale, const Okular::Page * page )
        {
            xscale=xScale;
            yscale=yScale;
            pagewidth = page->width();
            pageheight = page->height();
            // only proceed if pressing left button
            if ( button != Left )
                return QRect();

            // start operation on click
            if ( type == Press && clicked == false )
            {
                clicked = true;
                startpoint.x=nX;
                startpoint.y=nY;
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
            if ( center && pixmap )
            {
                rect.left = nX - ( pixmap->width() / ( xScale * 2.0 ) );
                rect.top = nY - ( pixmap->height() / ( yScale * 2.0 ) );
            }
            else
            {
                rect.left = nX;
                rect.top = nY;
            }
            rect.right = rect.left + ( pixmap ? pixmap->width() / xScale : 0 );
            rect.bottom = rect.top + ( pixmap ? pixmap->height() / yScale : 0 );
            QRect boundrect = rect.geometry( (int)xScale, (int)yScale ).adjusted( 0, 0, 1, 1 );
            if ( m_block )
            {
                Okular::NormalizedRect tmprect( qMin( startpoint.x, point.x ), qMin( startpoint.y, point.y ), qMax( startpoint.x, point.x ), qMax( startpoint.y, point.y ) );
                boundrect |= tmprect.geometry( (int)xScale, (int)yScale ).adjusted( 0, 0, 1, 1 );
            }
            return boundrect;
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            if ( clicked )
            {
                if ( m_block )
                {
                    QPen origpen = painter->pen();
                    QPen pen = painter->pen();
                    pen.setStyle( Qt::DashLine );
                    painter->setPen( pen );
                    Okular::NormalizedRect tmprect( qMin( startpoint.x, point.x ), qMin( startpoint.y, point.y ), qMax( startpoint.x, point.x ), qMax( startpoint.y, point.y ) );
                    QRect realrect = tmprect.geometry( (int)xScale, (int)yScale ).adjusted( 0, 0, 1, 1 );
                    painter->drawRect( realrect );
                    painter->setPen( origpen );
                }
                if ( pixmap )
                    painter->drawPixmap( QPointF( rect.left * xScale, rect.top * yScale ), *pixmap );
            }
        }

        QList< Okular::Annotation* > end()
        {
            m_creationCompleted = false;

            // find out annotation's description node
            if ( m_annotElement.isNull() )
                return QList< Okular::Annotation* >();

            // find out annotation's type
            Okular::Annotation * ann = 0;
            QString typeString = m_annotElement.attribute( "type" );
            // create TextAnnotation from path
            if ( typeString == "FreeText")	//<annotation type="Text"
            {
                //note dialog
                QString prompt = i18n( "Please input the free text:" ) ;
                bool resok;
                QString note = KInputDialog::getMultiLineText( i18n( "Free Text" ), prompt, QString::null, &resok );
                if(resok)
                {
                    //add note
                    Okular::TextAnnotation * ta = new Okular::TextAnnotation();
                    ann = ta;
                    ta->setInplaceText( note );
                    ta->setTextType( Okular::TextAnnotation::InPlace );
                    //set boundary
                    rect.left = qMin(startpoint.x,point.x);
                    rect.top = qMin(startpoint.y,point.y);
                    rect.right = qMax(startpoint.x,point.x);
                    rect.bottom = qMax(startpoint.y,point.y);
                    kDebug()<<"astario:   xyScale="<<xscale<<","<<yscale<<endl;
                    static int padding = 2;
                    QFontMetricsF mf(ta->textFont());
                    QRectF rcf = mf.boundingRect( Okular::NormalizedRect( rect.left, rect.top, 1.0, 1.0 ).geometry( (int)pagewidth, (int)pageheight ).adjusted( padding, padding, -padding, -padding ),
                                                  Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, ta->inplaceText() );
                    rect.right = qMax(rect.right, rect.left+(rcf.width()+padding*2)/pagewidth);
                    rect.bottom = qMax(rect.bottom, rect.top+(rcf.height()+padding*2)/pageheight);
                    ta->setBoundingRectangle( this->rect );
                    ta->window().setSummary( "TextBox" );
                }
            }
            else if ( typeString == "Text")
            {
                Okular::TextAnnotation * ta = new Okular::TextAnnotation();
                ann = ta;
                ta->setTextType( Okular::TextAnnotation::Linked );
                ta->window().setText( QString() );
                //ta->window.flags &= ~(Okular::Annotation::Hidden);
                double iconhei=0.03;
                rect.left = point.x;
                rect.top = point.y;
                rect.right=rect.left+iconhei;
                rect.bottom=rect.top+iconhei*xscale/yscale;
                ta->setBoundingRectangle( this->rect );
                ta->window().setSummary( "Note" );
            }
            // create StampAnnotation from path
            else if ( typeString == "Stamp" )
            {
                Okular::StampAnnotation * sa = new Okular::StampAnnotation();
                ann = sa;
                sa->setStampIconName( pixmapName );
                double stampxscale = size / xscale;
                double stampyscale = size / yscale;
                if ( center )
                {
                    rect.left = point.x - stampxscale / 2;
                    rect.top = point.y - stampyscale / 2;
                }
                else
                {
                    rect.left = point.x;
                    rect.top = point.y;
                }
                rect.right = rect.left + stampxscale;
                rect.bottom = rect.top + stampyscale;
                sa->setBoundingRectangle( rect );
            }
            // create GeomAnnotation
            else if ( typeString == "GeomSquare" || typeString == "GeomCircle" )
            {
                Okular::GeomAnnotation * ga = new Okular::GeomAnnotation();
                ann = ga;
                // set the type
                if ( typeString == "GeomSquare" )
                    ga->setGeometricalType( Okular::GeomAnnotation::InscribedSquare );
                else
                    ga->setGeometricalType( Okular::GeomAnnotation::InscribedCircle );
                ga->setGeometricalPointWidth( 18 );
                //set boundary
                rect.left = qMin( startpoint.x, point.x );
                rect.top = qMin( startpoint.y, point.y );
                rect.right = qMax( startpoint.x, point.x );
                rect.bottom = qMax( startpoint.y, point.y );
                ga->setBoundingRectangle( rect );
            }

            // safety check
            if ( !ann )
                return QList< Okular::Annotation* >();

            // set common attributes
            ann->style().setColor( m_annotElement.hasAttribute( "color" ) ?
                    m_annotElement.attribute( "color" ) : m_engineColor );
            if ( m_annotElement.hasAttribute( "opacity" ) )
                ann->style().setOpacity( m_annotElement.attribute( "opacity", "1.0" ).toDouble() );

            // return annotation
            return QList< Okular::Annotation* >() << ann;
        }

    private:
        bool clicked;
        Okular::NormalizedRect rect;
        Okular::NormalizedPoint startpoint;
        Okular::NormalizedPoint point;
        QPixmap * pixmap;
        QString pixmapName;
        int size;
        double xscale,yscale;
        double pagewidth, pageheight;
        bool center;
        bool m_block;
};

/** @short PolyLineEngine */
class PolyLineEngine : public AnnotatorEngine
{
    public:
        PolyLineEngine( const QDomElement & engineElement )
            : AnnotatorEngine( engineElement ), last( false )
        {
            // parse engine specific attributes
            m_block = engineElement.attribute( "block" ) == "true";
            bool ok = true;
            // numofpoints represents the max number of points for the current
            // polygon/polyline, with a pair of exceptions:
            // -1 means: the polyline must close on the first point (polygon)
            // 0 means: construct as many points as you want, right-click
            //   to construct the last point
            numofpoints = engineElement.attribute( "points" ).toInt( &ok );
            if ( !ok )
                numofpoints = -1;
        }

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale, const Okular::Page * /*page*/ )
        {
            // only proceed if pressing left button
//            if ( button != Left )
//                return rect;

            // start operation
            if ( type == Press )
            {
                newPoint.x = nX;
                newPoint.y = nY;
                if ( button == Right )
                    last = true;
            }
            // move the second point
            else if ( type == Move )
            {
                movingpoint.x = nX;
                movingpoint.y = nY;
                QRect oldmovingrect = movingrect;
                movingrect = rect | QRect( (int)( movingpoint.x * xScale ), (int)( movingpoint.y * yScale ), 1, 1 );
                return oldmovingrect | movingrect;
            }
            else if ( type == Release )
            {
                Okular::NormalizedPoint tmppoint;
                tmppoint.x = nX;
                tmppoint.y = nY;
                if ( fabs( tmppoint.x - newPoint.x + tmppoint.y - newPoint.y ) > 1e-2 )
                    return rect;

                if ( numofpoints == -1 && points.count() > 1 && ( fabs( points[0].x - newPoint.x + points[0].y - newPoint.y ) < 1e-2 ) )
                {
                    last = true;
                }
                else
                {
                    points.append( newPoint );
                    rect |= QRect( (int)( newPoint.x * xScale ), (int)( newPoint.y * yScale ), 1, 1 );
                }
                // end creation if we have constructed the last point of enough points
                if ( last || points.count() == numofpoints )
                {
                    m_creationCompleted = true;
                    last = false;
                    normRect = Okular::NormalizedRect( rect, xScale, yScale );
                }
            }

            return rect;
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            if ( points.count() < 1 )
                return;

            if ( m_block && points.count() == 2 )
            {
                Okular::NormalizedPoint first = points[0];
                Okular::NormalizedPoint second = points[1];
                // draw a semitransparent block around the 2 points
                painter->setPen( m_engineColor );
                painter->setBrush( QBrush( m_engineColor.light(), Qt::Dense4Pattern ) );
                painter->drawRect( (int)(first.x * (double)xScale), (int)(first.y * (double)yScale),
                                   (int)((second.x - first.x) * (double)xScale), (int)((second.y - first.y) * (double)yScale) );
            }
            else
            {
                // draw a polyline that connects the constructed points
                painter->setPen( QPen( m_engineColor, 2 ) );
                for ( int i = 1; i < points.count(); ++i )
                    painter->drawLine( (int)(points[i - 1].x * (double)xScale), (int)(points[i - 1].y * (double)yScale),
                                       (int)(points[i].x * (double)xScale), (int)(points[i].y * (double)yScale) );
                painter->drawLine( (int)(points.last().x * (double)xScale), (int)(points.last().y * (double)yScale),
                                   (int)(movingpoint.x * (double)xScale), (int)(movingpoint.y * (double)yScale) );
            }
        }

        QList< Okular::Annotation* > end()
        {
            m_creationCompleted = false;

            // find out annotation's description node
            if ( m_annotElement.isNull() )
                return QList< Okular::Annotation* >();

            // find out annotation's type
            Okular::Annotation * ann = 0;
            QString typeString = m_annotElement.attribute( "type" );

            // create LineAnnotation from path
            if ( typeString == "Line" || typeString == "Polyline" || typeString == "Polygon" )
            {
                if ( points.count() < 2 )
                    return QList< Okular::Annotation* >();

                //add note
                Okular::LineAnnotation * la = new Okular::LineAnnotation();
                ann = la;
                QLinkedList<Okular::NormalizedPoint> list;
                for ( int i = 0; i < points.count(); ++i )
                    list.append( points[ i ] );

                la->setLinePoints( list );

                if ( numofpoints == -1 )
                    la->setLineClosed( true );

                la->setBoundingRectangle( normRect );

            }

            // safety check
            if ( !ann )
                return QList< Okular::Annotation* >();

            // set common attributes
            ann->style().setColor( m_annotElement.hasAttribute( "color" ) ?
                    m_annotElement.attribute( "color" ) : m_engineColor );
            if ( m_annotElement.hasAttribute( "opacity" ) )
                ann->style().setOpacity( m_annotElement.attribute( "opacity", "1.0" ).toDouble() );
            // return annotation

            return QList< Okular::Annotation* >() << ann;
        }

    private:
        QList<Okular::NormalizedPoint> points;
        Okular::NormalizedPoint newPoint;
        Okular::NormalizedPoint movingpoint;
        QRect rect;
        QRect movingrect;
        Okular::NormalizedRect normRect;
        bool m_block;
        bool last;
        int numofpoints;
};

/** @short TextSelectorEngine */
class TextSelectorEngine : public AnnotatorEngine
{
    public:
        TextSelectorEngine( const QDomElement & engineElement, PageView * pageView )
            : AnnotatorEngine( engineElement ), m_pageView( pageView ),
            selection( 0 )
        {
            // parse engine specific attributes
        }

        QRect event( EventType type, Button button, double nX, double nY, double xScale, double yScale, const Okular::Page * /*page*/ )
        {
            // only proceed if pressing left button
            if ( button != Left )
                return QRect();

            if ( type == Press )
            {
                lastPoint.x = nX;
                lastPoint.y = nY;
                QRect oldrect = rect;
                rect = QRect();
                return oldrect;
            }
            else if ( type == Move )
            {
                if ( item() )
                {
                    QPoint start( (int)( lastPoint.x * item()->width() ), (int)( lastPoint.y * item()->height() ) );
                    QPoint end( (int)( nX * item()->width() ), (int)( nY * item()->height() ) );
                    Okular::RegularAreaRect * newselection = m_pageView->textSelectionForItem( item(), start, end );
                    QList<QRect> geom = newselection->geometry( (int)xScale, (int)yScale );
                    QRect newrect;
                    foreach( const QRect& r, geom )
                    {
                        if ( newrect.isNull() )
                            newrect = r;
                        else
                            newrect |= r;
                    }
                    rect |= newrect;
                    delete selection;
                    selection = newselection;
                }
            }
            else if ( type == Release && selection )
            {
                m_creationCompleted = true;
            }
            return rect;
        }

        void paint( QPainter * painter, double xScale, double yScale, const QRect & /*clipRect*/ )
        {
            if ( selection )
            {
                painter->setPen( Qt::NoPen );
                QColor col = m_engineColor;
                col.setAlphaF( 0.5 );
                painter->setBrush( col );
                foreach( const Okular::NormalizedRect & r, *selection )
                {
                    painter->drawRect( r.geometry( (int)xScale, (int)yScale ) );
                }
            }
        }

        QList< Okular::Annotation* > end()
        {
            m_creationCompleted = false;

            // safety checks
            if ( m_annotElement.isNull() || !selection )
                return QList< Okular::Annotation* >();

            // find out annotation's type
            Okular::Annotation * ann = 0;
            QString typeString = m_annotElement.attribute( "type" );

            Okular::HighlightAnnotation::HighlightType type = Okular::HighlightAnnotation::Highlight;
            bool typevalid = false;
            // create HighlightAnnotation's from the selected area
            if ( typeString == "Highlight" )
            {
                type = Okular::HighlightAnnotation::Highlight;
                typevalid = true;
            }
            else if ( typeString == "Squiggly" )
            {
                type = Okular::HighlightAnnotation::Squiggly;
                typevalid = true;
            }
            else if ( typeString == "Underline" )
            {
                type = Okular::HighlightAnnotation::Underline;
                typevalid = true;
            }
            else if ( typeString == "StrikeOut" )
            {
                type = Okular::HighlightAnnotation::StrikeOut;
                typevalid = true;
            }
            if ( typevalid )
            {
                Okular::HighlightAnnotation * ha = new Okular::HighlightAnnotation();
                ha->setHighlightType( type );
                ha->setBoundingRectangle( Okular::NormalizedRect( rect, (int)item()->width(), (int)item()->height() ) );
                foreach ( const Okular::NormalizedRect & r, *selection )
                {
                    Okular::HighlightAnnotation::Quad q;
                    q.setCapStart( false );
                    q.setCapEnd( false );
                    q.setFeather( 1.0 );
                    q.setPoint( Okular::NormalizedPoint( r.left, r.bottom ), 0 );
                    q.setPoint( Okular::NormalizedPoint( r.right, r.bottom ), 1 );
                    q.setPoint( Okular::NormalizedPoint( r.right, r.top ), 2 );
                    q.setPoint( Okular::NormalizedPoint( r.left, r.top ), 3 );
                    ha->highlightQuads().append( q );
                }
                ann = ha;
            }

            // safety check
            if ( !ann )
                return QList< Okular::Annotation* >();

            // set common attributes
            ann->style().setColor( m_annotElement.hasAttribute( "color" ) ?
                m_annotElement.attribute( "color" ) : m_engineColor );
            if ( m_annotElement.hasAttribute( "opacity" ) )
                ann->style().setOpacity( m_annotElement.attribute( "opacity", "1.0" ).toDouble() );

            // return annotations
            return QList< Okular::Annotation* >() << ann;
        }

    private:
        // data
        PageView * m_pageView;
        // TODO: support more pages
        Okular::RegularAreaRect * selection;
        Okular::NormalizedPoint lastPoint;
        QRect rect;
};


/** PageViewAnnotator **/

PageViewAnnotator::PageViewAnnotator( PageView * parent, Okular::Document * storage )
    : QObject( parent ), m_document( storage ), m_pageView( parent ),
    m_toolBar( 0 ), m_engine( 0 ), m_lastToolID( -1 ), m_lockedItem( 0 )
{
    // load the tools from the 'xml tools definition' file. store the tree internally.
    QFile infoFile( KStandardDirs::locate("data", "okular/tools.xml") );
    if ( infoFile.exists() && infoFile.open( QIODevice::ReadOnly ) )
    {
        QDomDocument doc( "annotatingTools" );
        if ( doc.setContent( &infoFile ) )
        {
            m_toolsDefinition = doc.elementsByTagName("annotatingTools").item( 0 ).toElement();

            // create the ToolBarItems from the XML dom tree
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
                    m_items.push_back( item );
                }
                toolDescription = toolDescription.nextSibling();
            }
        }
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
        // deactivate the active tool, if any
        slotToolSelected( -1 );
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

    // show the toolBar
    m_toolBar->showItems( (PageViewToolBar::Side)Okular::Settings::editToolBarPlacement(), m_items );

    // ask for Author's name if not already set
    if ( Okular::Settings::identityAuthor().isEmpty() )
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
        Okular::Settings::setIdentityAuthor( userName );
        Okular::Settings::writeConfig();
    }
}

bool PageViewAnnotator::routeEvents() const
{
    return m_engine && m_toolBar;
}

QRect PageViewAnnotator::routeEvent( QMouseEvent * e, PageViewItem * item )
{
    if ( !item ) return QRect();

    // find out mouse event type
    AnnotatorEngine::EventType eventType = AnnotatorEngine::Press;
    if ( e->type() == QEvent::MouseMove )
        eventType = AnnotatorEngine::Move;
    else if ( e->type() == QEvent::MouseButtonRelease )
        eventType = AnnotatorEngine::Release;

    // find out the pressed button
    AnnotatorEngine::Button button = AnnotatorEngine::None;
    Qt::MouseButtons buttonState = ( eventType == AnnotatorEngine::Move ) ? e->buttons() : e->button();
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

    QRect modifiedRect;

    // 1. lock engine to current item
    if ( m_lockedItem && item != m_lockedItem )
        return QRect();
    if ( !m_lockedItem && eventType == AnnotatorEngine::Press )
    {
        m_lockedItem = item;
        m_engine->setItem( m_lockedItem );
    }

    // 2. use engine to perform operations
    QRect paintRect = m_engine->event( eventType, button, nX, nY, itemWidth, itemHeight, item->page() );

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
            m_pageView->widget()->update( rects[i] );
        modifiedRect = compoundRegion.boundingRect() | m_lastDrawnRect;
    }

    // 4. if engine has finished, apply Annotation to the page
    if ( m_engine->creationCompleted() )
    {
        // apply engine data to the Annotation's and reset engine
        QList< Okular::Annotation* > annotations = m_engine->end();
        // attach the newly filled annotations to the page
        foreach ( Okular::Annotation * annotation, annotations )
        {
            if ( !annotation ) continue;

            annotation->setCreationDate( QDateTime::currentDateTime() );
            annotation->setModificationDate( QDateTime::currentDateTime() );
            annotation->setAuthor( Okular::Settings::identityAuthor() );
            m_document->addPageAnnotation( m_lockedItem->pageNumber(), annotation );
        }

        // go on creating annotations of the same type
        slotToolSelected( m_lastToolID );
    }

    return modifiedRect;
}

bool PageViewAnnotator::routePaints( const QRect & wantedRect ) const
{
    return m_engine && m_toolBar && wantedRect.intersects( m_lastDrawnRect ) && m_lockedItem;
}

void PageViewAnnotator::routePaint( QPainter * painter, const QRect & paintRect )
{
    // if there's no locked item, then there's no decided place to draw on
    if ( !m_lockedItem )
        return;

#ifndef NDEBUG
    // [DEBUG] draw the paint region if enabled
    if ( Okular::Settings::debugDrawAnnotationRect() )
        painter->drawRect( paintRect );
#endif
    // move painter to current itemGeometry rect
    const QRect & itemGeometry = m_lockedItem->geometry();
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
        m_pageView->widget()->update( m_lastDrawnRect );
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
                else if ( type == "PolyLine" )
                    m_engine = new PolyLineEngine( toolSubElement );
                else if ( type == "TextSelector" )
                    m_engine = new TextSelectorEngine( toolSubElement, m_pageView );
                else
                    kWarning() << "tools.xml: engine type:'" << type << "' is not defined!" << endl;
            }
            // display the tooltip
            else if ( toolSubElement.tagName() == "tooltip" )
            {
                QString tip = toolSubElement.text();
                if ( !tip.isEmpty() )
                    m_pageView->displayMessage( i18nc( "Annotation tool", tip.toUtf8() ), PageViewMessage::Annotation );
            }
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
    Okular::Settings::setEditToolBarPlacement( (int)side );
    Okular::Settings::writeConfig();
}

#include "pageviewannotator.moc"
