/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
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
#include <qmatrix.h>
#include <qstring.h>

// local includes
#include "global.h"

class QDomDocument;
class QDomNode;

namespace Okular {

class Action;
class Annotation;
class FormField;
class HighlightAreaRect;
class Page;
class PageSize;
class PageTransition;
class RotationJob;
class TextPage;

class PagePrivate
{
    public:
        PagePrivate( Page *page, uint n, double w, double h, Rotation o );
        ~PagePrivate();

        void imageRotationDone( RotationJob * job );
        QMatrix rotationMatrix() const;

        /**
         * Loads the local contents (e.g. annotations) of the page.
         */
        void restoreLocalContents( const QDomNode & pageNode );

        /**
         * Saves the local contents (e.g. annotations) of the page.
         */
        void saveLocalContents( QDomNode & parentNode, QDomDocument & document ) const;

        /**
         * Modifies an existing annotation by replacing it with a new @p annotation.
         *
         * The unique name is used to find the old annotation.
         */
        void modifyAnnotation( Annotation * annotation );

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

        class PixmapObject
        {
            public:
                QPixmap *m_pixmap;
                Rotation m_rotation;
        };
        QMap< int, PixmapObject > m_pixmaps;

        Page *m_page;
        int m_number;
        Rotation m_orientation;
        double m_width, m_height;
        Rotation m_rotation;
        int m_maxuniqueNum;

        TextPage * m_text;
        PageTransition * m_transition;
        HighlightAreaRect *m_textSelections;
        QLinkedList< FormField * > formfields;
        Action * m_openingAction;
        Action * m_closingAction;
        double m_duration;
        QString m_label;
};

}

#endif
