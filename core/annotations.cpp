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

/** @short What's that bloated file? */
/* In this file there is the implementation of Annotation and all its children
 * objects. For each object we do:
 *  1A) perform default initialization of data        // both done recurring \\
 *  1B) initialize data taking values from a XML node \\ to parent(s) too    //
 *   2) save data to XML node
 **/

/** Annotation **/

Annotation::DrawStyle::DrawStyle()
    : style( Solid ), effect( NoEffect ), width( 1 ), xCornerRadius( 0 ),
    yCornerRadius( 0 ), dashMarks( 3 ), dashSpaces( 0 ),  effectIntensity( 0 ) {}

Annotation::Annotation()
    : rUnscaledWidth( -1 ), rUnscaledHeight( -1 ), flags( 0 ), external( false )
{}

Annotation::Annotation( const QDomNode & node )
    : rUnscaledWidth( -1 ), rUnscaledHeight( -1 ), flags( 0 ), external( false )
{
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

        // loading complete
        break;
    }
}

void Annotation::store( QDomNode & node, QDomDocument & document ) const
{
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
}


/** MarkupAnnotation ** Annotation */

MarkupAnnotation::InReplyTo::InReplyTo()
    : ID( -1 ), scope( Reply ), stateModel( Mark ), state( Unmarked ) {}

MarkupAnnotation::MarkupAnnotation()
    : Annotation(), markupOpacity( 1.0 ), markupWindowID( -1 )
{}

MarkupAnnotation::MarkupAnnotation( const QDomNode & node )
    : Annotation( node ), markupOpacity( 1.0 ), markupWindowID( -1 )
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
        if ( e.hasAttribute( "refID" ) )
            markupWindowID = e.attribute( "refID" ).toInt();
        if ( e.hasAttribute( "title" ) )
            markupWindowTitle = e.attribute( "title" );
        if ( e.hasAttribute( "summary" ) )
            markupWindowSummary = e.attribute( "summary" );
        if ( e.hasAttribute( "creationDate" ) )
            markupCreationDate = QDateTime::fromString( e.attribute( "creationDate" ) );

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                markupWindowText = ee.firstChild().toCDATASection().data();
            }
            else if ( ee.tagName() == "irt" )
            {
                markupReply.ID = ee.attribute( "ID" ).toInt();
                markupReply.scope = (InReplyTo::S)ee.attribute( "scope" ).toInt();
                markupReply.stateModel = (InReplyTo::M)ee.attribute( "stateModel" ).toInt();
                markupReply.state = (InReplyTo::T)ee.attribute( "state" ).toInt();
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
    if ( markupWindowID != -1 )
        markupElement.setAttribute( "refID", markupWindowID );
    if ( !markupWindowTitle.isEmpty() )
        markupElement.setAttribute( "title", markupWindowTitle );
    if ( !markupWindowText.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        markupElement.appendChild( escapedText );
        QDomCDATASection cdataText = document.createCDATASection( markupWindowText );
        escapedText.appendChild( cdataText );
    }
    if ( !markupWindowSummary.isEmpty() )
        markupElement.setAttribute( "summary", markupWindowSummary );
    if ( markupCreationDate.isValid() )
        markupElement.setAttribute( "creationDate", markupCreationDate.toString() );
    if ( markupReply.ID != -1 )
    {
        QDomElement irtElement = document.createElement( "irt" );
        markupElement.appendChild( irtElement );
        irtElement.setAttribute( "ID", markupReply.ID );
        irtElement.setAttribute( "scope", (int)markupReply.scope );
        irtElement.setAttribute( "stateModel", (int)markupReply.stateModel );
        irtElement.setAttribute( "state", (int)markupReply.state );
    }
}


/** WindowAnnotation ** Annotation */

WindowAnnotation::WindowAnnotation()
    : Annotation(), windowMarkupParentID( -1 ), windowOpened( false )
{}

WindowAnnotation::WindowAnnotation( const QDomNode & node )
    : Annotation( node ), windowMarkupParentID( -1 ), windowOpened( false )
{
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
    }
}

void WindowAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
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
}


/** TextAnnotation ** MarkupAnnotation : Annotation */

TextAnnotation::TextAnnotation()
    : MarkupAnnotation(), textType( Linked ), textFont(),
    textOpened( false ), textIcon( "Note" ), inplaceAlign( 0 ),
    inplaceIntent( Unknown )
{}

TextAnnotation::TextAnnotation( const QDomNode & node )
    : MarkupAnnotation( node ), textType( Linked ), textFont(),
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
            textType = (TextAnnotation::T)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "font" ) )
            textFont.fromString( e.attribute( "font" ) );
        if ( e.hasAttribute( "opened" ) )
            textOpened = e.attribute( "opened" ).toInt();
        if ( e.hasAttribute( "icon" ) )
            textIcon = e.attribute( "icon" );
        if ( e.hasAttribute( "align" ) )
            inplaceAlign = e.attribute( "align" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            inplaceIntent = (TextAnnotation::I)e.attribute( "intent" ).toInt();

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                inplaceString = ee.firstChild().toCDATASection().data();
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
    MarkupAnnotation::store( node, document );

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
    if ( !inplaceString.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        textElement.appendChild( escapedText );
        QDomCDATASection cdataText = document.createCDATASection( inplaceString );
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


/** LineAnnotation ** MarkupAnnotation : Annotation */

LineAnnotation::LineAnnotation()
    : MarkupAnnotation(), lineStartStyle( None ), lineEndStyle( None ),
    lineClosed( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineShowCaption( false ), lineIntent( Unknown )
{}

LineAnnotation::LineAnnotation( const QDomNode & node )
    : MarkupAnnotation( node ), lineStartStyle( None ), lineEndStyle( None ),
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
            lineStartStyle = (LineAnnotation::S)e.attribute( "startStyle" ).toInt();
        if ( e.hasAttribute( "endStyle" ) )
            lineEndStyle = (LineAnnotation::S)e.attribute( "endStyle" ).toInt();
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
            lineIntent = (LineAnnotation::I)e.attribute( "intent" ).toInt();

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
    MarkupAnnotation::store( node, document );

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


/** GeomAnnotation ** MarkupAnnotation : Annotation */

GeomAnnotation::GeomAnnotation()
    : MarkupAnnotation(), geomType( InscribedSquare ), geomWidthPt( 18 )
{}

GeomAnnotation::GeomAnnotation( const QDomNode & node )
    : MarkupAnnotation( node ), geomType( InscribedSquare ), geomWidthPt( 18 )
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
            geomType = (GeomAnnotation::T)e.attribute( "type" ).toInt();
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
    MarkupAnnotation::store( node, document );

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


/** HighlightAnnotation ** MarkupAnnotation : Annotation */

HighlightAnnotation::HighlightAnnotation()
    : MarkupAnnotation(), highlightType( Highlight )
{}

HighlightAnnotation::HighlightAnnotation( const QDomNode & node )
    : MarkupAnnotation( node ), highlightType( Highlight )
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
            highlightType = (HighlightAnnotation::T)e.attribute( "type" ).toInt();

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
    MarkupAnnotation::store( node, document );

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


/** StampAnnotation ** MarkupAnnotation : Annotation */

StampAnnotation::StampAnnotation()
    : MarkupAnnotation(), stampIconName( "kpdf" )
{}

StampAnnotation::StampAnnotation( const QDomNode & node )
    : MarkupAnnotation( node ), stampIconName( "kpdf" )
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
    MarkupAnnotation::store( node, document );

    // create [stamp] element
    QDomElement stampElement = document.createElement( "stamp" );
    node.appendChild( stampElement );

    // append the optional attributes
    if ( !stampIconName.isEmpty() )
        stampElement.setAttribute( "icon", stampIconName );
}


/** InkAnnotation ** MarkupAnnotation : Annotation */

InkAnnotation::InkAnnotation()
    : MarkupAnnotation()
{}

InkAnnotation::InkAnnotation( const QDomNode & node )
    : MarkupAnnotation( node )
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
    MarkupAnnotation::store( node, document );

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
