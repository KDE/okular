/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_ANNOTATIONS_H_
#define _KPDF_ANNOTATIONS_H_

#include <qstring.h>
#include <qdatetime.h>
#include <qvaluelist.h>
#include <qdom.h>
#include <qfont.h>
#include "page.h"

class Annotation;

/**
 * @short Helper class for (recoursive) annotation retrieval/storage.
 *
 */
class AnnotationManager
{
    public:
        // restore an annotation (with revisions if needed) from a dom
        // element. returns a pointer to the complete annotation or 0 if
        // element is invalid.
        static Annotation * createAnnotation( const QDomElement & annElement );

        // save the 'ann' annotations as a child of parentElement taking
        // care of saving all revisions if 'ann' has any.
        static void storeAnnotation( const Annotation * ann, QDomNode & parentNode,
            QDomDocument & document );
};


/**
 * @short Annotation struct holds properties shared by all annotations.
 *
 * An Annotation is an object (text note, highlight, sound, popup window, ..)
 * contained by a KPDFPage in the document.
 *
 * For current state in relations to pdf embedded annotations:
 * @see generator_pdf/README.Annotations
 */
struct Annotation
{
    // enum definitions
    enum SubType { AText, ALine, AGeom, AHighlight, AStamp, AInk, A_BASE };
    enum Flag { Hidden = 1, FixedSize = 2, FixedRotation = 4, DenyPrint = 8,
                DenyWrite = 16, DenyDelete = 32, ToggleHidingOnMouse = 64, External = 128 };
    enum LineStyle { Solid, Dashed, Beveled, Inset, Underline };
    enum LineEffect { NoEffect, Cloudy };
    enum RevScope { Reply, Group, Delete };
    enum RevType { None,  Marked, Unmarked,  Accepted, Rejected, Cancelled, Completed };

    /** properties: contents related */
    QString         author;                 // ''
    QString         contents;               // ''
    QString         uniqueName;             // 'kpdf-#NUMBER#'
    QDateTime       modifyDate;             // before or equal to currentDateTime()
    QDateTime       creationDate;           // before or equal to modifyDate

    /** properties: look/interaction related */
    int             flags;                  // 0
    NormalizedRect  boundary;               // valid or isNull()
    struct Style
    {
        // appearance properties
        QColor          color;              // black
        double          opacity;            // 1.0
        // pen properties
        double          width;              // 1.0
        LineStyle       style;              // LineStyle::Solid
        double          xCorners;           // 0.0
        double          yCorners;           // 0.0
        int             marks;              // 3
        int             spaces;             // 0
        // pen effects
        LineEffect      effect;             // LineEffect::NoEffect
        double          effectIntensity;    // 1.0
    }               style;

    /** properties: popup window */
    struct Window
    {
        // window state (Hidden, FixedRotation, Deny* flags allowed)
        int             flags;              // -1 (never initialized) -> 0 (if inited and shown)
        // geometric properties
        NormalizedPoint topLeft;            // no default, inited to boundary.topLeft
        int             width;              // no default, fixed width inited to 200
        int             height;             // no default, fixed height inited to 150
        // window contens/override properties
        QString         title;              // '' text in the titlebar (overrides author)
        QString         summary;            // '' short description (displayed if not empty)
        QString         text;               // '' text for the window (overrides annot->contents)
    }               window;

    /** properties: versioning */
    struct Revision
    {
        // child revision
        Annotation *    annotation;         // not null
        // scope and type of revision
        RevScope        scope;              // Reply
        RevType         type;               // None
    };
    QValueList< Revision > revisions;       // empty by default

    // methods: query annotation's type for runtime type identification
    virtual SubType subType() const { return A_BASE; }
    //QRect geometry( int scaledWidth, int scaledHeight, KPDFPage * page );

    // methods: storage/retrieval from xml nodes
    Annotation( const QDomNode & node );
    virtual void store( QDomNode & parentNode, QDomDocument & document ) const;

    // methods: default constructor / virtual destructor
    Annotation();
    virtual ~Annotation() {};
};


//BEGIN  Annotations Definition
// a helper used to shorten the code presented below
#define AN_COMMONDECL( className, rttiType )\
    className();\
    className( const class QDomNode & node );\
    void store( QDomNode & parentNode, QDomDocument & document ) const;\
    SubType subType() const { return rttiType; }

struct TextAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( TextAnnotation, AText );

    // local enums
    enum TextType { Linked, InPlace };
    enum InplaceIntent { Unknown, Callout, TypeWriter };

    // data fields
    TextType        textType;
    QFont           textFont;
    bool            textOpened;
    QString         textIcon;
    int             inplaceAlign;   // 0:left, 1:center, 2:right
    QString         inplaceText;    // '' overrides contents
    NormalizedPoint inplaceCallout[3];
    InplaceIntent   inplaceIntent;
};

struct LineAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( LineAnnotation, ALine )

    // local enums
    enum TermStyle { Square, Circle, Diamond, OpenArrow, ClosedArrow, None,
                     Butt, ROpenArrow, RClosedArrow, Slash };
    enum LineIntent { Unknown, Arrow, Dimension, PolygonCloud };

    // data fields (note uses border for rendering style)
    QValueList<NormalizedPoint> linePoints;
    TermStyle       lineStartStyle;
    TermStyle       lineEndStyle;
    bool            lineClosed;     // false (if true connect first and last points)
    QColor          lineInnerColor;
    double          lineLeadingFwdPt;
    double          lineLeadingBackPt;
    bool            lineShowCaption;
    LineIntent      lineIntent;
};

struct GeomAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( GeomAnnotation, AGeom )

    // common enums
    enum GeomType { InscribedSquare, InscribedCircle };

    // data fields (note uses border for rendering style)
    GeomType        geomType;
    QColor          geomInnerColor;
    int             geomWidthPt;    // 18
};

struct HighlightAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( HighlightAnnotation, AHighlight )

    // local enums
    enum HighlightType { Highlight, Underline, Squiggly, StrikeOut };

    // data fields
    HighlightType   highlightType;
    QValueList<NormalizedPoint[4]>  highlightQuads;
};

struct StampAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( StampAnnotation, AStamp )

    // data fields
    QString         stampIconName;
};

struct InkAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( InkAnnotation, AInk )

    // data fields
    QValueList< QValueList<NormalizedPoint> > inkPaths;
};
//END  Annotations definition

#endif
