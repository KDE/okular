/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGE_H_
#define _KPDF_PAGE_H_

#include "okular_export.h"

#include <qmap.h>
#include <qlinkedlist.h>


class QPixmap;
class QRect;
class QDomNode;
class QDomDocument;
class KPDFPageTransition;
class Annotation;
class KPDFTextPage;
class TextSelection;
// didnt work with forward declarations
#include "area.h"
#include "textpage.h"
/*
class NormalizedRect;
class NormalizedPoint;
class RegularAreaRect;
class HighlightRect;
class HighlightAreaRect;
class ObjectRect;
*/


/**
 * @short Collector for all the data belonging to a page.
 *
 * The KPDFPage class contains pixmaps (referenced using observers id as key),
 * a search page (a class used internally for retrieving text), rect classes
 * (that describe links or other active areas in the current page) and more.
 *
 * All coordinates are normalized to the page, so {x,y} are valid in [0,1]
 * range as long as NormalizedRect components.
 *
 * Note: The class takes ownership of all objects.
 */
class OKULAR_EXPORT KPDFPage
{
    public:
        KPDFPage( uint number, double width, double height, int orientation );
        ~KPDFPage();

        // query properties (const read-only methods)
        inline int number() const { return m_number; }
        inline int orientation() const { return m_orientation; }
        inline double width() const { return m_width; }
        inline double height() const { return m_height; }
        inline double ratio() const { return m_height / m_width; }
        bool hasPixmap( int p_id, int width = -1, int height = -1 ) const;
        bool hasSearchPage() const;
        bool hasBookmark() const;
        bool hasObjectRect( double x, double y ) const;
        bool hasHighlights( int s_id = -1 ) const;
        //bool hasAnnotation( double x, double y ) const;
        bool hasTransition() const;

        RegularAreaRect * findText( int searchID, const QString & text, SearchDir dir, bool strictCase, const RegularAreaRect * lastRect=0) const;
        QString getText( const RegularAreaRect * rect ) const;
	RegularAreaRect * getTextArea ( TextSelection * ) const;
//	const ObjectRect * getObjectRect( double x, double y ) const;
        const ObjectRect * getObjectRect( ObjectRect::ObjectType type, double x, double y ) const;
        //const Annotation * getAnnotation( double x, double y ) const;
        const KPDFPageTransition * getTransition() const;
        //FIXME TEMP:
        bool hasAnnotations() const { return !m_annotations.isEmpty(); }
        const QLinkedList< Annotation * > getAnnotations() const { return m_annotations; }

        // operations: set contents (by KPDFDocument)
        void setPixmap( int p_id, QPixmap * pixmap );
        void setSearchPage( KPDFTextPage * text );
        void setBookmark( bool state );
        void setObjectRects( const QLinkedList< ObjectRect * > rects );
        void setHighlight( int s_id, RegularAreaRect *r, const QColor & color );
        void addAnnotation( Annotation * annotation );
        void setTransition( KPDFPageTransition * transition );
        // operations: delete contents (by KPDFDocument)
        void deletePixmap( int p_id );
        void deletePixmapsAndRects();
        void deleteHighlights( int s_id = -1 );
        void deleteAnnotations();

        // operations to save/restore page state (by KPDFDocument)
        void restoreLocalContents( const QDomNode & pageNode );
        void saveLocalContents( QDomNode & parentNode, QDomDocument & document );

    private:
        friend class PagePainter;
        friend class PageViewAnnotator;
        int m_number;
        int m_orientation;
        double m_width, m_height;
        bool m_bookmarked;

        QMap< int, QPixmap * > m_pixmaps;
        KPDFTextPage * m_text;
        QLinkedList< ObjectRect * > m_rects;
        QLinkedList< HighlightAreaRect * > m_highlights;
        QLinkedList< Annotation * > m_annotations;
        KPDFPageTransition * m_transition;
};

#endif
