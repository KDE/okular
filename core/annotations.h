/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_ANNOTATIONS_H_
#define _OKULAR_ANNOTATIONS_H_

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QLinkedList>
#include <QtCore/QRect>
#include <QtGui/QFont>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include "page.h"

namespace Okular {

class Annotation;

/**
 * @short Helper class for (recoursive) annotation retrieval/storage.
 *
 */
class AnnotationUtils
{
    public:
        // restore an annotation (with revisions if needed) from a dom
        // element. returns a pointer to the complete annotation or 0 if
        // element is invalid.
        static Annotation * createAnnotation( const QDomElement & annElement );

        // save the 'ann' annotations as a child of parentElement taking
        // care of saving all revisions if 'ann' has any.
        static void storeAnnotation( const Annotation * ann,
            QDomElement & annElement, QDomDocument & document );

        // return an element called 'name' from the direct children of
        // parentNode or a null element if not found
        static QDomElement findChildElement( const QDomNode & parentNode,
            const QString & name );

        // return the translated string with the type of the annotation
        // specified
        static QString captionForAnnotation( Annotation * ann );

        static QRect annotationGeometry( const Annotation * ann,
            double scaledWidth, double scaledHeight );
};


/**
 * @short Annotation struct holds properties shared by all annotations.
 *
 * An Annotation is an object (text note, highlight, sound, popup window, ..)
 * contained by a Page in the document.
 *
 * For current state in relations to pdf embedded annotations:
 * @see generator_pdf/README.Annotations
 */
class OKULAR_EXPORT Annotation
{
    public:
        /**
         * Describes the type of annotation as defined in PDF standard.
         */
        enum SubType
        {
            AText = 1,      ///< A textual annotation
            ALine = 2,      ///< A line annotation
            AGeom = 3,      ///< A geometrical annotation
            AHighlight = 4, ///< A highlight annotation
            AStamp = 5,     ///< A stamp annotation
            AInk = 6,       ///< An ink annotation
            A_BASE = 0      ///< No annotation
        };

        /**
         * Describes additional properties of an annotation.
         */
        enum Flag
        {
            Hidden = 1,               ///< Is not shown in the document
            FixedSize = 2,            ///< Has a fixed size
            FixedRotation = 4,        ///< Has a fixed rotation
            DenyPrint = 8,            ///< Cannot be printed
            DenyWrite = 16,           ///< Cannot be changed
            DenyDelete = 32,          ///< Cannot be deleted
            ToggleHidingOnMouse = 64, ///< Can be hidden/shown by mouse click
            External = 128            ///< Is stored external
        };

        /**
         * Describes possible line styles for @see ALine annotation.
         */
        enum LineStyle
        {
            Solid = 1,      ///< A solid line
            Dashed = 2,     ///< A dashed line
            Beveled = 4,    ///< A beveled line
            Inset = 8,      ///< A inseted line
            Underline = 16  ///< An underline
        };

        /**
         * Describes possible line effects for @see ALine annotation.
         */
        enum LineEffect
        {
            NoEffect = 1,  ///< No effect
            Cloudy = 2     ///< The cloudy effect
        };

        /**
         * Describes the scope of revision information.
         */
        enum RevisionScope
        {
            Reply = 1,  ///< Belongs to a reply
            Group = 2,  ///< Belongs to a group
            Delete = 4  ///< Belongs to a deleted paragraph
        };

        /**
         * Describes the type of revision information.
         */
        enum RevisionType
        {
            None = 1,        ///< Not specified
            Marked = 2,      ///< Is marked
            Unmarked = 4,    ///< Is unmarked
            Accepted = 8,    ///< Has been accepted
            Rejected = 16,   ///< Was rejected
            Cancelled = 32,  ///< Has been cancelled
            Completed = 64   ///< Has been completed
        };

        /**
         * Creates a new annotation.
         */
        Annotation();

        /**
         * Creates a new annotation from the xml @p description
         */
        Annotation( const QDomNode &description );

        /**
         * Destroys the annotation.
         */
        virtual ~Annotation();

        /**
         * Sets the @p author of the annotation.
         */
        void setAuthor( const QString &author );

        /**
         * Returns the author of the annotation.
         */
        QString author() const;

        /**
         * Sets the @p contents of the annotation.
         */
        void setContents( const QString &contents );

        /**
         * Returns the contents of the annotation.
         */
        QString contents() const;

        /**
         * Sets the unique @name of the annotation.
         *
         * The unique name should have the form 'okular-#NUMBER#'
         */
        void setUniqueName( const QString &name );

        /**
         * Returns the unique name of the annotation.
         */
        QString uniqueName() const;

        /**
         * Sets the last modification @p date of the annotation.
         *
         * The date must be before or equal to QDateTime::currentDateTime()
         */
        void setModificationDate( const QDateTime &date );

        /**
         * Returns the last modification date of the annotation.
         */
        QDateTime modificationDate() const;

        /**
         * Sets the creation @p date of the annotation.
         *
         * The date must be before or equal to @see modificationDate()
         */
        void setCreationDate( const QDateTime &date );

        /**
         * Returns the creation date of the annotation.
         */
        QDateTime creationDate() const;

        /**
         * Sets the @p flags of the annotation.
         * @see Flags
         */
        void setFlags( int flags );

        /**
         * Returns the flags of the annotation.
         * @see Flags
         */
        int flags() const;

        /**
         * Sets the bounding @p rectangle of the annotation.
         */
        void setBoundingRectangle( const NormalizedRect &rectangle );

        /**
         * Returns the bounding rectangle of the annotation.
         */
        NormalizedRect boundingRectangle() const;

        /**
         * Returns the transformed bounding rectangle of the annotation.
         *
         * This rectangle must be used when showing annotations on screen
         * to have them rotated correctly.
         */
        NormalizedRect transformedBoundingRectangle() const;

        /**
         * The Style class contains all information about style of the
         * annotation.
         */
        class Style
        {
            public:
                Style();

                void setColor( const QColor &color );
                QColor color() const;

                void setOpacity( double opacity );
                double opacity() const;

                void setWidth( double width );
                double width() const;

                void setLineStyle( LineStyle style );
                LineStyle lineStyle() const;

                void setXCorners( double xCorners );
                double xCorners() const;

                void setYCorners( double yCorners );
                double yCorners() const;

                void setMarks( int marks );
                int marks() const;

                void setSpaces( int spaces );
                int spaces() const;

                void setLineEffect( LineEffect effect );
                LineEffect lineEffect() const;

                void setEffectIntensity( double intensity );
                double effectIntensity() const;

            private:
                // appearance properties
                QColor          m_color;              // black
                double          m_opacity;            // 1.0
                // pen properties
                double          m_width;              // 1.0
                LineStyle       m_style;              // LineStyle::Solid
                double          m_xCorners;           // 0.0
                double          m_yCorners;           // 0.0
                int             m_marks;              // 3
                int             m_spaces;             // 0
                // pen effects
                LineEffect      m_effect;             // LineEffect::NoEffect
                double          m_effectIntensity;    // 1.0
        };

        /**
         * Returns a reference to the style object of the annotation.
         */
        Style & style();

        /**
         * Returns a const reference to the style object of the annotation.
         */
        const Style & style() const;

        /**
         * The Window class contains all information about the popup window
         * of the annotation that is used to edit the content and properties.
         */
        class Window
        {
            public:
                Window();

                void setFlags( int flags );
                int flags() const;

                void setTopLeft( const NormalizedPoint &point );
                NormalizedPoint topLeft() const;

                void setWidth( int width );
                int width() const;

                void setHeight( int height );
                int height() const;

                void setTitle( const QString &title );
                QString title() const;

                void setSummary( const QString &summary );
                QString summary() const;

                void setText( const QString &text );
                QString text() const;

            private:
                // window state (Hidden, FixedRotation, Deny* flags allowed)
                int             m_flags;              // -1 (never initialized) -> 0 (if inited and shown)
                // geometric properties
                NormalizedPoint m_topLeft;            // no default, inited to boundary.topLeft
                int             m_width;              // no default, fixed width inited to 200
                int             m_height;             // no default, fixed height inited to 150
                // window contens/override properties
                QString         m_title;              // '' text in the titlebar (overrides author)
                QString         m_summary;            // '' short description (displayed if not empty)
                QString         m_text;               // '' text for the window (overrides annot->contents)
        };

        /**
         * Returns a reference to the window object of the annotation.
         */
        Window & window();

        /**
         * Returns a const reference to the window object of the annotation.
         */
        const Window & window() const;

        /**
         * The Revision class contains all information about the revision
         * of the annotation.
         */
        class Revision
        {
            public:
                Revision();

                void setAnnotation( Annotation *annotation );
                Annotation *annotation() const;

                void setScope( RevisionScope scope );
                RevisionScope scope() const;

                void setType( RevisionType type );
                RevisionType type() const;

            private:
                // child revision
                Annotation *    m_annotation;         // not null
                // scope and type of revision
                RevisionScope       m_scope;              // Reply
                RevisionType        m_type;
        };

        /**
         * Returns a reference to the revision list of the annotation.
         */
        QLinkedList< Revision > & revisions();

        /**
         * Returns a reference to the revision list of the annotation.
         */
        const QLinkedList< Revision > & revisions() const;

        /**
         * Returns the sub type of the annotation.
         */
        virtual SubType subType() const;

        /**
         * Stores the annotation as xml in @p document under the given parent @p node.
         */
        virtual void store( QDomNode & node, QDomDocument & document ) const;

        /**
         * Transforms the annotation coordinates with the transformation defined
         * by @p matrix.
         */
        virtual void transform( const QMatrix &matrix );

    private:
        /** properties: contents related */
        QString         m_author;                 // ''
        QString         m_contents;               // ''
        QString         m_uniqueName;             // 'okular-#NUMBER#'
        QDateTime       m_modifyDate;             // before or equal to currentDateTime()
        QDateTime       m_creationDate;           // before or equal to modifyDate

        int             m_flags;                  // 0
        NormalizedRect  m_boundary;               // valid or isNull()
        NormalizedRect  m_transformedBoundary;

        Style           m_style;
        Window          m_window;
        QLinkedList< Revision > m_revisions;
};


// a helper used to shorten the code presented below
#define AN_COMMONDECL( className, rttiType )\
    className();\
    className( const class QDomNode & node );\
    void store( QDomNode & parentNode, QDomDocument & document ) const;\
    SubType subType() const { return rttiType; }

struct OKULAR_EXPORT TextAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( TextAnnotation, AText );

    // local enums
    enum TextType { Linked, InPlace };
    enum InplaceIntent { Unknown, Callout, TypeWriter };

    // data fields
    TextType        textType;               // Linked
    QString         textIcon;               // 'Comment'
    QFont           textFont;               // app def font
    int             inplaceAlign;           // 0:left, 1:center, 2:right
    QString         inplaceText;            // '' overrides contents
    NormalizedPoint inplaceCallout[3];      //
    NormalizedPoint transformedInplaceCallout[3];
    InplaceIntent   inplaceIntent;          // Unknown

    virtual void transform( const QMatrix &matrix );
};

struct OKULAR_EXPORT LineAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( LineAnnotation, ALine )

    // local enums
    enum TermStyle { Square, Circle, Diamond, OpenArrow, ClosedArrow, None,
                     Butt, ROpenArrow, RClosedArrow, Slash };
    enum LineIntent { Unknown, Arrow, Dimension, PolygonCloud };

    // data fields (note uses border for rendering style)
    QLinkedList<NormalizedPoint> linePoints;
    QLinkedList<NormalizedPoint> transformedLinePoints;
    TermStyle       lineStartStyle;         // None
    TermStyle       lineEndStyle;           // None
    bool            lineClosed;             // false (if true draw close shape)
    QColor          lineInnerColor;         //
    double          lineLeadingFwdPt;       // 0.0
    double          lineLeadingBackPt;      // 0.0
    bool            lineShowCaption;        // false
    LineIntent      lineIntent;             // Unknown

    virtual void transform( const QMatrix &matrix );
};

struct OKULAR_EXPORT GeomAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( GeomAnnotation, AGeom )

    // common enums
    enum GeomType { InscribedSquare, InscribedCircle };

    // data fields (note uses border for rendering style)
    GeomType        geomType;               // InscribedSquare
    QColor          geomInnerColor;         //
    int             geomWidthPt;            // 18
};

struct OKULAR_EXPORT HighlightAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( HighlightAnnotation, AHighlight )

    // local enums
    enum HighlightType { Highlight, Squiggly, Underline, StrikeOut };

    // data fields
    HighlightType   highlightType;          // Highlight
    struct Quad
    {
        NormalizedPoint points[4];          // 8 valid coords
        NormalizedPoint transformedPoints[4];
        bool            capStart;           // false (vtx 1-4) [K]
        bool            capEnd;             // false (vtx 2-3) [K]
        double          feather;            // 0.1 (in range 0..1) [K]
    };
    QList< Quad >  highlightQuads;     // not empty

    virtual void transform( const QMatrix &matrix );
};

struct OKULAR_EXPORT StampAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( StampAnnotation, AStamp )

    // data fields
    QString         stampIconName;          // 'Draft'
};

struct OKULAR_EXPORT InkAnnotation : public Annotation
{
    // common stuff for Annotation derived classes
    AN_COMMONDECL( InkAnnotation, AInk )

    // data fields
    QList< QLinkedList<NormalizedPoint> > inkPaths;
    QList< QLinkedList<NormalizedPoint> > transformedInkPaths;

    virtual void transform( const QMatrix &matrix );
};

}

#endif
