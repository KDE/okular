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

#include "area.h"
#include "okular_export.h"
#include "textpage.h"

class QDomDocument;
class QDomNode;
class QRect;

class PagePainter;

namespace Okular {

class Annotation;
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
         * @param orientation The orientation of the page with
         *                    0 = 0°
         *                    1 = 90°
         *                    2 = 180°
         *                    3 = 270°
         */
        Page( uint number, double width, double height, int orientation );

        /**
         * Destroys the page.
         */
        ~Page();

        // query properties (const read-only methods)
        inline int number() const { return m_number; }
        inline int orientation() const { return m_orientation; }
        inline int rotation() const { return m_rotation; }
        inline int totalOrientation() const { return ( m_orientation + m_rotation ) % 4; }
        inline double width() const { return m_width; }
        inline double height() const { return m_height; }
        inline double ratio() const { return m_height / m_width; }

        /**
         * Returns whether the page has a pixmap of size @p width x @p height
         * for the observer with given @p id.
         */
        bool hasPixmap( int id, int width = -1, int height = -1 ) const;

        /**
         * Returns whether the page provides a text page (@see TextPage).
         */
        bool hasTextPage() const;
        bool hasBookmark() const;
        bool hasObjectRect( double x, double y, double xScale, double yScale ) const;
        bool hasHighlights( int s_id = -1 ) const;
        //bool hasAnnotation( double x, double y ) const;
        bool hasTransition() const;

        RegularAreaRect * findText( int searchID, const QString & text, SearchDir dir, bool strictCase, const RegularAreaRect * lastRect=0) const;
        QString getText( const RegularAreaRect * rect ) const;
        RegularAreaRect * getTextArea ( TextSelection * ) const;
        //const ObjectRect * getObjectRect( double x, double y ) const;
        const ObjectRect * getObjectRect( ObjectRect::ObjectType type, double x, double y, double xScale, double yScale ) const;
        //const Annotation * getAnnotation( double x, double y ) const;
        const PageTransition * getTransition() const;
        const Link * getPageAction( PageAction act ) const;
        //FIXME TEMP:
        bool hasAnnotations() const { return !m_annotations.isEmpty(); }
        const QLinkedList< Annotation * > getAnnotations() const { return m_annotations; }

        // operations for rotate the page
        void rotateAt( int orientation );

        // operations: set contents (by Document)
        void setPixmap( int p_id, QPixmap *pixmap );
        void setTextPage( TextPage * text );
        void setBookmark( bool state );
        void setObjectRects( const QLinkedList< ObjectRect * > rects );
        void setHighlight( int s_id, RegularAreaRect *r, const QColor & color );
        void setTextSelections( RegularAreaRect *r, const QColor & color );
        void setSourceReferences( const QLinkedList< SourceRefObjectRect * > refRects );
        void addAnnotation( Annotation * annotation );
        void modifyAnnotation( Annotation * newannotation );
        bool removeAnnotation( Annotation * annotation );
        void setTransition( PageTransition * transition );
        void setPageAction( PageAction act, Link * action );
        // operations: delete contents (by Document)
        void deletePixmap( int s_id );
        void deletePixmaps();
        void deleteRects();
        void deleteHighlights( int s_id = -1 );
        void deleteTextSelections();
        void deleteSourceReferences();
        void deleteAnnotations();

        // operations to save/restore page state (by Document)
        void restoreLocalContents( const QDomNode & pageNode );
        void saveLocalContents( QDomNode & parentNode, QDomDocument & document );

    Q_SIGNALS:
        void rotationFinished( int page );

    private Q_SLOTS:
        void imageRotationDone();

    private:
        friend class ::PagePainter;

        QMatrix rotationMatrix() const;

        int m_number;
        int m_orientation;
        int m_rotation;
        double m_width, m_height;
        bool m_bookmarked;
        int m_maxuniqueNum;

        class PixmapObject
        {
            public:
                QPixmap *m_pixmap;
                int m_rotation;
        };
        QMap< int, PixmapObject > m_pixmaps;

        TextPage * m_text;
        QLinkedList< ObjectRect * > m_rects;
        QLinkedList< HighlightAreaRect * > m_highlights;
        QLinkedList< Annotation * > m_annotations;
        PageTransition * m_transition;
        HighlightAreaRect * m_textSelections;
        Link * m_openingAction;
        Link * m_closingAction;
};

}

#endif
