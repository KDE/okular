/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qdom.h>
#include <qcolor.h>
#include <qapplication.h>
#include <kdebug.h>

// local includes
#include "annotations.h"


/** AnnotationManager **/

Annotation * AnnotationManager::createAnnotation( const QDomElement & annElement )
{
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
    }

    // safety check
    if ( !annotation )
        return 0;

    // if annotations contains previous revisions, recursively restore them
    QDomNode childNode = annElement.firstChild();
    while ( childNode.isElement() )
    {
        QDomElement childElement = childNode.toElement();
        childNode = childNode.nextSibling();

        if ( childElement.tagName() == "annotation" )
        {
            //Annotation * prevRevision = createAnnotation( childElement );
            //TODO restore additional prevRevision parameters too
            /*if ( prevRevision )
            {
                annotation->prevRevision = new AnnRevision();
                annotation->prevRevision->annotation = prevRevision;
        }*/
            break;
        }
    }

    // return created annotation
    return annotation;
}

void AnnotationManager::storeAnnotation( const Annotation * ann, QDomNode & parentNode,
    QDomDocument & document )
{
    // create annotation element and append it to parent
    QDomElement annotElement = document.createElement( "annotation" );
    parentNode.appendChild( annotElement );

    // save annotation's type and states
    annotElement.setAttribute( "type", (uint)ann->subType() );
    ann->store( annotElement, document );

    // if annotation has a previous revision, recourse saving it
    //if ( ann->prevRevision && ann->prevRevision->annotation )
    //    storeAnnotation( ann->prevRevision->annotation, annotElement, document );
}


/** Annotation **/
/*
Annotation::DrawStyle::DrawStyle()
    : style( Solid ), effect( NoEffect ), width( 1 ), xCornerRadius( 0 ),
    yCornerRadius( 0 ), dashMarks( 3 ), dashSpaces( 0 ),  effectIntensity( 0 ) {}

Annotation::Window::Window()
    : opened( false ), flags( 0 ) {}

Annotation::Revision::Revision()
    : nextRevision( 0 ), scope( Reply ), stateModel( Mark ), state( Unmarked ) {}
*/

Annotation::Annotation()
    : flags( 0 )
{
}

Annotation::Annotation( const QDomNode & /*node*/ )
    : flags( 0 )
{/*
    // loop through the whole children looking for a 'base' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "base" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "author" ) )
            author = e.attribute( "author" );
        if ( e.hasAttribute( "contents" ) )
            contents = e.attribute( "contents" );
        if ( e.hasAttribute( "uniqueName" ) )
            uniqueName = e.attribute( "uniqueName" );
        if ( e.hasAttribute( "modifyDate" ) )
            modifyDate = QDateTime::fromString( e.attribute( "modifyDate" ) );
        if ( e.hasAttribute( "flags" ) )
            flags = (int)e.attribute( "flags" ).toInt();
        if ( e.hasAttribute( "color" ) )
            color = QColor( e.attribute( "color" ) );

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "rect" )
            {
                r.left = ee.attribute( "l" ).toDouble();
                r.top = ee.attribute( "t" ).toDouble();
                r.right = ee.attribute( "r" ).toDouble();
                r.bottom = ee.attribute( "b" ).toDouble();
                if ( ee.hasAttribute( "uw" ) )
                    rUnscaledWidth = ee.attribute( "uw" ).toDouble();
                if ( ee.hasAttribute( "uh" ) )
                    rUnscaledHeight = ee.attribute( "uh" ).toDouble();
            }
            else if ( ee.tagName() == "drawStyle" )
            {
                drawStyle.width = ee.attribute( "width" ).toDouble();
                drawStyle.xCornerRadius = ee.attribute( "xCR" ).toDouble();
                drawStyle.yCornerRadius = ee.attribute( "yCR" ).toDouble();
                drawStyle.style = (DrawStyle::S)ee.attribute( "style" ).toInt();
                drawStyle.dashMarks = ee.attribute( "dashMarks" ).toInt();
                drawStyle.dashSpaces = ee.attribute( "dashSpaces" ).toInt();
                drawStyle.effect = (DrawStyle::E)ee.attribute( "effect" ).toInt();
                drawStyle.effectIntensity = ee.attribute( "intensity" ).toInt();
            }
        }
 */
        /** da WindowAnnotation
        // loop through the whole children looking for a 'window' element
        QDomNode subNode = node.firstChild();
        while( subNode.isElement() )
        {
            QDomElement e = subNode.toElement();
            subNode = subNode.nextSibling();
            if ( e.tagName() != "window" )
                continue;

            // parse the attributes
            if ( e.hasAttribute( "parent" ) )
                windowMarkupParentID = e.attribute( "parent" ).toInt();
            if ( e.hasAttribute( "opened" ) )
                windowOpened = e.attribute( "opened" ).toInt();

            // loading complete
            break;
        }*/
    /*
        // loading complete
        break;
    }*/
}

void Annotation::store( QDomNode & /*node*/, QDomDocument & /*document*/ ) const
{/*
    // create [base] element (child of 'Annotation' node)
    QDomElement baseElement = document.createElement( "base" );
    node.appendChild( baseElement );

    // append rect as a subnode
    QDomElement rectElement = document.createElement( "rect" );
    baseElement.appendChild( rectElement );
    rectElement.setAttribute( "l", (double)r.left );
    rectElement.setAttribute( "t", (double)r.top );
    rectElement.setAttribute( "r", (double)r.right );
    rectElement.setAttribute( "b", (double)r.bottom );
    if ( rUnscaledWidth > 0 )
        rectElement.setAttribute( "uw", (double)rUnscaledWidth );
    if ( rUnscaledHeight > 0 )
        rectElement.setAttribute( "uh", (double)rUnscaledHeight );

    // append the optional attributes
    if ( !author.isEmpty() )
        baseElement.setAttribute( "author", author );
    if ( !contents.isEmpty() )
        baseElement.setAttribute( "contents", contents );
    if ( !uniqueName.isEmpty() )
        baseElement.setAttribute( "uniqueName", uniqueName );
    if ( modifyDate.isValid() )
        baseElement.setAttribute( "modifyDate", modifyDate.toString() );
    if ( flags )
        baseElement.setAttribute( "flags", flags );
    if ( drawStyle.width != 1 )
    {
        QDomElement styleElement = document.createElement( "drawStyle" );
        baseElement.appendChild( styleElement );
        styleElement.setAttribute( "width", drawStyle.width );
        styleElement.setAttribute( "xCR", drawStyle.xCornerRadius );
        styleElement.setAttribute( "yCR", drawStyle.yCornerRadius );
        styleElement.setAttribute( "style", (int)drawStyle.style );
        styleElement.setAttribute( "dashMarks", drawStyle.dashMarks );
        styleElement.setAttribute( "dashSpaces", drawStyle.dashSpaces );
        styleElement.setAttribute( "effect", (int)drawStyle.effect );
        styleElement.setAttribute( "intensity", drawStyle.effectIntensity );
    }
    if ( color.isValid() )
        baseElement.setAttribute( "color", color.name() );
        */
    /** from WindowAnnotation
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [window] element
    QDomElement windowElement = document.createElement( "window" );
    node.appendChild( windowElement );

    // append the optional attributes
    if ( windowMarkupParentID != -1 )
        windowElement.setAttribute( "parent", windowMarkupParentID );
    if ( windowOpened )
        windowElement.setAttribute( "opened", windowOpened );
    */
}


/** MarkupAnnotation ** Annotation */

/*
MarkupAnnotation::MarkupAnnotation()
    : Annotation(), markupOpacity( 1.0 )
{}

MarkupAnnotation::MarkupAnnotation( const QDomNode & node )
    : Annotation( node ), markupOpacity( 1.0 )
{
    // loop through the whole children looking for a 'markup' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "markup" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "opacity" ) )
            markupOpacity = e.attribute( "opacity" ).toDouble();
        if ( e.hasAttribute( "creationDate" ) )
            markupCreationDate = QDateTime::fromString( e.attribute( "creationDate" ) );

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "window" )
            {
                // parse window attributes
                if ( ee.hasAttribute( "opened" ) )
                    markupWindow.opened = (bool)ee.attribute( "opened" ).toInt();
                if ( ee.hasAttribute( "flags" ) )
                    markupWindow.flags = ee.attribute( "flags" ).toInt();
                if ( ee.hasAttribute( "title" ) )
                    markupWindow.title = ee.attribute( "title" );
                if ( ee.hasAttribute( "summary" ) )
                    markupWindow.summary = ee.attribute( "summary" );

                // parse window subnodes
                QDomNode eeSubNode = ee.firstChild();
                while ( eeSubNode.isElement() )
                {
                    QDomElement eee = eeSubNode.toElement();
                    eeSubNode = eeSubNode.nextSibling();

                    if ( eee.tagName() == "rect" )
                    {
                        markupWindow.r.left = eee.attribute( "l" ).toDouble();
                        markupWindow.r.top = eee.attribute( "t" ).toDouble();
                        markupWindow.r.right = eee.attribute( "r" ).toDouble();
                        markupWindow.r.bottom = eee.attribute( "b" ).toDouble();
                    }
                    else if ( eee.tagName() == "escapedText" )
                    {
                        markupWindow.text = eee.firstChild().toCDATASection().data();
                    }
                }
            }
            else if ( ee.tagName() == "revision" )
            {
                //TODO HERE - recursive rev parse!
                markupRevision.scope = (Revision::S)ee.attribute( "scope" ).toInt();
                markupRevision.stateModel = (Revision::M)ee.attribute( "stateModel" ).toInt();
                markupRevision.state = (Revision::T)ee.attribute( "state" ).toInt();
            }
        }

        // loading complete
        break;
    }
}

void MarkupAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [markup] element
    QDomElement markupElement = document.createElement( "markup" );
    node.appendChild( markupElement );

    // append the optional attributes
    if ( markupOpacity != 1.0 )
        markupElement.setAttribute( "opacity", markupOpacity );
    if ( markupCreationDate.isValid() )
        markupElement.setAttribute( "creationDate", markupCreationDate.toString() );

    // append the window attributes
    QDomElement winElement = document.createElement( "window" );
    markupElement.appendChild( winElement );
    if ( markupWindow.opened )
        winElement.setAttribute( "opened", markupWindow.opened );
    if ( markupWindow.flags )
        winElement.setAttribute( "flags", markupWindow.flags );
    if ( !markupWindow.r.isNull() )
    {
        QDomElement rectElement = document.createElement( "rect" );
        winElement.appendChild( rectElement );
        rectElement.setAttribute( "l", (double)markupWindow.r.left );
        rectElement.setAttribute( "t", (double)markupWindow.r.top );
        rectElement.setAttribute( "r", (double)markupWindow.r.right );
        rectElement.setAttribute( "b", (double)markupWindow.r.bottom );
    }
    if ( !markupWindow.title.isEmpty() )
        winElement.setAttribute( "title", markupWindow.title );
    if ( !markupWindow.summary.isEmpty() )
        markupElement.setAttribute( "summary", markupWindow.summary );
    if ( !markupWindow.text.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        winElement.appendChild( escapedText );
        QDomCDATASection cdataText = document.createCDATASection( markupWindow.text );
        escapedText.appendChild( cdataText );
    }

    // append the revision(s) recursively
    if ( markupRevision.nextRevision )
    {
        QDomElement revElement = document.createElement( "revision" );
        markupElement.appendChild( revElement );
        //FIXME revElement.setAttribute( "ID", .ID );
        revElement.setAttribute( "scope", (int)markupRevision.scope );
        revElement.setAttribute( "stateModel", (int)markupRevision.stateModel );
        revElement.setAttribute( "state", (int)markupRevision.state );
    }
}
*/

/** TextAnnotation ** Annotation */

TextAnnotation::TextAnnotation()
    : Annotation(), textType( Linked ), textFont(),
    textOpened( false ), textIcon( "Note" ), inplaceAlign( 0 ),
    inplaceIntent( Unknown )
{}

TextAnnotation::TextAnnotation( const QDomNode & node )
    : Annotation( node ), textType( Linked ), textFont(),
    textOpened( false ), textIcon( "Note" ), inplaceAlign( 0 ),
    inplaceIntent( Unknown )
{
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
            textType = (TextAnnotation::TextType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "font" ) )
            textFont.fromString( e.attribute( "font" ) );
        if ( e.hasAttribute( "opened" ) )
            textOpened = e.attribute( "opened" ).toInt();
        if ( e.hasAttribute( "icon" ) )
            textIcon = e.attribute( "icon" );
        if ( e.hasAttribute( "align" ) )
            inplaceAlign = e.attribute( "align" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            inplaceIntent = (TextAnnotation::InplaceIntent)e.attribute( "intent" ).toInt();

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                inplaceText = ee.firstChild().toCDATASection().data();
            }
            else if ( ee.tagName() == "callout" )
            {
                inplaceCallout[0].x = ee.attribute( "ax" ).toDouble();
                inplaceCallout[0].y = ee.attribute( "ay" ).toDouble();
                inplaceCallout[1].x = ee.attribute( "bx" ).toDouble();
                inplaceCallout[1].y = ee.attribute( "by" ).toDouble();
                inplaceCallout[2].x = ee.attribute( "cx" ).toDouble();
                inplaceCallout[2].y = ee.attribute( "cy" ).toDouble();
            }
        }

        // loading complete
        break;
    }
}

void TextAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [text] element
    QDomElement textElement = document.createElement( "text" );
    node.appendChild( textElement );

    // append the optional attributes
    if ( textType != Linked )
        textElement.setAttribute( "type", (int)textType );
    if ( textFont != QApplication::font() )
        textElement.setAttribute( "font", textFont.toString() );
    if ( textOpened )
        textElement.setAttribute( "opened", textOpened );
    if ( !textIcon.isEmpty() )
        textElement.setAttribute( "icon", textIcon );
    if ( inplaceAlign )
        textElement.setAttribute( "align", inplaceAlign );
    if ( !inplaceText.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        textElement.appendChild( escapedText );
        QDomCDATASection cdataText = document.createCDATASection( inplaceText );
        escapedText.appendChild( cdataText );
    }
    if ( inplaceCallout[0].x != 0.0 )
    {
        QDomElement calloutElement = document.createElement( "callout" );
        textElement.appendChild( calloutElement );
        calloutElement.setAttribute( "ax", inplaceCallout[0].x );
        calloutElement.setAttribute( "ay", inplaceCallout[0].y );
        calloutElement.setAttribute( "bx", inplaceCallout[1].x );
        calloutElement.setAttribute( "by", inplaceCallout[1].y );
        calloutElement.setAttribute( "cx", inplaceCallout[2].x );
        calloutElement.setAttribute( "cy", inplaceCallout[2].y );
    }
    if ( inplaceIntent != Unknown )
        textElement.setAttribute( "intent", (int)inplaceIntent );
}


/** LineAnnotation ** Annotation */

LineAnnotation::LineAnnotation()
    : Annotation(), lineStartStyle( None ), lineEndStyle( None ),
    lineClosed( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineShowCaption( false ), lineIntent( Unknown )
{}

LineAnnotation::LineAnnotation( const QDomNode & node )
    : Annotation( node ), lineStartStyle( None ), lineEndStyle( None ),
    lineClosed( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineShowCaption( false ), lineIntent( Unknown )
{
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
            lineStartStyle = (LineAnnotation::TermStyle)e.attribute( "startStyle" ).toInt();
        if ( e.hasAttribute( "endStyle" ) )
            lineEndStyle = (LineAnnotation::TermStyle)e.attribute( "endStyle" ).toInt();
        if ( e.hasAttribute( "closed" ) )
            lineClosed = e.attribute( "closed" ).toInt();
        if ( e.hasAttribute( "innerColor" ) )
            lineInnerColor = QColor( e.attribute( "innerColor" ) );
        if ( e.hasAttribute( "leadFwd" ) )
            lineLeadingFwdPt = e.attribute( "leadFwd" ).toDouble();
        if ( e.hasAttribute( "leadBack" ) )
            lineLeadingBackPt = e.attribute( "leadBack" ).toDouble();
        if ( e.hasAttribute( "showCaption" ) )
            lineShowCaption = e.attribute( "showCaption" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            lineIntent = (LineAnnotation::LineIntent)e.attribute( "intent" ).toInt();

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
            linePoints.append( p );
        }

        // loading complete
        break;
    }
}

void LineAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [line] element
    QDomElement lineElement = document.createElement( "line" );
    node.appendChild( lineElement );

    // append the list of points
    int points = linePoints.count();
    if ( points > 1 )
    {
        QValueList<NormalizedPoint>::const_iterator it = linePoints.begin(), end = linePoints.end();
        while ( it != end )
        {
            const NormalizedPoint & p = *it;
            QDomElement pElement = document.createElement( "point" );
            lineElement.appendChild( pElement );
            pElement.setAttribute( "x", p.x );
            pElement.setAttribute( "y", p.y );
        }
    }

    // append the optional attributes
    if ( lineStartStyle != None )
        lineElement.setAttribute( "startStyle", (int)lineStartStyle );
    if ( lineEndStyle != None )
        lineElement.setAttribute( "endStyle", (int)lineEndStyle );
    if ( lineClosed )
        lineElement.setAttribute( "closed", lineClosed );
    if ( lineInnerColor.isValid() )
        lineElement.setAttribute( "innerColor", lineInnerColor.name() );
    if ( lineLeadingFwdPt != 0.0 )
        lineElement.setAttribute( "leadFwd", lineLeadingFwdPt );
    if ( lineLeadingBackPt != 0.0 )
        lineElement.setAttribute( "leadBack", lineLeadingBackPt );
    if ( lineShowCaption )
        lineElement.setAttribute( "showCaption", lineShowCaption );
    if ( lineIntent != Unknown )
        lineElement.setAttribute( "intent", lineIntent );
}


/** GeomAnnotation ** Annotation */

GeomAnnotation::GeomAnnotation()
    : Annotation(), geomType( InscribedSquare ), geomWidthPt( 18 )
{}

GeomAnnotation::GeomAnnotation( const QDomNode & node )
    : Annotation( node ), geomType( InscribedSquare ), geomWidthPt( 18 )
{
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
            geomType = (GeomAnnotation::GeomType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "color" ) )
            geomInnerColor = QColor( e.attribute( "color" ) );
        if ( e.hasAttribute( "width" ) )
            geomWidthPt = e.attribute( "width" ).toInt();

        // loading complete
        break;
    }
}

void GeomAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [geom] element
    QDomElement geomElement = document.createElement( "geom" );
    node.appendChild( geomElement );

    // append the optional attributes
    if ( geomType != InscribedSquare )
        geomElement.setAttribute( "type", (int)geomType );
    if ( geomInnerColor.isValid() )
        geomElement.setAttribute( "color", geomInnerColor.name() );
    if ( geomWidthPt != 18 )
        geomElement.setAttribute( "width", geomWidthPt );
}


/** HighlightAnnotation ** Annotation */

HighlightAnnotation::HighlightAnnotation()
    : Annotation(), highlightType( Highlight )
{}

HighlightAnnotation::HighlightAnnotation( const QDomNode & node )
    : Annotation( node ), highlightType( Highlight )
{
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
            highlightType = (HighlightAnnotation::HighlightType)e.attribute( "type" ).toInt();

        // parse all 'quad' subnodes
        QDomNode quadNode = e.firstChild();
        while ( quadNode.isElement() )
        {
            QDomElement qe = quadNode.toElement();
            quadNode = quadNode.nextSibling();

            if ( qe.tagName() != "quad" )
                continue;

            NormalizedPoint p[4];
            p[0].x = qe.attribute( "ax", "0.0" ).toDouble();
            p[0].y = qe.attribute( "ay", "0.0" ).toDouble();
            p[1].x = qe.attribute( "bx", "0.0" ).toDouble();
            p[1].y = qe.attribute( "by", "0.0" ).toDouble();
            p[2].x = qe.attribute( "cx", "0.0" ).toDouble();
            p[2].y = qe.attribute( "cy", "0.0" ).toDouble();
            p[3].x = qe.attribute( "dx", "0.0" ).toDouble();
            p[3].y = qe.attribute( "dy", "0.0" ).toDouble();
            highlightQuads.append( p );
        }

        // loading complete
        break;
    }
}

void HighlightAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [hl] element
    QDomElement hlElement = document.createElement( "hl" );
    node.appendChild( hlElement );

    // append the optional attributes
    if ( highlightType != Highlight )
        hlElement.setAttribute( "type", (int)highlightType );
    if ( highlightQuads.count() < 1 )
        return;
    // append highlight quads, all children describe quads
    QValueList<NormalizedPoint[4]>::const_iterator it = highlightQuads.begin(), end = highlightQuads.end();
    for ( ; it != end; ++it )
    {
        QDomElement quadElement = document.createElement( "quad" );
        hlElement.appendChild( quadElement );
        const NormalizedPoint * p = *it;
        quadElement.setAttribute( "ax", p[0].x );
        quadElement.setAttribute( "ay", p[0].y );
        quadElement.setAttribute( "bx", p[1].x );
        quadElement.setAttribute( "by", p[1].y );
        quadElement.setAttribute( "cx", p[2].x );
        quadElement.setAttribute( "cy", p[2].y );
        quadElement.setAttribute( "dx", p[3].x );
        quadElement.setAttribute( "dy", p[3].y );
    }
}


/** StampAnnotation ** Annotation */

StampAnnotation::StampAnnotation()
    : Annotation(), stampIconName( "kpdf" )
{}

StampAnnotation::StampAnnotation( const QDomNode & node )
    : Annotation( node ), stampIconName( "kpdf" )
{
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
            stampIconName = e.attribute( "icon" );

        // loading complete
        break;
    }
}

void StampAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [stamp] element
    QDomElement stampElement = document.createElement( "stamp" );
    node.appendChild( stampElement );

    // append the optional attributes
    if ( !stampIconName.isEmpty() )
        stampElement.setAttribute( "icon", stampIconName );
}


/** InkAnnotation ** Annotation */

InkAnnotation::InkAnnotation()
    : Annotation()
{}

InkAnnotation::InkAnnotation( const QDomNode & node )
    : Annotation( node )
{
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
            QValueList<NormalizedPoint> path;
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
                inkPaths.append( path );
        }

        // loading complete
        break;
    }
}

void InkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [ink] element
    QDomElement inkElement = document.createElement( "ink" );
    node.appendChild( inkElement );

    // append the optional attributes
    if ( inkPaths.count() < 1 )
        return;
    QValueList< QValueList<NormalizedPoint> >::const_iterator pIt = inkPaths.begin(), pEnd = inkPaths.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        QDomElement pathElement = document.createElement( "path" );
        inkElement.appendChild( pathElement );
        const QValueList<NormalizedPoint> & path = *pIt;
        QValueList<NormalizedPoint>::const_iterator iIt = path.begin(), iEnd = path.end();
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
