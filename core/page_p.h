/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGE_PRIVATE_H_
#define _OKULAR_PAGE_PRIVATE_H_

// qt/kde includes
#include <qlinkedlist.h>
#include <qmap.h>
#include <qtransform.h>
#include <qstring.h>
#include <qdom.h>

// local includes
#include "global.h"
#include "area.h"

class QColor;

namespace Okular {

class Action;
class Annotation;
class DocumentObserver;
class DocumentPrivate;
class FormField;
class HighlightAreaRect;
class Page;
class PageSize;
class PageTransition;
class RotationJob;
class TextPage;
class TilesManager;

enum PageItem
{
    None = 0,
    AnnotationPageItems = 0x01,
    FormFieldPageItems = 0x02,
    AllPageItems = 0xff,

    /* If set along with AnnotationPageItems, tells saveLocalContents to save
     * the original annotations (if any) instead of the modified ones */
    OriginalAnnotationPageItems = 0x100,

    /* If set along with FormFieldPageItems, tells saveLocalContents to save
     * the original form contents (if any) instead of the modified one */
    OriginalFormFieldPageItems = 0x200
};
Q_DECLARE_FLAGS(PageItems, PageItem)

class PagePrivate
{
    public:
        PagePrivate( Page *page, uint n, double w, double h, Rotation o );
        ~PagePrivate();

        static PagePrivate *get( Page *page );

        void imageRotationDone( RotationJob * job );
        QTransform rotationMatrix() const;

        /**
         * Loads the local contents (e.g. annotations) of the page.
         */
        bool restoreLocalContents( const QDomNode & pageNode );

        /**
         * Saves the local contents (e.g. annotations) of the page.
         */
        void saveLocalContents( QDomNode & parentNode, QDomDocument & document, PageItems what = AllPageItems ) const;

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
         * Sets the @p color and @p areas of text selections.
         */
        void setTextSelections( RegularAreaRect *areas, const QColor & color );

        /**
         * Sets the @p color and @p area of the highlight for the observer with
         * the given @p id.
         */
        void setHighlight( int id, RegularAreaRect *area, const QColor & color );

        /**
         * Deletes all highlight objects for the observer with the given @p id.
         */
        void deleteHighlights( int id = -1 );

        /**
         * Deletes all text selection objects of the page.
         */
        void deleteTextSelections();

        /**
         * Get the tiles manager for the tiled @observer
         */
        TilesManager *tilesManager( const DocumentObserver *observer ) const;

        /**
         * Set the tiles manager for the tiled @observer
         */
        void setTilesManager( const DocumentObserver *observer, TilesManager *tm );

        /**
         * Moves contents that are generated from oldPage to this. And clears them from page
         * so it can be deleted fine.
         */
        void adoptGeneratedContents( PagePrivate *oldPage );

        /*
         * Tries to find an equivalent form field to oldField by looking into the rect, type and name
         */
        OKULARCORE_EXPORT static FormField *findEquivalentForm( const Page *p, FormField *oldField );

        class PixmapObject
        {
            public:
                QPixmap *m_pixmap;
                Rotation m_rotation;
        };
        QMap< DocumentObserver*, PixmapObject > m_pixmaps;
        QMap< const DocumentObserver*, TilesManager *> m_tilesManagers;

        Page *m_page;
        int m_number;
        Rotation m_orientation;
        double m_width, m_height;
        DocumentPrivate *m_doc;
        NormalizedRect m_boundingBox;
        Rotation m_rotation;

        TextPage * m_text;
        PageTransition * m_transition;
        HighlightAreaRect *m_textSelections;
        QLinkedList< FormField * > formfields;
        Action * m_openingAction;
        Action * m_closingAction;
        double m_duration;
        QString m_label;

        bool m_isBoundingBoxKnown : 1;
        QDomDocument restoredLocalAnnotationList; // <annotationList>...</annotationList>
        QDomDocument restoredFormFieldList; // <forms>...</forms>
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Okular::PageItems)

#endif
