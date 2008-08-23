/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotations.h"
#include "annotations_p.h"

// qt/kde includes
#include <QtGui/QApplication>
#include <QtGui/QColor>

// local includes
#include "document.h"
#include "movie.h"
#include "page_p.h"
#include "sound.h"

using namespace Okular;

//BEGIN AnnotationUtils implementation
Annotation * AnnotationUtils::createAnnotation( const QDomElement & annElement )
{
    // safety check on annotation element
    if ( !annElement.hasAttribute( "type" ) )
        return 0;

    // build annotation of given type
    Annotation * annotation = 0;
    int typeNumber = annElement.attribute( "type" ).toInt();
    switch ( typeNumber )
    {
        case Annotation::AText:
            annotation = new TextAnnotation( annElement );
            break;
        case Annotation::ALine:
            annotation = new LineAnnotation( annElement );
            break;
        case Annotation::AGeom:
            annotation = new GeomAnnotation( annElement );
            break;
        case Annotation::AHighlight:
            annotation = new HighlightAnnotation( annElement );
            break;
        case Annotation::AStamp:
            annotation = new StampAnnotation( annElement );
            break;
        case Annotation::AInk:
            annotation = new InkAnnotation( annElement );
            break;
        case Annotation::ACaret:
            annotation = new CaretAnnotation( annElement );
            break;
    }

    // return created annotation
    return annotation;
}

void AnnotationUtils::storeAnnotation( const Annotation * ann, QDomElement & annElement,
    QDomDocument & document )
{
    // save annotation's type as element's attribute
    annElement.setAttribute( "type", (uint)ann->subType() );

    // append all annotation data as children of this node
    ann->store( annElement, document );
}

QDomElement AnnotationUtils::findChildElement( const QDomNode & parentNode,
    const QString & name )
{
    // loop through the whole children and return a 'name' named element
    QDomNode subNode = parentNode.firstChild();
    while( subNode.isElement() )
    {
        QDomElement element = subNode.toElement();
        if ( element.tagName() == name )
            return element;
        subNode = subNode.nextSibling();
    }
    // if the name can't be found, return a dummy null element
    return QDomElement();
}

QRect AnnotationUtils::annotationGeometry( const Annotation * ann,
    double scaledWidth, double scaledHeight )
{
    if ( ann->subType() == Annotation::AText && ( ( (TextAnnotation*)ann )->textType() == TextAnnotation::Linked ) )
    {
        return QRect( (int)( ann->transformedBoundingRectangle().left * scaledWidth ),
                      (int)( ann->transformedBoundingRectangle().top * scaledHeight ), 24, 24 );
    }

    return ann->transformedBoundingRectangle().geometry( (int)scaledWidth, (int)scaledHeight );
}
//END AnnotationUtils implementation


//BEGIN Annotation implementation

class Annotation::Style::Private
{
    public:
        Private()
            : m_opacity( 1.0 ), m_width( 1.0 ), m_style( Solid ), m_xCorners( 0.0 ),
              m_yCorners( 0.0 ), m_marks( 3 ), m_spaces( 0 ), m_effect( NoEffect ),
              m_effectIntensity( 1.0 )
        {
        }

        QColor m_color;
        double m_opacity;
        double m_width;
        LineStyle m_style;
        double m_xCorners;
        double m_yCorners;
        int m_marks;
        int m_spaces;
        LineEffect m_effect;
        double m_effectIntensity;
};

Annotation::Style::Style()
    : d( new Private )
{
}

Annotation::Style::~Style()
{
    delete d;
}

Annotation::Style::Style( const Style &other )
    : d( new Private )
{
    *d = *other.d;
}

Annotation::Style& Annotation::Style::operator=( const Style &other )
{
    if ( this != &other )
        *d = *other.d;

    return *this;
}

void Annotation::Style::setColor( const QColor &color )
{
    d->m_color = color;
}

QColor Annotation::Style::color() const
{
    return d->m_color;
}

void Annotation::Style::setOpacity( double opacity )
{
    d->m_opacity = opacity;
}

double Annotation::Style::opacity() const
{
    return d->m_opacity;
}

void Annotation::Style::setWidth( double width )
{
    d->m_width = width;
}

double Annotation::Style::width() const
{
    return d->m_width;
}

void Annotation::Style::setLineStyle( LineStyle style )
{
    d->m_style = style;
}

Annotation::LineStyle Annotation::Style::lineStyle() const
{
    return d->m_style;
}

void Annotation::Style::setXCorners( double xCorners )
{
    d->m_xCorners = xCorners;
}

double Annotation::Style::xCorners() const
{
    return d->m_xCorners;
}

void Annotation::Style::setYCorners( double yCorners )
{
    d->m_yCorners = yCorners;
}

double Annotation::Style::yCorners() const
{
    return d->m_yCorners;
}

void Annotation::Style::setMarks( int marks )
{
    d->m_marks = marks;
}

int Annotation::Style::marks() const
{
    return d->m_marks;
}

void Annotation::Style::setSpaces( int spaces )
{
    d->m_spaces = spaces;
}

int Annotation::Style::spaces() const
{
    return d->m_spaces;
}

void Annotation::Style::setLineEffect( LineEffect effect )
{
    d->m_effect = effect;
}

Annotation::LineEffect Annotation::Style::lineEffect() const
{
    return d->m_effect;
}

void Annotation::Style::setEffectIntensity( double intensity )
{
    d->m_effectIntensity = intensity;
}

double Annotation::Style::effectIntensity() const
{
    return d->m_effectIntensity;
}


class Annotation::Window::Private
{
    public:
        Private()
            : m_flags( -1 ), m_width( 0 ), m_height( 0 )
        {
        }

        int m_flags;
        NormalizedPoint m_topLeft;
        int m_width;
        int m_height;
        QString m_title;
        QString m_summary;
        QString m_text;
};

Annotation::Window::Window()
    : d( new Private )
{
}

Annotation::Window::~Window()
{
    delete d;
}

Annotation::Window::Window( const Window &other )
    : d( new Private )
{
    *d = *other.d;
}

Annotation::Window& Annotation::Window::operator=( const Window &other )
{
    if ( this != &other )
        *d = *other.d;

    return *this;
}

void Annotation::Window::setFlags( int flags )
{
    d->m_flags = flags;
}

int Annotation::Window::flags() const
{
    return d->m_flags;
}

void Annotation::Window::setTopLeft( const NormalizedPoint &point )
{
    d->m_topLeft = point;
}

NormalizedPoint Annotation::Window::topLeft() const
{
    return d->m_topLeft;
}

void Annotation::Window::setWidth( int width )
{
    d->m_width = width;
}

int Annotation::Window::width() const
{
    return d->m_width;
}

void Annotation::Window::setHeight( int height )
{
    d->m_height = height;
}

int Annotation::Window::height() const
{
    return d->m_height;
}

void Annotation::Window::setTitle( const QString &title )
{
    d->m_title = title;
}

QString Annotation::Window::title() const
{
    return d->m_title;
}

void Annotation::Window::setSummary( const QString &summary )
{
    d->m_summary = summary;
}

QString Annotation::Window::summary() const
{
    return d->m_summary;
}

void Annotation::Window::setText( const QString &text )
{
    d->m_text = text;
}

QString Annotation::Window::text() const
{
    return d->m_text;
}


class Annotation::Revision::Private
{
    public:
        Private()
            : m_annotation( 0 ), m_scope( Reply ), m_type( None )
        {
        }

        Annotation *m_annotation;
        RevisionScope m_scope;
        RevisionType m_type;
};

Annotation::Revision::Revision()
    : d( new Private )
{
}

Annotation::Revision::~Revision()
{
    delete d;
}

Annotation::Revision::Revision( const Revision &other )
    : d( new Private )
{
    *d = *other.d;
}

Annotation::Revision& Annotation::Revision::operator=( const Revision &other )
{
    if ( this != &other )
        *d = *other.d;

    return *this;
}

void Annotation::Revision::setAnnotation( Annotation *annotation )
{
    d->m_annotation = annotation;
}

Annotation *Annotation::Revision::annotation() const
{
    return d->m_annotation;
}

void Annotation::Revision::setScope( RevisionScope scope )
{
    d->m_scope = scope;
}

Annotation::RevisionScope Annotation::Revision::scope() const
{
    return d->m_scope;
}

void Annotation::Revision::setType( RevisionType type )
{
    d->m_type = type;
}

Annotation::RevisionType Annotation::Revision::type() const
{
    return d->m_type;
}


AnnotationPrivate::AnnotationPrivate()
    : m_page( 0 ), m_flags( 0 ), m_disposeFunc( 0 )
{
}

AnnotationPrivate::~AnnotationPrivate()
{
    // delete all children revisions
    if ( m_revisions.isEmpty() )
        return;

    QLinkedList< Annotation::Revision >::iterator it = m_revisions.begin(), end = m_revisions.end();
    for ( ; it != end; ++it )
        delete (*it).annotation();
}

Annotation::Annotation( AnnotationPrivate &dd )
    : d_ptr( &dd )
{
}

Annotation::Annotation( AnnotationPrivate &dd, const QDomNode & annNode )
    : d_ptr( &dd )
{
    Q_D( Annotation );
    // get the [base] element of the annotation node
    QDomElement e = AnnotationUtils::findChildElement( annNode, "base" );
    if ( e.isNull() )
        return;

    // parse -contents- attributes
    if ( e.hasAttribute( "author" ) )
        d->m_author = e.attribute( "author" );
    if ( e.hasAttribute( "contents" ) )
        d->m_contents = e.attribute( "contents" );
    if ( e.hasAttribute( "uniqueName" ) )
        d->m_uniqueName = e.attribute( "uniqueName" );
    if ( e.hasAttribute( "modifyDate" ) )
        d->m_modifyDate = QDateTime::fromString( e.attribute("modifyDate"), Qt::ISODate );
    if ( e.hasAttribute( "creationDate" ) )
        d->m_creationDate = QDateTime::fromString( e.attribute("creationDate"), Qt::ISODate );

    // parse -other- attributes
    if ( e.hasAttribute( "flags" ) )
        d->m_flags = e.attribute( "flags" ).toInt();
    if ( e.hasAttribute( "color" ) )
        d->m_style.setColor( QColor( e.attribute( "color" ) ) );
    if ( e.hasAttribute( "opacity" ) )
        d->m_style.setOpacity( e.attribute( "opacity" ).toDouble() );

    // parse -the-subnodes- (describing Style, Window, Revision(s) structures)
    // Note: all subnodes if present must be 'attributes complete'
    QDomNode eSubNode = e.firstChild();
    while ( eSubNode.isElement() )
    {
        QDomElement ee = eSubNode.toElement();
        eSubNode = eSubNode.nextSibling();

        // parse boundary
        if ( ee.tagName() == "boundary" )
        {
            d->m_boundary=NormalizedRect(ee.attribute( "l" ).toDouble(),
                ee.attribute( "t" ).toDouble(),
                ee.attribute( "r" ).toDouble(),
                ee.attribute( "b" ).toDouble());
        }
        // parse penStyle if not default
        else if ( ee.tagName() == "penStyle" )
        {
            d->m_style.setWidth( ee.attribute( "width" ).toDouble() );
            d->m_style.setLineStyle( (LineStyle)ee.attribute( "style" ).toInt() );
            d->m_style.setXCorners( ee.attribute( "xcr" ).toDouble() );
            d->m_style.setYCorners( ee.attribute( "ycr" ).toDouble() );
            d->m_style.setMarks( ee.attribute( "marks" ).toInt() );
            d->m_style.setSpaces( ee.attribute( "spaces" ).toInt() );
        }
        // parse effectStyle if not default
        else if ( ee.tagName() == "penEffect" )
        {
            d->m_style.setLineEffect( (LineEffect)ee.attribute( "effect" ).toInt() );
            d->m_style.setEffectIntensity( ee.attribute( "intensity" ).toDouble() );
        }
        // parse window if present
        else if ( ee.tagName() == "window" )
        {
            d->m_window.setFlags( ee.attribute( "flags" ).toInt() );
            d->m_window.setTopLeft( NormalizedPoint( ee.attribute( "top" ).toDouble(),
                                                  ee.attribute( "left" ).toDouble() ) );
            d->m_window.setWidth( ee.attribute( "width" ).toInt() );
            d->m_window.setHeight( ee.attribute( "height" ).toInt() );
            d->m_window.setTitle( ee.attribute( "title" ) );
            d->m_window.setSummary( ee.attribute( "summary" ) );
            // parse window subnodes
            QDomNode winNode = ee.firstChild();
            for ( ; winNode.isElement(); winNode = winNode.nextSibling() )
            {
                QDomElement winElement = winNode.toElement();
                if ( winElement.tagName() == "text" )
                    d->m_window.setText( winElement.firstChild().toCDATASection().data() );
            }
        }
    }

    // get the [revisions] element of the annotation node
    QDomNode revNode = annNode.firstChild();
    for ( ; revNode.isElement(); revNode = revNode.nextSibling() )
    {
        QDomElement revElement = revNode.toElement();
        if ( revElement.tagName() != "revision" )
            continue;

        // compile the Revision structure crating annotation
        Revision revision;
        revision.setScope( (RevisionScope)revElement.attribute( "revScope" ).toInt() );
        revision.setType( (RevisionType)revElement.attribute( "revType" ).toInt() );
        revision.setAnnotation( AnnotationUtils::createAnnotation( revElement ) );

        // if annotation is valid, add revision to internal list
        if ( revision.annotation() )
            d->m_revisions.append( revision );
    }

    d->m_transformedBoundary = d->m_boundary;
}

Annotation::~Annotation()
{
    if ( d_ptr->m_disposeFunc )
        d_ptr->m_disposeFunc( this );

    delete d_ptr;
}

void Annotation::setAuthor( const QString &author )
{
    Q_D( Annotation );
    d->m_author = author;
}

QString Annotation::author() const
{
    Q_D( const Annotation );
    return d->m_author;
}

void Annotation::setContents( const QString &contents )
{
    Q_D( Annotation );
    d->m_contents = contents;
}

QString Annotation::contents() const
{
    Q_D( const Annotation );
    return d->m_contents;
}

void Annotation::setUniqueName( const QString &name )
{
    Q_D( Annotation );
    d->m_uniqueName = name;
}

QString Annotation::uniqueName() const
{
    Q_D( const Annotation );
    return d->m_uniqueName;
}

void Annotation::setModificationDate( const QDateTime &date )
{
    Q_D( Annotation );
    d->m_modifyDate = date;
}

QDateTime Annotation::modificationDate() const
{
    Q_D( const Annotation );
    return d->m_modifyDate;
}

void Annotation::setCreationDate( const QDateTime &date )
{
    Q_D( Annotation );
    d->m_creationDate = date;
}

QDateTime Annotation::creationDate() const
{
    Q_D( const Annotation );
    return d->m_creationDate;
}

void Annotation::setFlags( int flags )
{
    Q_D( Annotation );
    d->m_flags = flags;
}

int Annotation::flags() const
{
    Q_D( const Annotation );
    return d->m_flags;
}

void Annotation::setBoundingRectangle( const NormalizedRect &rectangle )
{
    Q_D( Annotation );
    d->m_boundary = rectangle;
    d->resetTransformation();
    if ( d->m_page )
    {
        d->transform( d->m_page->rotationMatrix() );
    }
}

NormalizedRect Annotation::boundingRectangle() const
{
    Q_D( const Annotation );
    return d->m_boundary;
}

NormalizedRect Annotation::transformedBoundingRectangle() const
{
    Q_D( const Annotation );
    return d->m_transformedBoundary;
}

void Annotation::translate( const NormalizedPoint &coord )
{
    Q_D( Annotation );
    d->translate( coord );
    d->resetTransformation();
    if ( d->m_page )
    {
        d->transform( d->m_page->rotationMatrix() );
    }
}

Annotation::Style & Annotation::style()
{
    Q_D( Annotation );
    return d->m_style;
}

const Annotation::Style & Annotation::style() const
{
    Q_D( const Annotation );
    return d->m_style;
}

Annotation::Window & Annotation::window()
{
    Q_D( Annotation );
    return d->m_window;
}

const Annotation::Window & Annotation::window() const
{
    Q_D( const Annotation );
    return d->m_window;
}

QLinkedList< Annotation::Revision > & Annotation::revisions()
{
    Q_D( Annotation );
    return d->m_revisions;
}

const QLinkedList< Annotation::Revision > & Annotation::revisions() const
{
    Q_D( const Annotation );
    return d->m_revisions;
}

void Annotation::setNativeId( const QVariant &id )
{
    Q_D( Annotation );
    d->m_nativeId = id;
}

QVariant Annotation::nativeId() const
{
    Q_D( const Annotation );
    return d->m_nativeId;
}

void Annotation::setDisposeDataFunction( DisposeDataFunction func )
{
    Q_D( Annotation );
    d->m_disposeFunc = func;
}

bool Annotation::canBeMoved() const
{
    Q_D( const Annotation );
    // for now, it is pointless moving external annotations
    // as we cannot change them anyway
    if ( d->m_flags & External )
        return false;

    // highlight "requires" to be "bounded" to text, and that's tricky for now
    if ( subType() == AHighlight )
        return false;

    return true;
}

void Annotation::store( QDomNode & annNode, QDomDocument & document ) const
{
    Q_D( const Annotation );
    // create [base] element of the annotation node
    QDomElement e = document.createElement( "base" );
    annNode.appendChild( e );

    // store -contents- attributes
    if ( !d->m_author.isEmpty() )
        e.setAttribute( "author", d->m_author );
    if ( !d->m_contents.isEmpty() )
        e.setAttribute( "contents", d->m_contents );
    if ( !d->m_uniqueName.isEmpty() )
        e.setAttribute( "uniqueName", d->m_uniqueName );
    if ( d->m_modifyDate.isValid() )
        e.setAttribute( "modifyDate", d->m_modifyDate.toString(Qt::ISODate) );
    if ( d->m_creationDate.isValid() )
        e.setAttribute( "creationDate", d->m_creationDate.toString(Qt::ISODate) );

    // store -other- attributes
    if ( d->m_flags )
        e.setAttribute( "flags", d->m_flags );
    if ( d->m_style.color().isValid() )
        e.setAttribute( "color", d->m_style.color().name() );
    if ( d->m_style.opacity() != 1.0 )
        e.setAttribute( "opacity", d->m_style.opacity() );

    // Sub-Node-1 - boundary
    QDomElement bE = document.createElement( "boundary" );
    e.appendChild( bE );
    bE.setAttribute( "l", (double)d->m_boundary.left );
    bE.setAttribute( "t", (double)d->m_boundary.top );
    bE.setAttribute( "r", (double)d->m_boundary.right );
    bE.setAttribute( "b", (double)d->m_boundary.bottom );

    // Sub-Node-2 - penStyle
    if ( d->m_style.width() != 1 || d->m_style.lineStyle() != Solid || d->m_style.xCorners() != 0 ||
         d->m_style.yCorners() != 0.0 || d->m_style.marks() != 3 || d->m_style.spaces() != 0 )
    {
        QDomElement psE = document.createElement( "penStyle" );
        e.appendChild( psE );
        psE.setAttribute( "width", d->m_style.width() );
        psE.setAttribute( "style", (int)d->m_style.lineStyle() );
        psE.setAttribute( "xcr", d->m_style.xCorners() );
        psE.setAttribute( "ycr", d->m_style.yCorners() );
        psE.setAttribute( "marks", d->m_style.marks() );
        psE.setAttribute( "spaces", d->m_style.spaces() );
    }

    // Sub-Node-3 - penEffect
    if ( d->m_style.lineEffect() != NoEffect || d->m_style.effectIntensity() != 1.0 )
    {
        QDomElement peE = document.createElement( "penEffect" );
        e.appendChild( peE );
        peE.setAttribute( "effect", (int)d->m_style.lineEffect() );
        peE.setAttribute( "intensity", d->m_style.effectIntensity() );
    }

    // Sub-Node-4 - window
    if ( d->m_window.flags() != -1 || !d->m_window.title().isEmpty() ||
         !d->m_window.summary().isEmpty() || !d->m_window.text().isEmpty() )
    {
        QDomElement wE = document.createElement( "window" );
        e.appendChild( wE );
        wE.setAttribute( "flags", d->m_window.flags() );
        wE.setAttribute( "top", d->m_window.topLeft().x );
        wE.setAttribute( "left", d->m_window.topLeft().y );
        wE.setAttribute( "width", d->m_window.width() );
        wE.setAttribute( "height", d->m_window.height() );
        wE.setAttribute( "title", d->m_window.title() );
        wE.setAttribute( "summary", d->m_window.summary() );
        // store window.text as a subnode, because we need escaped data
        if ( !d->m_window.text().isEmpty() )
        {
            QDomElement escapedText = document.createElement( "text" );
            wE.appendChild( escapedText );
            QDomCDATASection textCData = document.createCDATASection( d->m_window.text() );
            escapedText.appendChild( textCData );
        }
    }

    // create [revision] element of the annotation node (if any)
    if ( d->m_revisions.isEmpty() )
        return;

    // add all revisions as children of revisions element
    QLinkedList< Revision >::const_iterator it = d->m_revisions.begin(), end = d->m_revisions.end();
    for ( ; it != end; ++it )
    {
        // create revision element
        const Revision & revision = *it;
        QDomElement r = document.createElement( "revision" );
        annNode.appendChild( r );
        // set element attributes
        r.setAttribute( "revScope", (int)revision.scope() );
        r.setAttribute( "revType", (int)revision.type() );
        // use revision as the annotation element, so fill it up
        AnnotationUtils::storeAnnotation( revision.annotation(), r, document );
    }
}

void AnnotationPrivate::annotationTransform( const QMatrix &matrix )
{
    resetTransformation();
    transform( matrix );
}

void AnnotationPrivate::transform( const QMatrix &matrix )
{
    m_transformedBoundary.transform( matrix );
}

void AnnotationPrivate::baseTransform( const QMatrix &matrix )
{
    m_boundary.transform( matrix );
}

void AnnotationPrivate::resetTransformation()
{
    m_transformedBoundary = m_boundary;
}

void AnnotationPrivate::translate( const NormalizedPoint &coord )
{
    m_boundary.left = m_boundary.left + coord.x;
    m_boundary.right = m_boundary.right + coord.x;
    m_boundary.top = m_boundary.top + coord.y;
    m_boundary.bottom = m_boundary.bottom + coord.y;
}

//END Annotation implementation


/** TextAnnotation [Annotation] */

class Okular::TextAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        TextAnnotationPrivate()
            : AnnotationPrivate(), m_textType( TextAnnotation::Linked ),
              m_textIcon( "Note" ), m_inplaceAlign( 0 ),
              m_inplaceIntent( TextAnnotation::Unknown )
        {
        }

        virtual void transform( const QMatrix &matrix );
        virtual void baseTransform( const QMatrix &matrix );
        virtual void resetTransformation();
        virtual void translate( const NormalizedPoint &coord );

        TextAnnotation::TextType m_textType;
        QString m_textIcon;
        QFont m_textFont;
        int m_inplaceAlign;
        QString m_inplaceText;
        NormalizedPoint m_inplaceCallout[3];
        NormalizedPoint m_transformedInplaceCallout[3];
        TextAnnotation::InplaceIntent m_inplaceIntent;
};

/*
  The default textIcon for text annotation is Note as the PDF Reference says
*/
TextAnnotation::TextAnnotation()
    : Annotation( *new TextAnnotationPrivate() )
{
}

TextAnnotation::TextAnnotation( const QDomNode & node )
    : Annotation( *new TextAnnotationPrivate(), node )
{
    Q_D( TextAnnotation );
    // loop through the whole children looking for a 'text' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "text" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            d->m_textType = (TextAnnotation::TextType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "icon" ) )
            d->m_textIcon = e.attribute( "icon" );
        if ( e.hasAttribute( "font" ) )
            d->m_textFont.fromString( e.attribute( "font" ) );
        if ( e.hasAttribute( "align" ) )
            d->m_inplaceAlign = e.attribute( "align" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            d->m_inplaceIntent = (TextAnnotation::InplaceIntent)e.attribute( "intent" ).toInt();

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                d->m_inplaceText = ee.firstChild().toCDATASection().data();
            }
            else if ( ee.tagName() == "callout" )
            {
                d->m_inplaceCallout[0].x = ee.attribute( "ax" ).toDouble();
                d->m_inplaceCallout[0].y = ee.attribute( "ay" ).toDouble();
                d->m_inplaceCallout[1].x = ee.attribute( "bx" ).toDouble();
                d->m_inplaceCallout[1].y = ee.attribute( "by" ).toDouble();
                d->m_inplaceCallout[2].x = ee.attribute( "cx" ).toDouble();
                d->m_inplaceCallout[2].y = ee.attribute( "cy" ).toDouble();
            }
        }

        // loading complete
        break;
    }

    for ( int i = 0; i < 3; ++i )
        d->m_transformedInplaceCallout[i] = d->m_inplaceCallout[i];
}

TextAnnotation::~TextAnnotation()
{
}

void TextAnnotation::setTextType( TextType textType )
{
    Q_D( TextAnnotation );
    d->m_textType = textType;
}

TextAnnotation::TextType TextAnnotation::textType() const
{
    Q_D( const TextAnnotation );
    return d->m_textType;
}

void TextAnnotation::setTextIcon( const QString &icon )
{
    Q_D( TextAnnotation );
    d->m_textIcon = icon;
}

QString TextAnnotation::textIcon() const
{
    Q_D( const TextAnnotation );
    return d->m_textIcon;
}

void TextAnnotation::setTextFont( const QFont &font )
{
    Q_D( TextAnnotation );
    d->m_textFont = font;
}

QFont TextAnnotation::textFont() const
{
    Q_D( const TextAnnotation );
    return d->m_textFont;
}

void TextAnnotation::setInplaceAlignment( int alignment )
{
    Q_D( TextAnnotation );
    d->m_inplaceAlign = alignment;
}

int TextAnnotation::inplaceAlignment() const
{
    Q_D( const TextAnnotation );
    return d->m_inplaceAlign;
}

void TextAnnotation::setInplaceText( const QString &text )
{
    Q_D( TextAnnotation );
    d->m_inplaceText = text;
}

QString TextAnnotation::inplaceText() const
{
    Q_D( const TextAnnotation );
    return d->m_inplaceText;
}

void TextAnnotation::setInplaceCallout( const NormalizedPoint &point, int index )
{
    if ( index < 0 || index > 2 )
        return;

    Q_D( TextAnnotation );
    d->m_inplaceCallout[ index ] = point;
}

NormalizedPoint TextAnnotation::inplaceCallout( int index ) const
{
    if ( index < 0 || index > 2 )
        return NormalizedPoint();

    Q_D( const TextAnnotation );
    return d->m_inplaceCallout[ index ];
}

NormalizedPoint TextAnnotation::transformedInplaceCallout( int index ) const
{
    if ( index < 0 || index > 2 )
        return NormalizedPoint();

    Q_D( const TextAnnotation );
    return d->m_transformedInplaceCallout[ index ];
}

void TextAnnotation::setInplaceIntent( InplaceIntent intent )
{
    Q_D( TextAnnotation );
    d->m_inplaceIntent = intent;
}

TextAnnotation::InplaceIntent TextAnnotation::inplaceIntent() const
{
    Q_D( const TextAnnotation );
    return d->m_inplaceIntent;
}

Annotation::SubType TextAnnotation::subType() const
{
    return AText;
}

void TextAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const TextAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [text] element
    QDomElement textElement = document.createElement( "text" );
    node.appendChild( textElement );

    // store the optional attributes
    if ( d->m_textType != Linked )
        textElement.setAttribute( "type", (int)d->m_textType );
    if ( d->m_textIcon != "Comment" )
        textElement.setAttribute( "icon", d->m_textIcon );
    if ( d->m_textFont != QApplication::font() )
        textElement.setAttribute( "font", d->m_textFont.toString() );
    if ( d->m_inplaceAlign )
        textElement.setAttribute( "align", d->m_inplaceAlign );
    if ( d->m_inplaceIntent != Unknown )
        textElement.setAttribute( "intent", (int)d->m_inplaceIntent );

    // Sub-Node-1 - escapedText
    if ( !d->m_inplaceText.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        textElement.appendChild( escapedText );
        QDomCDATASection textCData = document.createCDATASection( d->m_inplaceText );
        escapedText.appendChild( textCData );
    }

    // Sub-Node-2 - callout
    if ( d->m_inplaceCallout[0].x != 0.0 )
    {
        QDomElement calloutElement = document.createElement( "callout" );
        textElement.appendChild( calloutElement );
        calloutElement.setAttribute( "ax", d->m_inplaceCallout[0].x );
        calloutElement.setAttribute( "ay", d->m_inplaceCallout[0].y );
        calloutElement.setAttribute( "bx", d->m_inplaceCallout[1].x );
        calloutElement.setAttribute( "by", d->m_inplaceCallout[1].y );
        calloutElement.setAttribute( "cx", d->m_inplaceCallout[2].x );
        calloutElement.setAttribute( "cy", d->m_inplaceCallout[2].y );
    }
}

void TextAnnotationPrivate::transform( const QMatrix &matrix )
{
    AnnotationPrivate::transform( matrix );

    for ( int i = 0; i < 3; ++i ) {
       m_transformedInplaceCallout[i].transform( matrix );
    }
}

void TextAnnotationPrivate::baseTransform( const QMatrix &matrix )
{
    AnnotationPrivate::baseTransform( matrix );

    for ( int i = 0; i < 3; ++i ) {
       m_inplaceCallout[i].transform( matrix );
    }
}

void TextAnnotationPrivate::resetTransformation()
{
    AnnotationPrivate::resetTransformation();

    for ( int i = 0; i < 3; ++i ) {
       m_transformedInplaceCallout[i] = m_inplaceCallout[i];
    }
}

void TextAnnotationPrivate::translate( const NormalizedPoint &coord )
{
    AnnotationPrivate::translate( coord );

#define ADD_COORD( c1, c2 ) \
{ \
  c1.x = c1.x + c2.x; \
  c1.y = c1.y + c2.y; \
}
    ADD_COORD( m_inplaceCallout[0], coord )
    ADD_COORD( m_inplaceCallout[1], coord )
    ADD_COORD( m_inplaceCallout[2], coord )
#undef ADD_COORD
}

/** LineAnnotation [Annotation] */

class Okular::LineAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        LineAnnotationPrivate()
            : AnnotationPrivate(),
              m_lineStartStyle( LineAnnotation::None ), m_lineEndStyle( LineAnnotation::None ),
              m_lineClosed( false ), m_lineShowCaption( false ), m_lineLeadingFwdPt( 0 ),
              m_lineLeadingBackPt( 0 ), m_lineIntent( LineAnnotation::Unknown )
        {
        }

        virtual void transform( const QMatrix &matrix );
        virtual void baseTransform( const QMatrix &matrix );
        virtual void resetTransformation();
        virtual void translate( const NormalizedPoint &coord );

        QLinkedList<NormalizedPoint> m_linePoints;
        QLinkedList<NormalizedPoint> m_transformedLinePoints;
        LineAnnotation::TermStyle m_lineStartStyle;
        LineAnnotation::TermStyle m_lineEndStyle;
        bool m_lineClosed : 1;
        bool m_lineShowCaption : 1;
        QColor m_lineInnerColor;
        double m_lineLeadingFwdPt;
        double m_lineLeadingBackPt;
        LineAnnotation::LineIntent m_lineIntent;
};

LineAnnotation::LineAnnotation()
    : Annotation( *new LineAnnotationPrivate() )
{
}

LineAnnotation::LineAnnotation( const QDomNode & node )
    : Annotation( *new LineAnnotationPrivate(), node )
{
    Q_D( LineAnnotation );
    // loop through the whole children looking for a 'line' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "line" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "startStyle" ) )
            d->m_lineStartStyle = (LineAnnotation::TermStyle)e.attribute( "startStyle" ).toInt();
        if ( e.hasAttribute( "endStyle" ) )
            d->m_lineEndStyle = (LineAnnotation::TermStyle)e.attribute( "endStyle" ).toInt();
        if ( e.hasAttribute( "closed" ) )
            d->m_lineClosed = e.attribute( "closed" ).toInt();
        if ( e.hasAttribute( "innerColor" ) )
            d->m_lineInnerColor = QColor( e.attribute( "innerColor" ) );
        if ( e.hasAttribute( "leadFwd" ) )
            d->m_lineLeadingFwdPt = e.attribute( "leadFwd" ).toDouble();
        if ( e.hasAttribute( "leadBack" ) )
            d->m_lineLeadingBackPt = e.attribute( "leadBack" ).toDouble();
        if ( e.hasAttribute( "showCaption" ) )
            d->m_lineShowCaption = e.attribute( "showCaption" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            d->m_lineIntent = (LineAnnotation::LineIntent)e.attribute( "intent" ).toInt();

        // parse all 'point' subnodes
        QDomNode pointNode = e.firstChild();
        while ( pointNode.isElement() )
        {
            QDomElement pe = pointNode.toElement();
            pointNode = pointNode.nextSibling();

            if ( pe.tagName() != "point" )
                continue;

            NormalizedPoint p;
            p.x = pe.attribute( "x", "0.0" ).toDouble();
            p.y = pe.attribute( "y", "0.0" ).toDouble();
            d->m_linePoints.append( p );
        }

        // loading complete
        break;
    }

    d->m_transformedLinePoints = d->m_linePoints;
}

LineAnnotation::~LineAnnotation()
{
}

void LineAnnotation::setLinePoints( const QLinkedList<NormalizedPoint> &points )
{
    Q_D( LineAnnotation );
    d->m_linePoints = points;
}

QLinkedList<NormalizedPoint> LineAnnotation::linePoints() const
{
    Q_D( const LineAnnotation );
    return d->m_linePoints;
}

QLinkedList<NormalizedPoint> LineAnnotation::transformedLinePoints() const
{
    Q_D( const LineAnnotation );
    return d->m_transformedLinePoints;
}

void LineAnnotation::setLineStartStyle( TermStyle style )
{
    Q_D( LineAnnotation );
    d->m_lineStartStyle = style;
}

LineAnnotation::TermStyle LineAnnotation::lineStartStyle() const
{
    Q_D( const LineAnnotation );
    return d->m_lineStartStyle;
}

void LineAnnotation::setLineEndStyle( TermStyle style )
{
    Q_D( LineAnnotation );
    d->m_lineEndStyle = style;
}

LineAnnotation::TermStyle LineAnnotation::lineEndStyle() const
{
    Q_D( const LineAnnotation );
    return d->m_lineEndStyle;
}

void LineAnnotation::setLineClosed( bool closed )
{
    Q_D( LineAnnotation );
    d->m_lineClosed = closed;
}

bool LineAnnotation::lineClosed() const
{
    Q_D( const LineAnnotation );
    return d->m_lineClosed;
}

void LineAnnotation::setLineInnerColor( const QColor &color )
{
    Q_D( LineAnnotation );
    d->m_lineInnerColor = color;
}

QColor LineAnnotation::lineInnerColor() const
{
    Q_D( const LineAnnotation );
    return d->m_lineInnerColor;
}

void LineAnnotation::setLineLeadingForwardPoint( double point )
{
    Q_D( LineAnnotation );
    d->m_lineLeadingFwdPt = point;
}

double LineAnnotation::lineLeadingForwardPoint() const
{
    Q_D( const LineAnnotation );
    return d->m_lineLeadingFwdPt;
}

void LineAnnotation::setLineLeadingBackwardPoint( double point )
{
    Q_D( LineAnnotation );
    d->m_lineLeadingBackPt = point;
}

double LineAnnotation::lineLeadingBackwardPoint() const
{
    Q_D( const LineAnnotation );
    return d->m_lineLeadingBackPt;
}

void LineAnnotation::setShowCaption( bool show )
{
    Q_D( LineAnnotation );
    d->m_lineShowCaption = show;
}

bool LineAnnotation::showCaption() const
{
    Q_D( const LineAnnotation );
    return d->m_lineShowCaption;
}

void LineAnnotation::setLineIntent( LineIntent intent )
{
    Q_D( LineAnnotation );
    d->m_lineIntent = intent;
}

LineAnnotation::LineIntent LineAnnotation::lineIntent() const
{
    Q_D( const LineAnnotation );
    return d->m_lineIntent;
}

Annotation::SubType LineAnnotation::subType() const
{
    return ALine;
}

void LineAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const LineAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [line] element
    QDomElement lineElement = document.createElement( "line" );
    node.appendChild( lineElement );

    // store the attributes
    if ( d->m_lineStartStyle != None )
        lineElement.setAttribute( "startStyle", (int)d->m_lineStartStyle );
    if ( d->m_lineEndStyle != None )
        lineElement.setAttribute( "endStyle", (int)d->m_lineEndStyle );
    if ( d->m_lineClosed )
        lineElement.setAttribute( "closed", d->m_lineClosed );
    if ( d->m_lineInnerColor.isValid() )
        lineElement.setAttribute( "innerColor", d->m_lineInnerColor.name() );
    if ( d->m_lineLeadingFwdPt != 0.0 )
        lineElement.setAttribute( "leadFwd", d->m_lineLeadingFwdPt );
    if ( d->m_lineLeadingBackPt != 0.0 )
        lineElement.setAttribute( "leadBack", d->m_lineLeadingBackPt );
    if ( d->m_lineShowCaption )
        lineElement.setAttribute( "showCaption", d->m_lineShowCaption );
    if ( d->m_lineIntent != Unknown )
        lineElement.setAttribute( "intent", d->m_lineIntent );

    // append the list of points
    int points = d->m_linePoints.count();
    if ( points > 1 )
    {
        QLinkedList<NormalizedPoint>::const_iterator it = d->m_linePoints.begin(), end = d->m_linePoints.end();
        while ( it != end )
        {
            const NormalizedPoint & p = *it;
            QDomElement pElement = document.createElement( "point" );
            lineElement.appendChild( pElement );
            pElement.setAttribute( "x", p.x );
            pElement.setAttribute( "y", p.y );
            it++; //to avoid loop
        }
    }
}

void LineAnnotationPrivate::transform( const QMatrix &matrix )
{
    AnnotationPrivate::transform( matrix );

    QMutableLinkedListIterator<NormalizedPoint> it( m_transformedLinePoints );
    while ( it.hasNext() )
        it.next().transform( matrix );
}

void LineAnnotationPrivate::baseTransform( const QMatrix &matrix )
{
    AnnotationPrivate::baseTransform( matrix );

    QMutableLinkedListIterator<NormalizedPoint> it( m_linePoints );
    while ( it.hasNext() )
        it.next().transform( matrix );
}

void LineAnnotationPrivate::resetTransformation()
{
    AnnotationPrivate::resetTransformation();

    m_transformedLinePoints = m_linePoints;
}

void LineAnnotationPrivate::translate( const NormalizedPoint &coord )
{
    AnnotationPrivate::translate( coord );

    QMutableLinkedListIterator<NormalizedPoint> it( m_linePoints );
    while ( it.hasNext() )
    {
        NormalizedPoint& p = it.next();
        p.x = p.x + coord.x;
        p.y = p.y + coord.y;
    }
}

/** GeomAnnotation [Annotation] */

class Okular::GeomAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        GeomAnnotationPrivate()
            : AnnotationPrivate(), m_geomType( GeomAnnotation::InscribedSquare )
        {
        }

        GeomAnnotation::GeomType m_geomType;
        QColor m_geomInnerColor;
};

GeomAnnotation::GeomAnnotation()
    : Annotation( *new GeomAnnotationPrivate() )
{
}

GeomAnnotation::GeomAnnotation( const QDomNode & node )
    : Annotation( *new GeomAnnotationPrivate(), node )
{
    Q_D( GeomAnnotation );
    // loop through the whole children looking for a 'geom' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "geom" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            d->m_geomType = (GeomAnnotation::GeomType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "color" ) )
            d->m_geomInnerColor = QColor( e.attribute( "color" ) );
        // compatibility
        if ( e.hasAttribute( "width" ) )
            d->m_style.setWidth( e.attribute( "width" ).toInt() );

        // loading complete
        break;
    }
}

GeomAnnotation::~GeomAnnotation()
{
}

void GeomAnnotation::setGeometricalType( GeomType type )
{
    Q_D( GeomAnnotation );
    d->m_geomType = type;
}

GeomAnnotation::GeomType GeomAnnotation::geometricalType() const
{
    Q_D( const GeomAnnotation );
    return d->m_geomType;
}

void GeomAnnotation::setGeometricalInnerColor( const QColor &color )
{
    Q_D( GeomAnnotation );
    d->m_geomInnerColor = color;
}

QColor GeomAnnotation::geometricalInnerColor() const
{
    Q_D( const GeomAnnotation );
    return d->m_geomInnerColor;
}

void GeomAnnotation::setGeometricalPointWidth( int width )
{
    Q_D( GeomAnnotation );
    d->m_style.setWidth( width );
}

int GeomAnnotation::geometricalPointWidth() const
{
    Q_D( const GeomAnnotation );
    return static_cast< int >( d->m_style.width() );
}

Annotation::SubType GeomAnnotation::subType() const
{
    return AGeom;
}

void GeomAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const GeomAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [geom] element
    QDomElement geomElement = document.createElement( "geom" );
    node.appendChild( geomElement );

    // append the optional attributes
    if ( d->m_geomType != InscribedSquare )
        geomElement.setAttribute( "type", (int)d->m_geomType );
    if ( d->m_geomInnerColor.isValid() )
        geomElement.setAttribute( "color", d->m_geomInnerColor.name() );
}

/** HighlightAnnotation [Annotation] */

class HighlightAnnotation::Quad::Private
{
    public:
        Private()
        {
        }

        NormalizedPoint m_points[4];
        NormalizedPoint m_transformedPoints[4];
        bool m_capStart : 1;
        bool m_capEnd : 1;
        double m_feather;
};

HighlightAnnotation::Quad::Quad()
    : d( new Private )
{
}

HighlightAnnotation::Quad::~Quad()
{
    delete d;
}

HighlightAnnotation::Quad::Quad( const Quad &other )
    : d( new Private )
{
    *d = *other.d;
}

HighlightAnnotation::Quad& HighlightAnnotation::Quad::operator=( const Quad &other )
{
    if ( this != &other )
        *d = *other.d;

    return *this;
}

void HighlightAnnotation::Quad::setPoint( const NormalizedPoint &point, int index )
{
    if ( index < 0 || index > 3 )
        return;

    d->m_points[ index ] = point;
}

NormalizedPoint HighlightAnnotation::Quad::point( int index ) const
{
    if ( index < 0 || index > 3 )
        return NormalizedPoint();

    return d->m_points[ index ];
}

NormalizedPoint HighlightAnnotation::Quad::transformedPoint( int index ) const
{
    if ( index < 0 || index > 3 )
        return NormalizedPoint();

    return d->m_transformedPoints[ index ];
}

void HighlightAnnotation::Quad::setCapStart( bool value )
{
    d->m_capStart = value;
}

bool HighlightAnnotation::Quad::capStart() const
{
    return d->m_capStart;
}

void HighlightAnnotation::Quad::setCapEnd( bool value )
{
    d->m_capEnd = value;
}

bool HighlightAnnotation::Quad::capEnd() const
{
    return d->m_capEnd;
}

void HighlightAnnotation::Quad::setFeather( double width )
{
    d->m_feather = width;
}

double HighlightAnnotation::Quad::feather() const
{
    return d->m_feather;
}

void HighlightAnnotation::Quad::transform( const QMatrix &matrix )
{
    for ( int i = 0; i < 4; ++i ) {
        d->m_transformedPoints[ i ] = d->m_points[ i ];
        d->m_transformedPoints[ i ].transform( matrix );
    }
}


class Okular::HighlightAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        HighlightAnnotationPrivate()
            : AnnotationPrivate(), m_highlightType( HighlightAnnotation::Highlight )
        {
        }

        virtual void transform( const QMatrix &matrix );
        virtual void baseTransform( const QMatrix &matrix );

        HighlightAnnotation::HighlightType m_highlightType;
        QList< HighlightAnnotation::Quad > m_highlightQuads;
};

HighlightAnnotation::HighlightAnnotation()
    : Annotation( *new HighlightAnnotationPrivate() )
{
}

HighlightAnnotation::HighlightAnnotation( const QDomNode & node )
    : Annotation( *new HighlightAnnotationPrivate(), node )
{
    Q_D( HighlightAnnotation );
    // loop through the whole children looking for a 'hl' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "hl" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            d->m_highlightType = (HighlightAnnotation::HighlightType)e.attribute( "type" ).toInt();

        // parse all 'quad' subnodes
        QDomNode quadNode = e.firstChild();
        for ( ; quadNode.isElement(); quadNode = quadNode.nextSibling() )
        {
            QDomElement qe = quadNode.toElement();
            if ( qe.tagName() != "quad" )
                continue;

            HighlightAnnotation::Quad q;
            q.setPoint( NormalizedPoint( qe.attribute( "ax", "0.0" ).toDouble(), qe.attribute( "ay", "0.0" ).toDouble() ), 0 );
            q.setPoint( NormalizedPoint( qe.attribute( "bx", "0.0" ).toDouble(), qe.attribute( "by", "0.0" ).toDouble() ), 1 );
            q.setPoint( NormalizedPoint( qe.attribute( "cx", "0.0" ).toDouble(), qe.attribute( "cy", "0.0" ).toDouble() ), 2 );
            q.setPoint( NormalizedPoint( qe.attribute( "dx", "0.0" ).toDouble(), qe.attribute( "dy", "0.0" ).toDouble() ), 3 );
            q.setCapStart( qe.hasAttribute( "start" ) );
            q.setCapEnd( qe.hasAttribute( "end" ) );
            q.setFeather( qe.attribute( "feather", "0.1" ).toDouble() );

            q.transform( QMatrix() );

            d->m_highlightQuads.append( q );
        }

        // loading complete
        break;
    }
}

HighlightAnnotation::~HighlightAnnotation()
{
}

void HighlightAnnotation::setHighlightType( HighlightType type )
{
    Q_D( HighlightAnnotation );
    d->m_highlightType = type;
}

HighlightAnnotation::HighlightType HighlightAnnotation::highlightType() const
{
    Q_D( const HighlightAnnotation );
    return d->m_highlightType;
}

QList< HighlightAnnotation::Quad > & HighlightAnnotation::highlightQuads()
{
    Q_D( HighlightAnnotation );
    return d->m_highlightQuads;
}

void HighlightAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const HighlightAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [hl] element
    QDomElement hlElement = document.createElement( "hl" );
    node.appendChild( hlElement );

    // append the optional attributes
    if ( d->m_highlightType != Highlight )
        hlElement.setAttribute( "type", (int)d->m_highlightType );
    if ( d->m_highlightQuads.count() < 1 )
        return;
    // append highlight quads, all children describe quads
    QList< Quad >::const_iterator it = d->m_highlightQuads.begin(), end = d->m_highlightQuads.end();
    for ( ; it != end; ++it )
    {
        QDomElement quadElement = document.createElement( "quad" );
        hlElement.appendChild( quadElement );
        const Quad & q = *it;
        quadElement.setAttribute( "ax", q.point( 0 ).x );
        quadElement.setAttribute( "ay", q.point( 0 ).y );
        quadElement.setAttribute( "bx", q.point( 1 ).x );
        quadElement.setAttribute( "by", q.point( 1 ).y );
        quadElement.setAttribute( "cx", q.point( 2 ).x );
        quadElement.setAttribute( "cy", q.point( 2 ).y );
        quadElement.setAttribute( "dx", q.point( 3 ).x );
        quadElement.setAttribute( "dy", q.point( 3 ).y );
        if ( q.capStart() )
            quadElement.setAttribute( "start", 1 );
        if ( q.capEnd() )
            quadElement.setAttribute( "end", 1 );
        quadElement.setAttribute( "feather", q.feather() );
    }
}

Annotation::SubType HighlightAnnotation::subType() const
{
    return AHighlight;
}

void HighlightAnnotationPrivate::transform( const QMatrix &matrix )
{
    AnnotationPrivate::transform( matrix );

    QMutableListIterator<HighlightAnnotation::Quad> it( m_highlightQuads );
    while ( it.hasNext() )
        it.next().transform( matrix );
}

void HighlightAnnotationPrivate::baseTransform( const QMatrix &matrix )
{
    AnnotationPrivate::baseTransform( matrix );

    QMutableListIterator<HighlightAnnotation::Quad> it( m_highlightQuads );
    while ( it.hasNext() )
        it.next().transform( matrix );
}

/** StampAnnotation [Annotation] */

class Okular::StampAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        StampAnnotationPrivate()
            : AnnotationPrivate(), m_stampIconName( "Draft" )
        {
        }

        QString m_stampIconName;
};

StampAnnotation::StampAnnotation()
    : Annotation( *new StampAnnotationPrivate() )
{
}

StampAnnotation::StampAnnotation( const QDomNode & node )
    : Annotation( *new StampAnnotationPrivate(), node )
{
    Q_D( StampAnnotation );
    // loop through the whole children looking for a 'stamp' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "stamp" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "icon" ) )
            d->m_stampIconName = e.attribute( "icon" );

        // loading complete
        break;
    }
}

StampAnnotation::~StampAnnotation()
{
}

void StampAnnotation::setStampIconName( const QString &name )
{
    Q_D( StampAnnotation );
    d->m_stampIconName = name;
}

QString StampAnnotation::stampIconName() const
{
    Q_D( const StampAnnotation );
    return d->m_stampIconName;
}

Annotation::SubType StampAnnotation::subType() const
{
    return AStamp;
}

void StampAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const StampAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [stamp] element
    QDomElement stampElement = document.createElement( "stamp" );
    node.appendChild( stampElement );

    // append the optional attributes
    if ( d->m_stampIconName != "Draft" )
        stampElement.setAttribute( "icon", d->m_stampIconName );
}

/** InkAnnotation [Annotation] */

class Okular::InkAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        InkAnnotationPrivate()
            : AnnotationPrivate()
        {
        }

        virtual void transform( const QMatrix &matrix );
        virtual void baseTransform( const QMatrix &matrix );
        virtual void resetTransformation();
        virtual void translate( const NormalizedPoint &coord );

        QList< QLinkedList<NormalizedPoint> > m_inkPaths;
        QList< QLinkedList<NormalizedPoint> > m_transformedInkPaths;
};

InkAnnotation::InkAnnotation()
    : Annotation( *new InkAnnotationPrivate() )
{
}

InkAnnotation::InkAnnotation( const QDomNode & node )
    : Annotation( *new InkAnnotationPrivate(), node )
{
    Q_D( InkAnnotation );
    // loop through the whole children looking for a 'ink' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "ink" )
            continue;

        // parse the 'path' subnodes
        QDomNode pathNode = e.firstChild();
        while ( pathNode.isElement() )
        {
            QDomElement pathElement = pathNode.toElement();
            pathNode = pathNode.nextSibling();

            if ( pathElement.tagName() != "path" )
                continue;

            // build each path parsing 'point' subnodes
            QLinkedList<NormalizedPoint> path;
            QDomNode pointNode = pathElement.firstChild();
            while ( pointNode.isElement() )
            {
                QDomElement pointElement = pointNode.toElement();
                pointNode = pointNode.nextSibling();

                if ( pointElement.tagName() != "point" )
                    continue;

                NormalizedPoint p;
                p.x = pointElement.attribute( "x", "0.0" ).toDouble();
                p.y = pointElement.attribute( "y", "0.0" ).toDouble();
                path.append( p );
            }

            // add the path to the path list if it contains at least 2 nodes
            if ( path.count() >= 2 )
                d->m_inkPaths.append( path );
        }

        // loading complete
        break;
    }

    d->m_transformedInkPaths = d->m_inkPaths;
}

InkAnnotation::~InkAnnotation()
{
}

void InkAnnotation::setInkPaths( const QList< QLinkedList<NormalizedPoint> > &paths )
{
    Q_D( InkAnnotation );
    d->m_inkPaths = paths;
}

QList< QLinkedList<NormalizedPoint> > InkAnnotation::inkPaths() const
{
    Q_D( const InkAnnotation );
    return d->m_inkPaths;
}

QList< QLinkedList<NormalizedPoint> > InkAnnotation::transformedInkPaths() const
{
    Q_D( const InkAnnotation );
    return d->m_transformedInkPaths;
}

Annotation::SubType InkAnnotation::subType() const
{
    return AInk;
}

void InkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const InkAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [ink] element
    QDomElement inkElement = document.createElement( "ink" );
    node.appendChild( inkElement );

    // append the optional attributes
    if ( d->m_inkPaths.count() < 1 )
        return;

    QList< QLinkedList<NormalizedPoint> >::const_iterator pIt = d->m_inkPaths.begin(), pEnd = d->m_inkPaths.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        QDomElement pathElement = document.createElement( "path" );
        inkElement.appendChild( pathElement );
        const QLinkedList<NormalizedPoint> & path = *pIt;
        QLinkedList<NormalizedPoint>::const_iterator iIt = path.begin(), iEnd = path.end();
        for ( ; iIt != iEnd; ++iIt )
        {
            const NormalizedPoint & point = *iIt;
            QDomElement pointElement = document.createElement( "point" );
            pathElement.appendChild( pointElement );
            pointElement.setAttribute( "x", point.x );
            pointElement.setAttribute( "y", point.y );
        }
    }
}

void InkAnnotationPrivate::transform( const QMatrix &matrix )
{
    AnnotationPrivate::transform( matrix );

    for ( int i = 0; i < m_transformedInkPaths.count(); ++i )
    {
        QMutableLinkedListIterator<NormalizedPoint> it( m_transformedInkPaths[ i ] );
        while ( it.hasNext() )
            it.next().transform( matrix );
    }
}

void InkAnnotationPrivate::baseTransform( const QMatrix &matrix )
{
    AnnotationPrivate::baseTransform( matrix );

    for ( int i = 0; i < m_inkPaths.count(); ++i )
    {
        QMutableLinkedListIterator<NormalizedPoint> it( m_inkPaths[ i ] );
        while ( it.hasNext() )
            it.next().transform( matrix );
    }
}

void InkAnnotationPrivate::resetTransformation()
{
    AnnotationPrivate::resetTransformation();

    m_transformedInkPaths = m_inkPaths;
}

void InkAnnotationPrivate::translate( const NormalizedPoint &coord )
{
    AnnotationPrivate::translate( coord );

    for ( int i = 0; i < m_inkPaths.count(); ++i )
    {
        QMutableLinkedListIterator<NormalizedPoint> it( m_inkPaths[ i ] );
        while ( it.hasNext() )
        {
            NormalizedPoint& p = it.next();
            p.x = p.x + coord.x;
            p.y = p.y + coord.y;
        }
    }
}

/** CaretAnnotation [Annotation] */

class Okular::CaretAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        CaretAnnotationPrivate()
            : AnnotationPrivate(), m_symbol( CaretAnnotation::None )
        {
        }

        CaretAnnotation::CaretSymbol m_symbol;
};

static QString caretSymbolToString( CaretAnnotation::CaretSymbol symbol )
{
    switch ( symbol )
    {
        case CaretAnnotation::None:
            return QString::fromLatin1( "None" );
        case CaretAnnotation::P:
            return QString::fromLatin1( "P" );
    }
    return QString();
}

static CaretAnnotation::CaretSymbol caretSymbolFromString( const QString &symbol )
{
    if ( symbol == QLatin1String( "None" ) )
        return CaretAnnotation::None;
    else if ( symbol == QLatin1String( "P" ) )
        return CaretAnnotation::P;
    return CaretAnnotation::None;
}

CaretAnnotation::CaretAnnotation()
    : Annotation( *new CaretAnnotationPrivate() )
{
}

CaretAnnotation::CaretAnnotation( const QDomNode & node )
    : Annotation( *new CaretAnnotationPrivate(), node )
{
    Q_D( CaretAnnotation );
    // loop through the whole children looking for a 'caret' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "caret" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "symbol" ) )
            d->m_symbol = caretSymbolFromString( e.attribute( "symbol" ) );

        // loading complete
        break;
    }
}

CaretAnnotation::~CaretAnnotation()
{
}

void CaretAnnotation::setCaretSymbol( CaretAnnotation::CaretSymbol symbol )
{
    Q_D( CaretAnnotation );
    d->m_symbol = symbol;
}

CaretAnnotation::CaretSymbol CaretAnnotation::caretSymbol() const
{
    Q_D( const CaretAnnotation );
    return d->m_symbol;
}

Annotation::SubType CaretAnnotation::subType() const
{
    return ACaret;
}

void CaretAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    Q_D( const CaretAnnotation );
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [caret] element
    QDomElement caretElement = document.createElement( "caret" );
    node.appendChild( caretElement );

    // append the optional attributes
    if ( d->m_symbol != None )
        caretElement.setAttribute( "symbol", caretSymbolToString( d->m_symbol ) );
}

/** FileAttachmentAnnotation [Annotation] */

class Okular::FileAttachmentAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        FileAttachmentAnnotationPrivate()
            : AnnotationPrivate(), icon( "PushPin" ), embfile( 0 )
        {
        }
        ~FileAttachmentAnnotationPrivate()
        {
            delete embfile;
        }

        // data fields
        QString icon;
        EmbeddedFile *embfile;
};

FileAttachmentAnnotation::FileAttachmentAnnotation()
    : Annotation( *new FileAttachmentAnnotationPrivate() )
{
}

FileAttachmentAnnotation::FileAttachmentAnnotation( const QDomNode & node )
    : Annotation( *new FileAttachmentAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'fileattachment' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "fileattachment" )
            continue;

        // loading complete
        break;
    }
}

FileAttachmentAnnotation::~FileAttachmentAnnotation()
{
}

void FileAttachmentAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [fileattachment] element
    QDomElement fileAttachmentElement = document.createElement( "fileattachment" );
    node.appendChild( fileAttachmentElement );
}

Annotation::SubType FileAttachmentAnnotation::subType() const
{
    return AFileAttachment;
}

QString FileAttachmentAnnotation::fileIconName() const
{
    Q_D( const FileAttachmentAnnotation );
    return d->icon;
}

void FileAttachmentAnnotation::setFileIconName( const QString &icon )
{
    Q_D( FileAttachmentAnnotation );
    d->icon = icon;
}

EmbeddedFile* FileAttachmentAnnotation::embeddedFile() const
{
    Q_D( const FileAttachmentAnnotation );
    return d->embfile;
}

void FileAttachmentAnnotation::setEmbeddedFile( EmbeddedFile *ef )
{
    Q_D( FileAttachmentAnnotation );
    d->embfile = ef;
}

/** SoundAnnotation [Annotation] */

class Okular::SoundAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        SoundAnnotationPrivate()
            : AnnotationPrivate(), icon( "Speaker" ), sound( 0 )
        {
        }
        ~SoundAnnotationPrivate()
        {
            delete sound;
        }

        // data fields
        QString icon;
        Sound *sound;
};

SoundAnnotation::SoundAnnotation()
    : Annotation( *new SoundAnnotationPrivate() )
{
}

SoundAnnotation::SoundAnnotation( const QDomNode & node )
    : Annotation( *new SoundAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'sound' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "sound" )
            continue;

        // loading complete
        break;
    }
}

SoundAnnotation::~SoundAnnotation()
{
}

void SoundAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [sound] element
    QDomElement soundElement = document.createElement( "sound" );
    node.appendChild( soundElement );
}

Annotation::SubType SoundAnnotation::subType() const
{
    return ASound;
}

QString SoundAnnotation::soundIconName() const
{
    Q_D( const SoundAnnotation );
    return d->icon;
}

void SoundAnnotation::setSoundIconName( const QString &icon )
{
    Q_D( SoundAnnotation );
    d->icon = icon;
}

Sound* SoundAnnotation::sound() const
{
    Q_D( const SoundAnnotation );
    return d->sound;
}

void SoundAnnotation::setSound( Sound *s )
{
    Q_D( SoundAnnotation );
    d->sound = s;
}

/** MovieAnnotation [Annotation] */

class Okular::MovieAnnotationPrivate : public Okular::AnnotationPrivate
{
    public:
        MovieAnnotationPrivate()
            : AnnotationPrivate(), movie( 0 )
        {
        }
        ~MovieAnnotationPrivate()
        {
            delete movie;
        }

        // data fields
        Movie *movie;
};

MovieAnnotation::MovieAnnotation()
    : Annotation( *new MovieAnnotationPrivate() )
{
}

MovieAnnotation::MovieAnnotation( const QDomNode & node )
    : Annotation( *new MovieAnnotationPrivate(), node )
{
    // loop through the whole children looking for a 'movie' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "movie" )
            continue;

        // loading complete
        break;
    }
}

MovieAnnotation::~MovieAnnotation()
{
}

void MovieAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [movie] element
    QDomElement movieElement = document.createElement( "movie" );
    node.appendChild( movieElement );
}

Annotation::SubType MovieAnnotation::subType() const
{
    return AMovie;
}

Movie* MovieAnnotation::movie() const
{
    Q_D( const MovieAnnotation );
    return d->movie;
}

void MovieAnnotation::setMovie( Movie *movie )
{
    Q_D( MovieAnnotation );
    d->movie = movie;
}
