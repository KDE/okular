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
#include <qfont.h>
#include <qdom.h>
#include "page.h"

/** Annotations in PDF 1.6 specification **

KDPF tries to support ALL annotations and ALL parameters for each one. If
this can't be done, we must support at least the most common ones and the
most common parameters.

Current data structure status:
- COMPLETE: [markup], popup, text, freetext, line, polygon, polyline, highlight,
    underline, squiggly, strikeout, stamp, ink
- PARTIAL: base{P,AP,AS,A,AA,StructPar,OC}, geom{RD}
- MISSING: link, caret, fileattachment, sound, movie, screen, widget,
    printermark, trapnet, watermark, 3d *//*

COMMON FIELDS (note: '*' is required)
  Type        name            'Annot'
  *Subtype    name            [Text...3D]
   \ see second table (below)
  *Rect       rectangle       norma rect
  Contents    text string     alternate text / description
  P           dictionary      page reference (mandatory for Screen)
  NM          text string     unique name
  M           date or string  last modify date
  F           integer         flags (default:0)
   \ OR-ed flags: Invisible(0), Hidden(1), Print(2), NoZoom(3), NoRotate(4),
    \NoView(5), ReadOnly(6), Locked(7), ToggleNoView(8)
  BS          dictionary      border styles (also style for: Line,Square,Circle,Ink)
   \ Type (name='Border'), W (number), S (name), D (array)
  BE          dictionary      border effect (only for square, circle, and polygon)
   \ S (name (effect[S:no effect,C: cloudy])), I (number (intensity(0..2))
  AP          dictionary      visual representation (handler should provide own impl)
   \ renderable data to be used with composition algorithm 8.1 (in pdf spec)
  AS          name            visual state (indexes an AP representation)
  Border      array           pre-BS: x-corner-rad, y-corner-rad, width [,dash array]
  C           array           color (3 components in range [0..1])
  A           dictionary      action (NA on link, specialized on Movie)
   \ 'LinkAction' to extract those and use internal handler instead of external one
  AA          dictionary      additional actions for events (used by Widget only)
  StructPar.  integer         annotation's key in the structural tree(not used)
  OC          dictionary      optional content properties (custom visi-check)

MARKUP +additional+ fields (see 'X' marks in the Subtype table):
 Markup annotations have an associated pop-up window that may contain text.
 If the popup id is given, that popup is used for displaying text, otherwise a
 popup is created on annotation opening but the popup is rendered 'in place'
 with the annotation and can not be moved without moving the annotation.
  T           text string     titlebar text (creator name by convention)
  Popup       dictionary      indirect refrence to pupup annot for editing text
  CA          number          opacity (def: 1.0)
  RC          text string/eam rich text displayed if opened (overrides Contents)
  CreationD.  date            creation date
  IRT         dictionary      reference to the ann this is 'in reply to' (if RT)
  Subj        text string     short subject addressed
  RT          name            'R':in reply 'Group':grouped (if IRT)
  IT          name            annotation intent (specialized for certail types)
  [annotation states? pg 585]

CUSTOM FIELDS (for each subType) (X: is markup):
  Popup           .     no gfx only a parent (inherits Contents,M,C,T)
    Parent     dictionary    indirect reference to parent (from wich Mark. are inh)
    Open       boolean       initially displayed opened (def:false)

  Text            X     a 'sticky note' attached to a point in document
    Open       boolean       default:false
    Name       name          icon[Comment,Key,Note,Help,NewParagraph,Paragraph,Insert]
    [M]State   text string   if RT,IRT,StateModel are set
    [M]StateM. text string   if RT,IRT,State are set

  FreeText        X     like Text but the text is always visible
    *DA        string        appearance string (AP takes precedence)
    Q          integer       0(Left-justified) 1(Centered) 2(Right-justified)
    RC         text string/eam rich text string (overrides Contents)
    DS         text string   default text string
    CL         array         2 or 3 {x,y} couples for callout line
    [M]IT      name          not present,FreeTextCallout,FreeTextTypeWriter

  Line            X     a single straight line on the page (has popup note)
    *L         array         4 numbers (2 x,y couples)
    BS         dictionary    width and dash pattern to be used in drawing the line
    LE         array         2 names (start and end styles) (def:None,None)
    \ values [Square,Circle,Diamond,OpenArrow,ClosedArrow,None,
      Butt,ROpenArrow,RClosedArrow,Slash]
    IC         array         interior color (3 components in range [0..1])
    LL         number        leader line fwd (if LLE) in points
    LLE        number        leader line bk (if LL) in points
    Cap        boolean       has caption (RC or Contents) (def:false)
    [M]IT      name          not present,LineArrow,LineDimension

  Polygon         X     closed polygon on the page
  PolyLine        X     polygon without first and last vtx closed
    *Vertices  array         n*{x,y} pairs of all line vertices
    LE         array         2 names (start and end styles) (def:None,None)
    BS         dictionary    width and dash pattern
    IC         array         interior color (3 components in range [0..1])
    BE         dictionary    border effect
    IT         name          not present,PolygonCloud

  Square          X     rect or ellipse on the page (has popup note) the square
  Circle          X     or circle have 18pt border are inscribed into rect
    BS         dictionary    line width and dash pattern
    IC         array         interior color (3 components in range [0..1])
    BE         dictionary    border effect
    RD         rectangle     negative border offsets (4 positive coords)

  Highlight       X
  Underline       X     appears as highlights, underlines, strikeouts. has
  Squiggly        X     popup text of associated note)
  StrikeOut       X
    *QuadPo.   array         array of ccw quadrilats (8 x n numbers) (AP takes prec)

  Caret           X     visual symbol that indicates the presence of text
    RD         rectangle     rect displacement from effective rect to annotation one
    Sy         name          'P':paragraph symbol, 'None':no symbol(defaulr)

  Stamp           X     displays text or graphics intended to look as rubber stamps
    Name       name          [Approved,Experimental,NotApproved,AsIs,Expired,
        NotForPublicRelease,Confidential,Final,Sold,Departmental,
        ForComment,TopSecret,Draft,ForPublicRelease]

  Ink             X     freehand ?scribble? composed of one or more disjoint paths
   *InkList    array         array or arrays of {x,y} userspace couples
    BS         dictionary    line width and dash pattern

Unused / Incomplete :
  Link            .     hypertext link to a location in document or action
    Dest       arr,nam,str   if A not present
    H          name          N(none) I(invert) O(outline) P(sunken)
    PA         dictionary    URI action
    QuadPoints array         array of quadrilaterals (8 x n numbers)

  FileAttachment  X     reference to file (typically embedded)
    *FS        file          file associated
    Name       name          icon [Graph,PushPin,Paperclip,Tag]

  Sound           X     like Text but contains sound
    *Sound     stream        sound to be played when annot is activated
    Name       name          icon [Speaker,Mic,_more_]

  Movie           .     contains animated graphics and sound
    Movie      dictionary    the movie to be played when annot is actived
    A          boolean       whether and how to play the movie (def:true)

  Screen          .     specifies a region of a page on which play media clips
  Widget          .     appearance of the fields for user interaction
  PrinterMark     .     a graphic symbol used to assist production personnel
  TrapNet         .     add color marks along colour boundaries to avoid artifacts
  Watermark       .     graphics to be printed at a fixed size and position on a page
  3D              .     the mean by which 3D artwork is represented in a document
*/

/**
 * @short Base options for an annotation (highlights, stamps, boxes, ..).
 *
 * From PDFreferece v.1.6:
 *  An annotation associates an object such as a note, sound, or movie with a
 *  location on a page of a PDF document ...
 *
 * Inherited classes must modify protected variables as appropriate.
 * Other fields in pdf reference we dropped here:
 *   -subtype, rectangle(we are a rect), border stuff
 */
struct Annotation
{
    // enum definitions
    enum SubType { APopup, AText, ALine, AGeom, AHighlight, AStamp, AInk };
    enum Flags { Hidden, NoOpenable, Print, Locked, ReadOnly };

    // struct definitions
    struct Border
    {
        double width;
        double xCornerRadius;
        double yCornerRadius;
        enum { Solid, Dashed, Beveled, Inset, Underline } style;
        int dashMarks;
        int dashSpaces;
        enum { NoEffect, Cloudy } effect;
        int effectIntensity;
        // initialize default valudes
        Border();
    };

    // data fields
    virtual SubType subType() const = 0;            // for RTTI
    virtual bool isMarkup() const { return false; } // for RTTI
    NormalizedRect  r;
    QString         contents;
    QString         uniqueName;
    QDateTime       modifyDate;
    int             flags;
    Border          border;
    QColor          color;

    // initialize default values
    Annotation();
    virtual ~Annotation() {};
    // read values from XML node
    Annotation( const QDomElement & node );
    // store custom config to XML node
    virtual void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct MarkupAnnotation : public Annotation
{
    // struct definitions
    struct InReplyTo
    {
        int ID;
        enum { Reply, Grouped } scope;
        enum { Mark, Review } stateModel;
        enum { Marked, Unmarked, Accepted, Rejected, Cancelled, Completed, None } state;
        // initialize default valudes
        InReplyTo();
    };

    // data fields
    bool isMarkup() const { return true; }          // for RTTI
    QString         markupCreator;
    double          markupOpacity;
    int             markupPopupID;
    QString         markupPopupText;
    QString         markupSubject;
    QDateTime       markupCreationDate;
    InReplyTo       markupReply;

    // initialize default values
    MarkupAnnotation();
    virtual ~MarkupAnnotation() {};
    // read values from XML node
    MarkupAnnotation( const QDomElement & node );
    // store custom config to XML node
    virtual void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct PopupAnnotation : public Annotation
{
    // data fields (takes {Contents,M,C,T,..} from related parent)
    int             popupMarkupParentID;
    bool            popupOpened;

    // RTTI
    SubType subType() const { return APopup; }

    // common functions
    PopupAnnotation();
    PopupAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct TextAnnotation : public MarkupAnnotation
{
    // data fields
    enum { Popup, InPlace } textType;
    QFont       textFont;
    bool        popupOpened;
    QString     popupIcon;
    int         inplaceAlign; // 0:left, 1:center, 2:right
    QString     inplaceRichString;
    QString     inplacePlainString;
    NormalizedPoint inplaceCallout[3];
    enum { Unknown, Callout, TypeWriter } inplaceIntent;

    // RTTI
    SubType subType() const { return AText; }

    // common functions
    TextAnnotation();
    TextAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct LineAnnotation : public MarkupAnnotation
{
    // data fields (note uses border for rendering style)
    QValueList<NormalizedPoint> linePoints;
    enum { Square, Circle, Diamond, OpenArrow, ClosedArrow, None, Butt,
           ROpenArrow, RClosedArrow, Slash } lineStartStyle, lineEndStyle;
    bool        lineClosed; // def:false (if true connect first and last points)
    QColor      lineInnerColor;
    double      lineLeadingFwdPt;
    double      lineLeadingBackPt;
    bool        lineShowCaption;
    enum { Unknown, Arrow, Dimension, PolygonCloud } lineIntent;

    // RTTI
    SubType subType() const { return ALine; }

    // common functions
    LineAnnotation();
    LineAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct GeomAnnotation : public MarkupAnnotation
{
    // data fields (note uses border for rendering style)
    enum { InscribedSquare, InscribedCircle } geomType;
    QColor      geomInnerColor;
    int         geomWidthPt;    //def:18

    // RTTI
    SubType subType() const { return AGeom; }

    // common functions
    GeomAnnotation();
    GeomAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct HighlightAnnotation : public MarkupAnnotation
{
    // data fields
    enum { Highlight, Underline, Squiggly, StrikeOut } highlightType;
    QValueList<NormalizedPoint[4]>  highlightQuads;

    // RTTI
    SubType subType() const { return AHighlight; }

    // common functions
    HighlightAnnotation();
    HighlightAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct StampAnnotation : public MarkupAnnotation
{
    // data fields
    QString     stampIconName;

    // RTTI
    SubType subType() const { return AStamp; }

    // common functions
    StampAnnotation();
    StampAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

struct InkAnnotation : public MarkupAnnotation
{
    // data fields (note uses border for rendering style)
    QValueList< QValueList<NormalizedPoint> > inkPaths;

    // RTTI
    SubType subType() const { return AInk; }

    // common functions
    InkAnnotation();
    InkAnnotation( const QDomElement & node );
    void store( QDomNode & parentNode, QDomDocument & document ) const;
};

#endif
