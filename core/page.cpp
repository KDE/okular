/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <kdebug.h>
#include <QTime>

// local includes
#include "annotations.h"
#include "area.h"
#include "link.h"
#include "page.h"
#include "pagetransition.h"
#include "rotationjob.h"

using namespace Okular;

class TextSelection;

static void deleteObjectRects( QLinkedList< ObjectRect * >& rects, const QSet<ObjectRect::ObjectType>& which )
{
    QLinkedList< ObjectRect * >::iterator it = rects.begin(), end = rects.end();
    for ( ; it != end; )
        if ( which.contains( (*it)->objectType() ) )
        {
            delete *it;
            it = rects.erase( it );
        }
        else
            ++it;
}

class Page::Private
{
    public:
        Private( Page *page, uint n, double w, double h, int o )
            : m_page( page ), m_number( n ), m_orientation( o ),
              m_width( w ), m_height( h ),
              m_rotation( 0 ), m_bookmarked( false ), m_maxuniqueNum( 0 ),
              m_text( 0 ), m_transition( 0 ),
              m_openingAction( 0 ), m_closingAction( 0 )
        {
            // avoid Division-By-Zero problems in the program
            if ( m_width <= 0 )
                m_width = 1;

            if ( m_height <= 0 )
                m_height = 1;
        }

        ~Private()
        {
            delete m_text;
            delete m_transition;
        }

        void imageRotationDone();
        QMatrix rotationMatrix() const;

        Page *m_page;
        int m_number;
        int m_orientation;
        double m_width, m_height;
        int m_rotation;
        bool m_bookmarked;
        int m_maxuniqueNum;

        TextPage * m_text;
        PageTransition * m_transition;
        Link * m_openingAction;
        Link * m_closingAction;
};

void Page::Private::imageRotationDone()
{
    RotationJob *job = static_cast<RotationJob*>( m_page->sender() );

    if ( m_page->m_pixmaps.contains( job->id() ) ) {
        PixmapObject &object = m_page->m_pixmaps[ job->id() ];
        (*object.m_pixmap) = QPixmap::fromImage( job->image() );
        object.m_rotation = job->rotation();
    } else {
        PixmapObject object;
        object.m_pixmap = new QPixmap( QPixmap::fromImage( job->image() ) );
        object.m_rotation = job->rotation();

        m_page->m_pixmaps.insert( job->id(), object );
    }

    emit m_page->rotationFinished( m_number );

    job->deleteLater();
}

QMatrix Page::Private::rotationMatrix() const
{
    QMatrix matrix;
    matrix.rotate( m_rotation * 90 );

    if ( m_rotation == 1 ) {
      matrix.translate( 0, -1 );
    } else if ( m_rotation == 2 ) {
      matrix.translate( -1, -1 );
    } else if ( m_rotation == 3 ) {
      matrix.translate( -1, 0 );
    }

    return matrix;
}

/** class Page **/

Page::Page( uint page, double w, double h, int o )
    : QObject( 0 ), d( new Private( this, page, w, h, o ) ),
      m_textSelections( 0 )
{
}

Page::~Page()
{
    deletePixmaps();
    deleteRects();
    deleteHighlights();
    deleteAnnotations();
    deleteTextSelections();
    deleteSourceReferences();

    delete d;
}

int Page::number() const
{
    return d->m_number;
}

int Page::orientation() const
{
    return d->m_orientation;
}

int Page::rotation() const
{
    return d->m_rotation;
}

int Page::totalOrientation() const
{
    return ( d->m_orientation + d->m_rotation ) % 4;
}

double Page::width() const
{
    return d->m_width;
}

double Page::height() const
{
    return d->m_height;
}

double Page::ratio() const
{
    return d->m_height / d->m_width;
}

bool Page::hasPixmap( int id, int width, int height ) const
{
    QMap< int, PixmapObject >::const_iterator it = m_pixmaps.find( id );
    if ( it == m_pixmaps.end() )
        return false;

    if ( width == -1 || height == -1 )
        return true;

    const QPixmap *pixmap = it.value().m_pixmap;

    return (pixmap->width() == width && pixmap->height() == height);
}

bool Page::hasTextPage() const
{
    return d->m_text != 0;
}

bool Page::isBookmarked() const
{
    return d->m_bookmarked;
}

RegularAreaRect * Page::textArea ( TextSelection * selection ) const
{
    if ( d->m_text )
        return d->m_text->textArea( selection );

    return 0;
}

bool Page::hasObjectRect( double x, double y, double xScale, double yScale ) const
{
    if ( m_rects.isEmpty() )
        return false;

    QLinkedList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( x, y, xScale, yScale ) )
            return true;

    return false;
}

bool Page::hasHighlights( int s_id ) const
{
    // simple case: have no highlights
    if ( m_highlights.isEmpty() )
        return false;
    // simple case: we have highlights and no id to match
    if ( s_id == -1 )
        return true;
    // iterate on the highlights list to find an entry by id
    QLinkedList< HighlightAreaRect * >::const_iterator it = m_highlights.begin(), end = m_highlights.end();
    for ( ; it != end; ++it )
        if ( (*it)->s_id == s_id )
            return true;
    return false;
}

bool Page::hasTransition() const
{
    return d->m_transition != 0;
}

bool Page::hasAnnotations() const
{
    return !m_annotations.isEmpty();
}

RegularAreaRect * Page::findText( int id, const QString & text, SearchDirection direction,
                                  Qt::CaseSensitivity caseSensitivity, const RegularAreaRect *lastRect ) const
{
    RegularAreaRect* rect = 0;
    if ( text.isEmpty() )
        return rect;

    rect = d->m_text->findText( id, text, direction, caseSensitivity, lastRect );
    return rect;
}

QString Page::text( const RegularAreaRect * area ) const
{
    QString ret;

    if ( !d->m_text )
        return ret;

    ret = d->m_text->text( area );

    return ret;
}

void Page::rotateAt( int orientation )
{
    int neworientation = orientation % 4;
    if ( neworientation == d->m_rotation )
        return;

    deleteHighlights();
    deleteTextSelections();

    if ( d->m_text )
        d->m_text->transform( d->rotationMatrix() );

    if ( ( d->m_orientation + d->m_rotation ) % 2 != ( d->m_orientation + neworientation ) % 2 )
        qSwap( d->m_width, d->m_height );

    d->m_rotation = neworientation;

    QMap< int, RotationJob::Rotation > rotationMap;
    rotationMap.insert( 0, RotationJob::Rotation0 );
    rotationMap.insert( 1, RotationJob::Rotation90 );
    rotationMap.insert( 2, RotationJob::Rotation180 );
    rotationMap.insert( 3, RotationJob::Rotation270 );

    /**
     * Rotate the images of the page.
     */
    QMapIterator< int, PixmapObject > it( m_pixmaps );
    while ( it.hasNext() ) {
        it.next();

        const PixmapObject &object = m_pixmaps[ it.key() ];

        RotationJob::Rotation newRotation = rotationMap[ d->m_rotation ];
        RotationJob::Rotation oldRotation = rotationMap[ object.m_rotation ];

        RotationJob *job = new RotationJob( object.m_pixmap->toImage(), oldRotation, newRotation, it.key() );
        connect( job, SIGNAL( finished() ), this, SLOT( imageRotationDone() ) );
        job->start();
    }

    /**
     * Rotate the object rects on the page.
     */
    const QMatrix matrix = d->rotationMatrix();
    QLinkedList< ObjectRect * >::const_iterator objectIt = m_rects.begin(), end = m_rects.end();
    for ( ; objectIt != end; ++objectIt )
        (*objectIt)->transform( matrix );
}

const ObjectRect * Page::objectRect( ObjectRect::ObjectType type, double x, double y, double xScale, double yScale ) const
{
    QLinkedList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( ( (*it)->objectType() == type ) && (*it)->contains( x, y, xScale, yScale ) )
            return *it;
    return 0;
}

const PageTransition * Page::transition() const
{
    return d->m_transition;
}

const QLinkedList< Annotation* > Page::annotations() const
{
    return m_annotations;
}

const Link * Page::pageAction( PageAction action ) const
{
    switch ( action )
    {
        case Page::Opening:
            return d->m_openingAction;
            break;
        case Page::Closing:
            return d->m_closingAction;
            break;
    }

    return 0;
}

void Page::setPixmap( int id, QPixmap *pixmap )
{
    if ( d->m_rotation == 0 ) {
        PixmapObject object;

        object.m_pixmap = pixmap;
        object.m_rotation = d->m_rotation;

        m_pixmaps[ id ] = object;
    } else {
        static QMap< int, RotationJob::Rotation > rotationMap;
        if ( rotationMap.isEmpty() ) {
            rotationMap.insert( 0, RotationJob::Rotation0 );
            rotationMap.insert( 1, RotationJob::Rotation90 );
            rotationMap.insert( 2, RotationJob::Rotation180 );
            rotationMap.insert( 3, RotationJob::Rotation270 );
        }

        RotationJob *job = new RotationJob( pixmap->toImage(), RotationJob::Rotation0, rotationMap[ d->m_rotation ], id );
        connect( job, SIGNAL( finished() ), this, SLOT( imageRotationDone() ) );
        job->start();

        delete pixmap;
    }
}

void Page::setTextPage( TextPage * textPage )
{
    delete d->m_text;

    textPage->transform( d->rotationMatrix() );

    d->m_text = textPage;
}

void Page::setBookmarked( bool state )
{
    d->m_bookmarked = state;
}

void Page::setObjectRects( const QLinkedList< ObjectRect * > rects )
{
    QSet<ObjectRect::ObjectType> which;
    which << ObjectRect::Link << ObjectRect::Image;
    deleteObjectRects( m_rects, which );

    /**
     * Rotate the object rects of the page.
     */
    const QMatrix matrix = d->rotationMatrix();

    QLinkedList< ObjectRect * >::const_iterator objectIt = rects.begin(), end = rects.end();
    for ( ; objectIt != end; ++objectIt )
        (*objectIt)->transform( matrix );

    m_rects << rects;
}

void Page::setHighlight( int s_id, RegularAreaRect *rect, const QColor & color )
{
    HighlightAreaRect * hr = new HighlightAreaRect(rect);
    hr->s_id = s_id;
    hr->color = color;
    m_highlights.append( hr );
}

void Page::setTextSelections( RegularAreaRect *r, const QColor & color )
{
    deleteTextSelections();
    HighlightAreaRect * hr = new HighlightAreaRect( r );
    hr->s_id = -1;
    hr->color = color;
    m_textSelections = hr;
}

void Page::setSourceReferences( const QLinkedList< SourceRefObjectRect * > refRects )
{
    deleteSourceReferences();
    foreach( SourceRefObjectRect * rect, refRects )
        m_rects << rect;
}

const RegularAreaRect * Page::textSelection() const
{
    return m_textSelections;
}

void Page::addAnnotation( Annotation * annotation )
{
    //uniqueName: okular-PAGENUM-ID
    if(annotation->uniqueName().isEmpty())
    {
        QString uniqueName = "okular-";
        uniqueName += ( QString::number(d->m_number) + '-' + QString::number(++(d->m_maxuniqueNum)) );

        kDebug()<<"astario:     inc m_maxuniqueNum="<<d->m_maxuniqueNum<<endl;

        annotation->setUniqueName( uniqueName );
    }
    m_annotations.append( annotation );

    AnnotationObjectRect *rect = new AnnotationObjectRect( annotation );

    /**
     * Rotate the annotation on the page.
     */
    const QMatrix matrix = d->rotationMatrix();
    rect->transform( matrix );

    m_rects.append( rect );
}

void Page::modifyAnnotation(Annotation * newannotation )
{
    if(!newannotation)
        return;

    QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
    {
        if((*aIt)==newannotation)
            return; //modified already
        if((*aIt) && (*aIt)->uniqueName()==newannotation->uniqueName())
        {
            int rectfound = false;
            QLinkedList< ObjectRect * >::iterator it = m_rects.begin(), end = m_rects.end();
            for ( ; it != end && !rectfound; ++it )
                if ( ( (*it)->objectType() == ObjectRect::OAnnotation ) && ( (*it)->pointer() == (*aIt) ) )
                {
                    delete *it;
                    *it = new AnnotationObjectRect( newannotation );
                    rectfound = true;
                }
            delete *aIt;
            *aIt = newannotation;
            break;
        }
    }
}

bool Page::removeAnnotation( Annotation * annotation )
{
    if ( !annotation || ( annotation->flags() & Annotation::DenyDelete ) )
        return false;

    QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
    {
        if((*aIt) && (*aIt)->uniqueName()==annotation->uniqueName())
        {
            int rectfound = false;
            QLinkedList< ObjectRect * >::iterator it = m_rects.begin(), end = m_rects.end();
            for ( ; it != end && !rectfound; ++it )
                if ( ( (*it)->objectType() == ObjectRect::OAnnotation ) && ( (*it)->pointer() == (*aIt) ) )
                {
                    delete *it;
                    it = m_rects.erase( it );
                    rectfound = true;
                }
            kDebug() << "removed annotation: " << annotation->uniqueName() << endl;
            delete *aIt;
            m_annotations.erase( aIt );
            break;
        }
    }

    return true;
}

void Page::setTransition( PageTransition * transition )
{
    delete d->m_transition;
    d->m_transition = transition;
}

void Page::setPageAction( PageAction action, Link * link )
{
    switch ( action )
    {
        case Page::Opening:
            d->m_openingAction = link;
            break;
        case Page::Closing:
            d->m_closingAction = link;
            break;
    }
}

void Page::deletePixmap( int id )
{
    PixmapObject object = m_pixmaps.take( id );
    delete object.m_pixmap;
}

void Page::deletePixmaps()
{
    QMapIterator< int, PixmapObject > it( m_pixmaps );
    while ( it.hasNext() ) {
        it.next();
        delete it.value().m_pixmap;
    }

    m_pixmaps.clear();
}

void Page::deleteRects()
{
    // delete ObjectRects of type Link and Image
    QSet<ObjectRect::ObjectType> which;
    which << ObjectRect::Link << ObjectRect::Image;
    deleteObjectRects( m_rects, which );
}

void Page::deleteHighlights( int s_id )
{
    // delete highlights by ID
    QLinkedList< HighlightAreaRect* >::iterator it = m_highlights.begin(), end = m_highlights.end();
    while ( it != end )
    {
        HighlightAreaRect* highlight = *it;
        if ( s_id == -1 || highlight->s_id == s_id )
        {
            it = m_highlights.erase( it );
            delete highlight;
        }
        else
            ++it;
    }
}

void Page::deleteTextSelections()
{
    if (m_textSelections)
    {
        qDeleteAll(*m_textSelections);
        delete m_textSelections;
        m_textSelections = 0;
    }
}

void Page::deleteSourceReferences()
{
    deleteObjectRects( m_rects, QSet<ObjectRect::ObjectType>() << ObjectRect::SourceRef );
}

void Page::deleteAnnotations()
{
    // delete ObjectRects of type Annotation
    deleteObjectRects( m_rects, QSet<ObjectRect::ObjectType>() << ObjectRect::OAnnotation );
    // delete all stored annotations
    QLinkedList< Annotation * >::const_iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    m_annotations.clear();
}

void Page::restoreLocalContents( const QDomNode & pageNode )
{
    // iterate over all chilren (bookmark, annotationList, ...)
    QDomNode childNode = pageNode.firstChild();
    while ( childNode.isElement() )
    {
        QDomElement childElement = childNode.toElement();
        childNode = childNode.nextSibling();

        // parse annotationList child element
        if ( childElement.tagName() == "annotationList" )
        {
            QTime time;
            time.start();

            // iterate over all annotations
            QDomNode annotationNode = childElement.firstChild();
            while( annotationNode.isElement() )
            {
                // get annotation element and advance to next annot
                QDomElement annotElement = annotationNode.toElement();
                annotationNode = annotationNode.nextSibling();

                // get annotation from the dom element
                Annotation * annotation = AnnotationUtils::createAnnotation( annotElement );

                // append annotation to the list or show warning
                if ( annotation )
                {
                    m_annotations.append( annotation );
                    m_rects.append( new AnnotationObjectRect( annotation ) );
                    int pos = annotation->uniqueName().lastIndexOf("-");
                    if(pos != -1)
                    {
                        int uniqID=annotation->uniqueName().right(annotation->uniqueName().length()-pos-1).toInt();
                        if(d->m_maxuniqueNum<uniqID)
                            d->m_maxuniqueNum=uniqID;
                    }

                    kDebug()<<"astario:  restored annot:"<<annotation->uniqueName()<<endl;
                }
                else
                    kWarning() << "page (" << d->m_number << "): can't restore an annotation from XML." << endl;
            }
            kDebug() << "annots: XML Load time: " << time.elapsed() << "ms" << endl;
        }

        // parse bookmark child element
        else if ( childElement.tagName() == "bookmark" )
            d->m_bookmarked = true;
    }
}

void Page::saveLocalContents( QDomNode & parentNode, QDomDocument & document )
{
    // only add a node if there is some stuff to write into
    if ( !d->m_bookmarked && m_annotations.isEmpty() )
        return;

    // create the page node and set the 'number' attribute
    QDomElement pageElement = document.createElement( "page" );
    pageElement.setAttribute( "number", d->m_number );

    // add bookmark info if is bookmarked
    if ( d->m_bookmarked )
    {
        // create the pageElement's 'bookmark' child
        QDomElement bookmarkElement = document.createElement( "bookmark" );
        pageElement.appendChild( bookmarkElement );

        // add attributes to the element
        //bookmarkElement.setAttribute( "name", bookmark name );
    }

    // add annotations info if has got any
    if ( !m_annotations.isEmpty() )
    {
        // create the annotationList
        QDomElement annotListElement = document.createElement( "annotationList" );

        // add every annotation to the annotationList
        QLinkedList< Annotation * >::const_iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
        for ( ; aIt != aEnd; ++aIt )
        {
            // get annotation
            const Annotation * a = *aIt;
            // only save okular annotations (not the embedded in file ones)
            if ( !(a->flags() & Annotation::External) )
            {
                // append an filled-up element called 'annotation' to the list
                QDomElement annElement = document.createElement( "annotation" );
                AnnotationUtils::storeAnnotation( a, annElement, document );
                annotListElement.appendChild( annElement );
                kDebug()<<"astario:  save annot:"<<a->uniqueName()<<endl;
            }
        }

        // append the annotationList element if annotations have been set
        if ( annotListElement.hasChildNodes() )
            pageElement.appendChild( annotListElement );
    }

    // append the page element only if has children
    if ( pageElement.hasChildNodes() )
        parentNode.appendChild( pageElement );
}

#include "page.moc"
