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

#include <okular/core/okular_export.h>
#include <okular/core/area.h>

namespace Okular {

class Annotation;
class AnnotationObjectRect;
class AnnotationPrivate;
class Document;
class EmbeddedFile;
class Page;
class PagePrivate;
class Sound;
class Movie;
class TextAnnotationPrivate;
class LineAnnotationPrivate;
class GeomAnnotationPrivate;
class HighlightAnnotationPrivate;
class StampAnnotationPrivate;
class InkAnnotationPrivate;
class CaretAnnotationPrivate;
class FileAttachmentAnnotationPrivate;
class SoundAnnotationPrivate;
class MovieAnnotationPrivate;

/**
 * @short Helper class for (recursive) annotation retrieval/storage.
 */
class OKULAR_EXPORT AnnotationUtils
{
    public:
        /**
         * Restore an annotation (with revisions if needed) from the dom @p element.
         *
         * Returns a pointer to the complete annotation or 0 if element is invalid.
         */
        static Annotation * createAnnotation( const QDomElement & element );

        /**
         * Saves the @p annotation as a child of @p element taking
         * care of saving all revisions if it has any.
         */
        static void storeAnnotation( const Annotation * annotation,
                                     QDomElement & element, QDomDocument & document );

        /**
         * Returns the child element with the given @p name from the direct
         * children of @p parentNode or a null element if not found.
         */
        static QDomElement findChildElement( const QDomNode & parentNode,
                                             const QString & name );

        /**
         * Returns the geometry of the given @p annotation scaled by
         * @p scaleX and @p scaleY.
         */
        static QRect annotationGeometry( const Annotation * annotation,
                                         double scaleX, double scaleY );
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
    /// @cond PRIVATE
    friend class AnnotationObjectRect;
    friend class Document;
    friend class Page;
    friend class PagePrivate;
    /// @endcond

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
            ACaret = 8,     ///< A caret annotation
            AFileAttachment = 9, ///< A file attachment annotation
            ASound = 10,    ///< A sound annotation
            AMovie = 11,    ///< A movie annotation
            A_BASE = 0      ///< The annotation base class
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
         * A function to be called when the annotation is destroyed.
         *
         * @warning the function must *not* call any virtual function,
         *          nor subcast.
         *
         * @since 0.7 (KDE 4.1)
         */
        typedef void ( * DisposeDataFunction )( const Okular::Annotation * );

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
         * Sets the unique @p name of the annotation.
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
         * @see @ref Flag
         */
        void setFlags( int flags );

        /**
         * Returns the flags of the annotation.
         * @see @ref Flag
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
         * Move the annotation by the specified coordinates.
         *
         * @see canBeMoved()
         */
        void translate( const NormalizedPoint &coord );

        /**
         * The Style class contains all information about style of the
         * annotation.
         */
        class OKULAR_EXPORT Style
        {
            public:
                /**
                 * Creates a new style.
                 */
                Style();

                /**
                 * Destroys the style.
                 */
                ~Style();

                Style( const Style &other );
                Style& operator=( const Style &other );

                /**
                 * Sets the @p color of the style.
                 */
                void setColor( const QColor &color );

                /**
                 * Returns the color of the style.
                 */
                QColor color() const;

                /**
                 * Sets the @p opacity of the style.
                 */
                void setOpacity( double opacity );

                /**
                 * Returns the opacity of the style.
                 */
                double opacity() const;

                /**
                 * Sets the @p width of the style.
                 */
                void setWidth( double width );

                /**
                 * Returns the width of the style.
                 */
                double width() const;

                /**
                 * Sets the line @p style of the style.
                 */
                void setLineStyle( LineStyle style );

                /**
                 * Returns the line style of the style.
                 */
                LineStyle lineStyle() const;

                /**
                 * Sets the x-corners of the style.
                 */
                void setXCorners( double xCorners );

                /**
                 * Returns the x-corners of the style.
                 */
                double xCorners() const;

                /**
                 * Sets the y-corners of the style.
                 */
                void setYCorners( double yCorners );

                /**
                 * Returns the y-corners of the style.
                 */
                double yCorners() const;

                /**
                 * Sets the @p marks of the style.
                 */
                void setMarks( int marks );

                /**
                 * Returns the marks of the style.
                 */
                int marks() const;

                /**
                 * Sets the @p spaces of the style.
                 */
                void setSpaces( int spaces );

                /**
                 * Returns the spaces of the style.
                 */
                int spaces() const;

                /**
                 * Sets the line @p effect of the style.
                 */
                void setLineEffect( LineEffect effect );

                /**
                 * Returns the line effect of the style.
                 */
                LineEffect lineEffect() const;

                /**
                 * Sets the effect @p intensity of the style.
                 */
                void setEffectIntensity( double intensity );

                /**
                 * Returns the effect intensity of the style.
                 */
                double effectIntensity() const;

            private:
                class Private;
                Private* const d;
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
        class OKULAR_EXPORT Window
        {
            public:
                /**
                 * Creates a new window.
                 */
                Window();

                /**
                 * Destroys the window.
                 */
                ~Window();

                Window( const Window &other );
                Window& operator=( const Window &other );

                /**
                 * Sets the @p flags of the window.
                 */
                void setFlags( int flags );

                /**
                 * Returns the flags of the window.
                 */
                int flags() const;

                /**
                 * Sets the top-left @p point of the window.
                 */
                void setTopLeft( const NormalizedPoint &point );

                /**
                 * Returns the top-left point of the window.
                 */
                NormalizedPoint topLeft() const;

                /**
                 * Sets the @p width of the window.
                 */
                void setWidth( int width );

                /**
                 * Returns the width of the window.
                 */
                int width() const;

                /**
                 * Sets the @p height of the window.
                 */
                void setHeight( int height );

                /**
                 * Returns the height of the window.
                 */
                int height() const;

                /**
                 * Sets the @p title of the window.
                 */
                void setTitle( const QString &title );

                /**
                 * Returns the title of the window.
                 */
                QString title() const;

                /**
                 * Sets the @p summary of the window.
                 */
                void setSummary( const QString &summary );

                /**
                 * Returns the summary of the window.
                 */
                QString summary() const;

                /**
                 * Sets the @p text of the window.
                 */
                void setText( const QString &text );

                /**
                 * Returns the text of the window.
                 */
                QString text() const;

            private:
                class Private;
                Private* const d;
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
                /**
                 * Creates a new revision.
                 */
                Revision();

                /**
                 * Destroys the revision.
                 */
                ~Revision();

                Revision( const Revision &other );
                Revision& operator=( const Revision &other );

                /**
                 * Sets the @p annotation the revision belongs to.
                 */
                void setAnnotation( Annotation *annotation );

                /**
                 * Returns the annotation the revision belongs to.
                 */
                Annotation *annotation() const;

                /**
                 * Sets the @p scope of the revision.
                 * @see RevisionScope
                 */
                void setScope( RevisionScope scope );

                /**
                 * Returns the scope of the revision.
                 */
                RevisionScope scope() const;

                /**
                 * Sets the @p type of the revision.
                 * @see RevisionType
                 */
                void setType( RevisionType type );

                /**
                 * Returns the type of the revision.
                 */
                RevisionType type() const;

            private:
                class Private;
                Private* const d;
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
         * Sets the "native" @p id of the annotation.
         *
         * This is for use of the Generator, that can optionally store an
         * handle (a pointer, an identifier, etc) of the "native" annotation
         * object, if any.
         *
         * @note Okular makes no use of this
         *
         * @since 0.7 (KDE 4.1)
         */
        void setNativeId( const QVariant &id );

        /**
         * Returns the "native" id of the annotation.
         *
         * @since 0.7 (KDE 4.1)
         */
        QVariant nativeId() const;

        /**
         * Sets a function to be called when the annotation is destroyed.
         *
         * @warning the function must *not* call any virtual function,
         *          nor subcast.
         *
         * @since 0.7 (KDE 4.1)
         */
        void setDisposeDataFunction( DisposeDataFunction func );

        /**
         * Returns whether the annotation can be moved.
         *
         * @since 0.7 (KDE 4.1)
         */
        bool canBeMoved() const;

        /**
         * Returns the sub type of the annotation.
         */
        virtual SubType subType() const = 0;

        /**
         * Stores the annotation as xml in @p document under the given parent @p node.
         */
        virtual void store( QDomNode & node, QDomDocument & document ) const;

    protected:
        /// @cond PRIVATE
        Annotation( AnnotationPrivate &dd );
        Annotation( AnnotationPrivate &dd, const QDomNode &description );
        Q_DECLARE_PRIVATE( Annotation )
        AnnotationPrivate *d_ptr;
        /// @endcond

    private:
        Q_DISABLE_COPY( Annotation )
};

class OKULAR_EXPORT TextAnnotation : public Annotation
{
    public:
        /**
         * Describes the type of the text.
         */
        enum TextType
        {
            Linked,  ///< The annotation is linked to a text
            InPlace  ///< The annotation is located next to the text
        };

        /**
         * Describes the style of the text.
         */
        enum InplaceIntent
        {
            Unknown,    ///< Unknown style
            Callout,    ///< Callout style
            TypeWriter  ///< Type writer style
        };

        /**
         * Creates a new text annotation.
         */
        TextAnnotation();

        /**
         * Creates a new text annotation from the xml @p description
         */
        TextAnnotation( const QDomNode &description );

        /**
         * Destroys the text annotation.
         */
        ~TextAnnotation();

        /**
         * Sets the text @p type of the text annotation.
         * @see TextType
         */
        void setTextType( TextType type );

        /**
         * Returns the text type of the text annotation.
         */
        TextType textType() const;

        /**
         * Sets the @p icon of the text annotation.
         */
        void setTextIcon( const QString &icon );

        /**
         * Returns the icon of the text annotation.
         */
        QString textIcon() const;

        /**
         * Sets the @p font of the text annotation.
         */
        void setTextFont( const QFont &font );

        /**
         * Returns the font of the text annotation.
         */
        QFont textFont() const;

        /**
         * Sets the inplace @p alignment of the text annotation.
         */
        void setInplaceAlignment( int alignment );

        /**
         * Returns the inplace alignment of the text annotation.
         */
        int inplaceAlignment() const;

        /**
         * Sets the inplace @p text of the text annotation.
         */
        void setInplaceText( const QString &text );

        /**
         * Returns the inplace text of the text annotation.
         */
        QString inplaceText() const;

        /**
         * Sets the inplace callout @p point at @p index.
         *
         * @p index must be between 0 and 2.
         */
        void setInplaceCallout( const NormalizedPoint &point, int index );

        /**
         * Returns the inplace callout point for @p index.
         *
         * @p index must be between 0 and 2.
         */
        NormalizedPoint inplaceCallout( int index ) const;

        /**
         * Returns the transformed (e.g. rotated) inplace callout point for @p index.
         *
         * @p index must be between 0 and 2.
         */
        NormalizedPoint transformedInplaceCallout( int index ) const;

        /**
         * Returns the inplace @p intent of the text annotation.
         * @see InplaceIntent
         */
        void setInplaceIntent( InplaceIntent intent );

        /**
         * Returns the inplace intent of the text annotation.
         */
        InplaceIntent inplaceIntent() const;

        /**
         * Returns the sub type of the text annotation.
         */
        SubType subType() const;

        /**
         * Stores the text annotation as xml in @p document under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( TextAnnotation )
        Q_DISABLE_COPY( TextAnnotation )
};

class OKULAR_EXPORT LineAnnotation : public Annotation
{
    public:
        /**
         * Describes the line ending style.
         */
        enum TermStyle
        {
            Square,         ///< Using a square
            Circle,         ///< Using a circle
            Diamond,        ///< Using a diamond
            OpenArrow,      ///< Using an open arrow
            ClosedArrow,    ///< Using a closed arrow
            None,           ///< No special ending style
            Butt,           ///< Using a butt ending
            ROpenArrow,     ///< Using an arrow opened at the right side
            RClosedArrow,   ///< Using an arrow closed at the right side
            Slash           ///< Using a slash
        };

        /**
         * Describes the line intent.
         */
        enum LineIntent
        {
            Unknown,        ///< Unknown intent
            Arrow,          ///< Arrow intent
            Dimension,      ///< Dimension intent
            PolygonCloud    ///< Polygon cloud intent
        };

        /**
         * Creates a new line annotation.
         */
        LineAnnotation();

        /**
         * Creates a new line annotation from the xml @p description
         */
        explicit LineAnnotation( const QDomNode &description );

        /**
         * Destroys the line annotation.
         */
        ~LineAnnotation();

        /**
         * Sets the normalized line @p points of the line annotation.
         */
        void setLinePoints( const QLinkedList<NormalizedPoint> &points );

        /**
         * Returns the normalized line points of the line annotation.
         */
        QLinkedList<NormalizedPoint> linePoints() const;

        /**
         * Returns the transformed (e.g. rotated) normalized line points
         * of the line annotation.
         */
        QLinkedList<NormalizedPoint> transformedLinePoints() const;

        /**
         * Sets the line starting @p style of the line annotation.
         * @see TermStyle
         */
        void setLineStartStyle( TermStyle style );

        /**
         * Returns the line starting style of the line annotation.
         */
        TermStyle lineStartStyle() const;

        /**
         * Sets the line ending @p style of the line annotation.
         * @see TermStyle
         */
        void setLineEndStyle( TermStyle style );

        /**
         * Returns the line ending style of the line annotation.
         */
        TermStyle lineEndStyle() const;

        /**
         * Sets whether the line shall be @p closed.
         */
        void setLineClosed( bool closed );

        /**
         * Returns whether the line shall be closed.
         */
        bool lineClosed() const;

        /**
         * Sets the inner line @p color of the line annotation.
         */
        void setLineInnerColor( const QColor &color );

        /**
         * Returns the inner line color of the line annotation.
         */
        QColor lineInnerColor() const;

        /**
         * Sets the leading forward @p point of the line annotation.
         */
        void setLineLeadingForwardPoint( double point );

        /**
         * Returns the leading forward point of the line annotation.
         */
        double lineLeadingForwardPoint() const;

        /**
         * Sets the leading backward @p point of the line annotation.
         */
        void setLineLeadingBackwardPoint( double point );

        /**
         * Returns the leading backward point of the line annotation.
         */
        double lineLeadingBackwardPoint() const;

        /**
         * Sets whether the caption shall be @p shown.
         */
        void setShowCaption( bool shown );

        /**
         * Returns whether the caption shall be shown.
         */
        bool showCaption() const;

        /**
         * Sets the line @p intent of the line annotation.
         * @see LineIntent
         */
        void setLineIntent( LineIntent intent );

        /**
         * Returns the line intent of the line annotation.
         */
        LineIntent lineIntent() const;

        /**
         * Returns the sub type of the line annotation.
         */
        SubType subType() const;

        /**
         * Stores the line annotation as xml in @p document under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( LineAnnotation )
        Q_DISABLE_COPY( LineAnnotation )
};

class OKULAR_EXPORT GeomAnnotation : public Annotation
{
    public:
        // common enums
        enum GeomType
        {
            InscribedSquare,  ///< Draw a square
            InscribedCircle   ///< Draw a circle
        };

        /**
         * Creates a new geometrical annotation.
         */
        GeomAnnotation();

        /**
         * Creates a new geometrical annotation from the xml @p description
         */
        GeomAnnotation( const QDomNode &description );

        /**
         * Destroys the geometrical annotation.
         */
        ~GeomAnnotation();

        /**
         * Sets the geometrical @p type of the geometrical annotation.
         * @see GeomType
         */
        void setGeometricalType( GeomType type );

        /**
         * Returns the geometrical type of the geometrical annotation.
         */
        GeomType geometricalType() const;

        /**
         * Sets the inner @p color of the geometrical annotation.
         */
        void setGeometricalInnerColor( const QColor &color );

        /**
         * Returns the inner color of the geometrical annotation.
         */
        QColor geometricalInnerColor() const;

        /**
         * Sets the point @p width of the geometrical annotation.
         */
        KDE_DEPRECATED void setGeometricalPointWidth( int width );

        /**
         * Returns the point width of the geometrical annotation.
         */
        KDE_DEPRECATED int geometricalPointWidth() const;

        /**
         * Returns the sub type of the geometrical annotation.
         */
        SubType subType() const;

        /**
         * Stores the geometrical annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( GeomAnnotation )
        Q_DISABLE_COPY( GeomAnnotation )
};

class OKULAR_EXPORT HighlightAnnotation : public Annotation
{
    public:
        /**
         * Describes the highlighting style of the annotation.
         */
        enum HighlightType
        {
            Highlight,  ///< Highlights the text
            Squiggly,   ///< Squiggles the text
            Underline,  ///< Underlines the text
            StrikeOut   ///< Strikes out the text
        };

        /**
         * Creates a new highlight annotation.
         */
        HighlightAnnotation();

        /**
         * Creates a new highlight annotation from the xml @p description
         */
        explicit HighlightAnnotation( const QDomNode &description );

        /**
         * Destroys the highlight annotation.
         */
        ~HighlightAnnotation();

        /**
         * Sets the @p type of the highlight annotation.
         * @see HighlightType
         */
        void setHighlightType( HighlightType type );

        /**
         * Returns the type of the highlight annotation.
         */
        HighlightType highlightType() const;

        /**
         * The Quad class contains 8 coordinates and style definitions
         * which describe a line part of the whole highlight annotation.
         */
        class OKULAR_EXPORT Quad
        {
            public:
                /**
                 * Creates a new quad.
                 */
                Quad();

                /**
                 * Destroys the quad.
                 */
                ~Quad();

                Quad( const Quad &other );
                Quad& operator=( const Quad &other );

                /**
                 * Sets the normalized @p point at @p index.
                 *
                 * @p index must be between 0 and 3.
                 */
                void setPoint( const NormalizedPoint &point, int index );

                /**
                 * Returns the normalized point at @p index.
                 *
                 * @p index must be between 0 and 3.
                 */
                NormalizedPoint point( int index ) const;

                /**
                 * Returns the transformed (e.g. rotated) normalized point at @p index.
                 *
                 * @p index must be between 0 and 3.
                 */
                NormalizedPoint transformedPoint( int index ) const;

                /**
                 * Sets whether a cap should be used at the start.
                 */
                void setCapStart( bool value );

                /**
                 * Returns whether a cap should be used at the start.
                 */
                bool capStart() const;

                /**
                 * Sets whether a cap should be used at the end.
                 */
                void setCapEnd( bool value );

                /**
                 * Returns whether a cap should be used at the end.
                 */
                bool capEnd() const;

                /**
                 * Sets the @p width of the drawing feather.
                 */
                void setFeather( double width );

                /**
                 * Returns the width of the drawing feather.
                 */
                double feather() const;

                /**
                 * Transforms the quad coordinates with the transformation defined
                 * by @p matrix.
                 */
                void transform( const QMatrix &matrix );

            private:
                class Private;
                Private* const d;
        };

        /**
         * Returns a reference to the quad list of the highlight annotation.
         */
        QList< Quad > & highlightQuads();

        /**
         * Returns the sub type of the highlight annotation.
         */
        SubType subType() const;

        /**
         * Stores the highlight annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( HighlightAnnotation )
        Q_DISABLE_COPY( HighlightAnnotation )
};

class OKULAR_EXPORT StampAnnotation : public Annotation
{
    public:
        /**
         * Creates a new stamp annotation.
         */
        StampAnnotation();

        /**
         * Creates a new stamp annotation from the xml @p description
         */
        explicit StampAnnotation( const QDomNode &description );

        /**
         * Destroys the stamp annotation.
         */
        ~StampAnnotation();

        /**
         * Sets the @p name of the icon for the stamp annotation.
         */
        void setStampIconName( const QString &name );

        /**
         * Returns the name of the icon.
         */
        QString stampIconName() const;

        /**
         * Returns the sub type of the stamp annotation.
         */
        SubType subType() const;

        /**
         * Stores the stamp annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( StampAnnotation )
        Q_DISABLE_COPY( StampAnnotation )
};

class OKULAR_EXPORT InkAnnotation : public Annotation
{
    public:
        /**
         * Creates a new ink annotation.
         */
        InkAnnotation();

        /**
         * Creates a new ink annotation from the xml @p description
         */
        InkAnnotation( const QDomNode &description );

        /**
         * Destroys the ink annotation.
         */
        ~InkAnnotation();

        /**
         * Sets the @p paths of points for the ink annotation.
         */
        void setInkPaths( const QList< QLinkedList<NormalizedPoint> > &paths );

        /**
         * Returns the paths of points of the ink annotation.
         */
        QList< QLinkedList<NormalizedPoint> > inkPaths() const;

        /**
         * Returns the paths of transformed (e.g. rotated) points of
         * the ink annotation.
         */
        QList< QLinkedList<NormalizedPoint> > transformedInkPaths() const;

        /**
         * Returns the sub type of the ink annotation.
         */
        SubType subType() const;

        /**
         * Stores the ink annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( InkAnnotation )
        Q_DISABLE_COPY( InkAnnotation )
};

class OKULAR_EXPORT CaretAnnotation : public Annotation
{
    public:
        /**
         * Describes the highlighting style of the annotation.
         */
        enum CaretSymbol
        {
            None,       ///< No symbol to be associated with the text
            P           ///< A 'paragraph' symbol
        };

        /**
         * Creates a new caret annotation.
         */
        CaretAnnotation();

        /**
         * Creates a new caret annotation from the xml @p description
         */
        explicit CaretAnnotation( const QDomNode &description );

        /**
         * Destroys the caret annotation.
         */
        ~CaretAnnotation();

        /**
         * Sets the @p symbol for the caret annotation.
         */
        void setCaretSymbol( CaretAnnotation::CaretSymbol symbol );

        /**
         * Returns the symbol of the annotation.
         */
        CaretAnnotation::CaretSymbol caretSymbol() const;

        /**
         * Returns the sub type of the caret annotation.
         */
        SubType subType() const;

        /**
         * Stores the caret annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( CaretAnnotation )
        Q_DISABLE_COPY( CaretAnnotation )
};

class OKULAR_EXPORT FileAttachmentAnnotation : public Annotation
{
    public:
        /**
         * Creates a new file attachment annotation.
         */
        FileAttachmentAnnotation();
        /**
         * Creates a new file attachment annotation from the xml @p description
         */
        explicit FileAttachmentAnnotation( const QDomNode &description );
        /**
         * Destroys the file attachment annotation.
         */
        virtual ~FileAttachmentAnnotation();

        /**
         * Gets the name of the icon.
         */
        QString fileIconName() const;

        /**
         * Sets the @p name of the icon for the file attachment annotation.
         */
        void setFileIconName( const QString &name );

        /**
         * Gets the embedded file object.
         */
        EmbeddedFile* embeddedFile() const;

        /**
         * Sets the @p object representing the embedded file of the file
         * attachment annotation.
         */
        void setEmbeddedFile( EmbeddedFile *object );

        /**
         * Returns the sub type of the file attachment annotation.
         */
        SubType subType() const;

        /**
         * Stores the file attachment annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( FileAttachmentAnnotation )
        Q_DISABLE_COPY( FileAttachmentAnnotation )
};

/**
 * \short Sound annotation.
 *
 * The sound annotation represents a sound to be played when activated.
 *
 * @since 0.7 (KDE 4.1)
 */
class OKULAR_EXPORT SoundAnnotation : public Annotation
{
    public:
        /**
         * Creates a new sound annotation.
         */
        SoundAnnotation();
        /**
         * Creates a new sound annotation from the xml @p description
         */
        SoundAnnotation( const QDomNode &description );
        /**
         * Destroys the sound annotation.
         */
        virtual ~SoundAnnotation();

        /**
         * Gets the name of the icon.
         */
        QString soundIconName() const;

        /**
         * Sets the @p name of the icon for the sound annotation.
         */
        void setSoundIconName( const QString &name );

        /**
         * Gets the sound object.
         */
        Sound* sound() const;

        /**
         * Sets the @p object representing the sound of the file
         * attachment annotation.
         */
        void setSound( Sound *object );

        /**
         * Returns the sub type of the sound annotation.
         */
        SubType subType() const;

        /**
         * Stores the sound annotation as xml in @p document
         * under the given parent @p node.
         */
        void store( QDomNode &node, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( SoundAnnotation )
        Q_DISABLE_COPY( SoundAnnotation )
};

/**
 * \short Movie annotation.
 *
 * The movie annotation represents a movie to be played when activated.
 *
 * @since 0.8 (KDE 4.2)
 */
class OKULAR_EXPORT MovieAnnotation : public Annotation
{
    public:
        /**
         * Creates a new movie annotation.
         */
        MovieAnnotation();
        /**
         * Creates a new movie annotation from the xml @p description
         */
        MovieAnnotation( const QDomNode &description );
        /**
         * Destroys the movie annotation.
         */
        virtual ~MovieAnnotation();
        /**
         * Gets the movie object.
         */
        Movie* movie() const;
        /**
         * Sets the new @p movie object.
         */
        void setMovie( Movie *movie );
        /**
         * Returns the sub type of the movie annotation.
         */
        SubType subType() const;
        /**
         * Stores the movie annotation as xml in @p document
         * under the given @p parentNode.
         */
        void store( QDomNode &parentNode, QDomDocument &document ) const;

    private:
        Q_DECLARE_PRIVATE( MovieAnnotation )
        Q_DISABLE_COPY( MovieAnnotation )
};

}

#endif
