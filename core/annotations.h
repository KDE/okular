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
#include <qpoint.h>
#include <qvaluelist.h>
#include "page.h"

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
struct Annotation : public NormalizedRect
{
    // enum definitions
    enum Flags { Hidden, NoOpenable, Print, Locked, ReadOnly };
    enum State { Closed, Opened };

    // common annotation fields
    int pageNumber;
    State state;
    int flags;
    QString text;
    QString uniqueName;
    QDateTime creationDate;
    QDateTime modifyDate;
    QColor baseColor;

    // virtual function to identify annotation subclasses
    enum AType { AText, ALine, AGeom, APath, AHighlight, AStamp, AMedia };
    virtual AType aType() const = 0;

    // initialize default values
    Annotation();
    virtual ~Annotation();
};

/** Annotations in PDF 1.6 specification **
KDPF will try to support ALL annotations and ALL parameters for each one. If
this can't be done, we must support at least the most common ones and the
most common parameters. */
/*
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
   \ Type (name), W (number), S (name), D (array)
 (BE          dictionary      border effect (only for square, circle, and polygon))
 ( \ S (name (effect[S:no effect,C: cloudy])), I (number (intensity(0..2)))
  AP          dictionary      visual representation (handler should provide own impl)
   \ renderable data to be used with composition algorithm 8.1 (in pdf spec)
  AS          name            visual state (indexes an AP representation)
  Border      array           pre-BS: x-corner-rad, y-corner-rad, border width
  C           array           color (3 components in range [0..1])
  A           dictionary      action (NA on link, specialized on Movie)
  AA          dictionary      additional actions for events (used by Widget only)
  StructPar.  integer         annotation?s key in the structural tree(not used)
  OC          dictionary      optional content properties (custom visi-check)

MARKUP Annotations additional fields (see X entries in the Subtype table)
  T           text string     titlebar text (creator name by convention)
  Popup       dictionary      refrence to pupup annot for editing text
  CA          number          opacity (def: 1.0)
  RC          text string     rich text displayed if opened (overrides Contents)
  CreationD.  date            creation date
  IRT         dictionary      reference to the ann this is 'in reply to' (if RT)
  Subj        text string     short subject addressed
  RT          name            'R':in reply 'Group':grouped (if IRT)
  IT          name            annotation intent (specialized for certail types)
  [annotation states? pg 585]

Most other markup annotations have an associated pop-up window that may
contain text. The annotation?s Contents entry specifies the text to be displayed
when the pop-up window is opened. These include text, line, square, circle,
polygon, polyline, highlight, underline, squiggly-underline, strikeout, rubber
stamp, caret, ink, and file attachment annotations.

CUSTOM FIELDS (for each subType):
X: uses markup for

  Text            X     a 'sticky note' attached to a point in document
   \Open       boolean     default:false
    Name       name        icon[Comment,Key,Note,Help,NewParagraph,Paragraph,Insert]
    [M]State   text        if RT,IRT,StateModel are set
    [M]StateM. text        if RT,IRT,State are set

  Link            .     hypertext link to a location in document or action
   \Dest       linkDest    if A not present
    H          name        N(none) I(invert) O(outline) P(sunken)
    PA         dictionary  URI action
    QuadPoints array       array of quadrilaterals (8 x n numbers)

  FreeText        X     like Text but the text is always visible
   \*DA        string      appearance string (AP takes precedence)
    Q          integer     0(Left-justified) 1(Centered) 2(Right-justified)
    RC         text        rich text string (overrides Contents)
    DS         text        default text string
    CL         array       2 or 3 {x,y} couples for callout line
    [M]IT      name        not present,FreeTextCallout,FreeTextTypeWriter


  Line            X     a single straight line on the page (has popup note)
   \*L         array       4 numbers (2 x,y couples)
    BS         dictionary  width and dash pattern to be used in drawing the line
    LE         array       2 names (start and end styles) (def:None,None)
    \ values [Square,Circle,Diamond,OpenArrow,ClosedArrow,None,
      Butt,ROpenArrow,RClosedArrow,Slash]
    IC         array       interior color (3 components in range [0..1])
    LL         number      leader line fwd (if LLE) in points
    LLE        number      leader line bk (if LL) in points
    Cap        boolean     has caption (RC or Contents) (def:false)
    [M]IT      name        not present,LineArrow,LineDimension

  Square          X     rect or ellipse on the page (has popup note) the square
  Circle          X     or circle have 18pt border are inscribed into rect
   \BS         dictionary  line width and dash pattern
    IC         array       interior color (3 components in range [0..1])
    BE         dictionary  border effect
    RD         rectangle   negative border offsets (4 positive coords)

  Polygon         X     closed polygon on the page
  PolyLine        X     polygon without first and last vtx closed
   \*Vertices  array       n*{x,y} pairs of all line vertices
    LE         array       2 names (start and end styles) (def:None,None)
    BS         dictionary  width and dash pattern
    IC         array       interior color (3 components in range [0..1])
    BE         dictionary  border effect
    IT         name        not present,PolygonCloud

  Highlight       X
  Underline       X     appears as highlights, underlines, strikeouts. has
  Squiggly        X     popup text of associated note)
  StrikeOut       X
   \*QuadPo.   array       array of ccw quadrilats (8 x n numbers) (AP takes prec)

  Caret           X     visual symbol that indicates the presence of text
   \RD         rectangle   rect displacement from effective rect to annotation one
    Sy         name        'P':paragraph symbol, 'None':no symbol(defaulr)

  Stamp           X     displays text or graphics intended to look as rubber stamps
   \Name       name        [Approved,Experimental,NotApproved,AsIs,Expired,
        NotForPublicRelease,Confidential,Final,Sold,Departmental,
        ForComment,TopSecret,Draft,ForPublicRelease]

  Ink             X     freehand ?scribble? composed of one or more disjoint paths
   \InkList    array       array or arrays of {x,y} userspace couples
    BS         dictionary  line width and dash pattern


  Popup           .     no gfx only a parent (inherits Contents,M,C,T)
   \Parent     dictionary  indirect reference to parent (from wich Mark. are inh)
    Open       boolean     initially displayed opened (def:false)

  FileAttachment  X     reference to file (typically embedded)
   \*FS        file        file associated
    Name       name        icon [Graph,PushPin,Paperclip,Tag]

  Sound           X     like Text but contains sound
   \*Sound     stream      sound to be played when annot is activated
    Name       name        icon [Speaker,Mic,_more_]

  Movie           .     contains animated graphics and sound
   \Movie      dictionary  the movie to be played when annot is actived
    A          boolean     whether and how to play the movie (def:true)

NOT USED BY KPDF:
  Screen          .     specifies a region of a page on which play media clips
   \MK         dictionary  appearance characteristics dictionary
   (AP is the screen appearance, AA (addit actions)controls, P mandatory for rend)
  Widget          .     appearance of the fields for user interaction
   \H          name        highlighting N:none,I:invert,O:outline,T|P:sunken
    MK         dictionary  appearance characteristics dictionary (see table 8.36)
  PrinterMark     .     a graphic symbol used to assist production personnel
  TrapNet         .     add color marks along colour boundaries to avoid artifacts
  Watermark       .     graphics to be printed at a fixed size and position on a page
  3D              .     the mean by which 3D artwork is represented in a document
*/

struct TextAnnotation : public Annotation
{
    // enum definitions
    enum TextType { InPlace, Popup };

    TextType subType;   // InPlace

      TextAnnotation();
      AType aType() const { return AText; }
};

struct LineAnnotation : public Annotation
{
    NormalizedPoint point[2];
    bool srcArrow;  // false
    bool dstArrow;  // false
    int width;      // 2

      LineAnnotation();
      AType aType() const { return ALine; }
};

struct GeomAnnotation : public Annotation
{
    // enum definitions
    enum GeomType { Square, Circle, Polygon };

    GeomType subType;   // Square

      GeomAnnotation();
      AType aType() const { return AGeom; }
};

struct PathAnnotation : public Annotation
{
    // enum definitions
    enum PathType { Ink, PolyLine };

    PathType subType;   // Ink

      PathAnnotation();
      AType aType() const { return APath; }
};

struct HighlightAnnotation : public Annotation
{
    // enum definitions
    enum HighlightType { Highlight, Underline, Squiggly, StrikeOut, BLOCK };

    HighlightType subType;  // Highlight

      HighlightAnnotation();
      AType aType() const { return AHighlight; }
};

struct StampAnnotation : public Annotation
{
    QString stampIcon;  // ""

      StampAnnotation();
      AType aType() const { return AStamp; }
};

struct MediaAnnotation : public Annotation
{
    // enum definitions
    enum MediaType { URL, FileAttachment, Sound, Movie };

    MediaType subType;  //URL

      MediaAnnotation();
      AType aType() const { return AMedia; }
};

#endif
