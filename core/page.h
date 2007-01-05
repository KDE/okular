/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGE_H_
#define _OKULAR_PAGE_H_

#include <QtCore/QLinkedList>
#include <QtCore/QMap>

#include <okular/core/okular_export.h>
#include <okular/core/area.h>
#include <okular/core/global.h>

class QDomDocument;
class QDomNode;
class QRect;

class PagePainter;

namespace Okular {

class Annotation;
class PageSize;
class PageTransition;
class SourceReference;
class TextPage;
class TextSelection;

/**
 * @short Collector for all the data belonging to a page.
 *
 * The Page class contains pixmaps (referenced using observers id as key),
 * a search page (a class used internally for retrieving text), rect classes
 * (that describe links or other active areas in the current page) and more.
 *
 * All coordinates are normalized to the page, so {x,y} are valid in [0,1]
 * range as long as NormalizedRect components.
 *
 * Note: The class takes ownership of all objects.
 */
class OKULAR_EXPORT Page : public QObject
{
    Q_OBJECT

    public:
        enum PageAction
        {
            Opening,
            Closing
        };

        /**
         * Creates a new page.
         *
         * @param number The number of the page in the document.
         * @param width The width of the page.
         * @param height The height of the page.
         * @param orientation The orientation of the page
         */
        Page( uint number, double width, double height, Rotation orientation );

        /**
         * Destroys the page.
         */
        ~Page();

        /**
         * Returns the number of the page in the document.
         */
        int number() const;

        /**
         * Returns the orientation of the page as defined by the document.
         */
        Rotation orientation() const;

        /**
         * Returns the rotation of the page as defined by the user.
         */
        Rotation rotation() const;

        /**
         * Returns the total orientation which is the original orientation plus
         * the user defined rotation.
         */
        Rotation totalOrientation() const;

        /**
         * Returns the width of the page.
         */
        double width() const;

        /**
         * Returns the height of the page.
         */
        double height() const;

        /**
         * Returns the ration (height / width) of the page.
         */
        double ratio() const;

        /**
         * Returns whether the page has a pixmap of size @p width x @p height
         * for the observer with given @p id.
         */
        bool hasPixmap( int id, int width = -1, int height = -1 ) const;

        /**
         * Returns whether the page provides a text page (@see TextPage).
         */
        bool hasTextPage() const;

        /**
         * Returns whether the page has an object rect which includes the point (@p x, @p y)
         * at scale (@p xScale, @p yScale).
         */
        bool hasObjectRect( double x, double y, double xScale, double yScale ) const;

        /**
         * Returns whether the page provides highlighting for the observer with the
         * given @p id.
         */
        bool hasHighlights( int id = -1 ) const;

        /**
         * Returns whether the page provides a transition effect.
         */
        bool hasTransition() const;

        /**
         * Returns whether the page provides annotations.
         */
        bool hasAnnotations() const;

        /**
         * Returns the bounding rect of the text which matches the following criteria
         * or 0 if the search is not successful.
         *
         * @param id An unique id for this search.
         * @param text The search text.
         * @param direction The direction of the search (@see SearchDirection)
         * @param caseSensitivity If Qt::CaseSensitive, the search is case sensitive; otherwise
         *                        the search is case insensitive.
         * @param lastRect If 0 (default) the search starts at the beginning of the page, otherwise
         *                 right/below the coordinates of the the given rect.
         */
        RegularAreaRect* findText( int id, const QString & text, SearchDirection direction,
                                   Qt::CaseSensitivity caseSensitivity, const RegularAreaRect * lastRect=0) const;

        /**
         * Returns the page text (or part of it).
         * @see TextPage::text()
         */
        QString text( const RegularAreaRect * rect = 0 ) const;

        /**
         * Returns the rectangular area of the given @p selection.
         */
        RegularAreaRect * textArea( TextSelection *selection ) const;

        /**
         * Returns the object rect of the given @p type which is at point (@p x, @p y) at scale (@p xScale, @p yScale).
         */
        const ObjectRect * objectRect( ObjectRect::ObjectType type, double x, double y, double xScale, double yScale ) const;

        /**
         * Returns the transition effect of the page or 0 if no transition effect is set (@see hasTransition).
         */
        const PageTransition * transition() const;

        /**
         * Returns the list of annotations of the page.
         */
        const QLinkedList< Annotation* > annotations() const;

        /**
         * Returns the @see Link object which is associated with the given page @p action
         * or 0 if no page action is set.
         */
        const Link * pageAction( PageAction action ) const;

        /**
         * Rotates the image and object rects of the page to the given @p orientation.
         */
        void rotateAt( Rotation orientation );

        /**
         * Changes the size of the page to the given @p size.
         *
         * The @p size is meant to be referred to the page not rotated.
         */
        void changeSize( const PageSize &size );

        /**
         * Sets the @p pixmap for the observer with the given @p id.
         */
        void setPixmap( int id, QPixmap *pixmap );

        /**
         * Sets the @p text page.
         */
        void setTextPage( TextPage * text );

        /**
         * Sets the list of object @p rects of the page.
         */
        void setObjectRects( const QLinkedList< ObjectRect * > rects );

        /**
         * Sets the @p color and @p area of the highlight for the observer with
         * the given @p id.
         */
        void setHighlight( int id, RegularAreaRect *area, const QColor & color );

        /**
         * Sets the @p color and @p areas of text selections.
         */
        void setTextSelections( RegularAreaRect *areas, const QColor & color );

        /**
         * Sets the list of source reference objects @p rects.
         */
        void setSourceReferences( const QLinkedList< SourceRefObjectRect * > rects );

        /**
         * Sets the duration of the page to @p seconds when displayed in presentation mode.
         *
         * Setting a negative number disables the duration.
         */
        void setDuration( double seconds );

        /**
         * Returns the duration in seconds of the page when displayed in presentation mode.
         *
         * A negative number means that no time is set.
         */
        double duration() const;

        /**
         * Returns the current text selection.
         */
        const RegularAreaRect * textSelection() const;

        /**
         * Adds a new @p annotation to the page.
         */
        void addAnnotation( Annotation * annotation );

        /**
         * Modifies an existing annotation by replacing it with a new @p annotation.
         *
         * The unique name is used to find the old annotation.
         */
        void modifyAnnotation( Annotation * annotation );

        /**
         * Removes the @p annotation from the page.
         */
        bool removeAnnotation( Annotation * annotation );

        /**
         * Sets the page @p transition effect.
         */
        void setTransition( PageTransition * transition );

        /**
         * Sets the @p link object for the given page @p action.
         */
        void setPageAction( PageAction action, Link * link );

        /**
         * Deletes the pixmap for the observer with the given @p id.
         */
        void deletePixmap( int id );

        /**
         * Deletes all pixmaps of the page.
         */
        void deletePixmaps();

        /**
         * Deletes all object rects of the page.
         */
        void deleteRects();

        /**
         * Deletes all highlight objects for the observer with the given @p id.
         */
        void deleteHighlights( int id = -1 );

        /**
         * Deletes all text selection objects of the page.
         */
        void deleteTextSelections();

        /**
         * Deletes all source reference objects of the page.
         */
        void deleteSourceReferences();

        /**
         * Deletes all annotations of the page.
         */
        void deleteAnnotations();

        /**
         * Loads the local contents (e.g. annotations) of the page.
         */
        void restoreLocalContents( const QDomNode & pageNode );

        /**
         * Saves the local contents (e.g. annotations) of the page.
         */
        void saveLocalContents( QDomNode & parentNode, QDomDocument & document );

    Q_SIGNALS:
        void rotationFinished( int page );

    private:
        class Private;
        Private* const d;

        /**
         * To improve performance PagePainter accesses the following
         * member variables directly.
         */
        friend class ::PagePainter;

        class PixmapObject
        {
            public:
                QPixmap *m_pixmap;
                Rotation m_rotation;
        };
        QMap< int, PixmapObject > m_pixmaps;

        QLinkedList< ObjectRect* > m_rects;
        QLinkedList< HighlightAreaRect* > m_highlights;
        QLinkedList< Annotation* > m_annotations;
        HighlightAreaRect *m_textSelections;

        Q_DISABLE_COPY( Page )
        Q_PRIVATE_SLOT( d, void imageRotationDone() )

};

}

#endif
