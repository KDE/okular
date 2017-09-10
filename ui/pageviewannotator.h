/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGEVIEWANNOTATOR_H_
#define _OKULAR_PAGEVIEWANNOTATOR_H_

#include <qobject.h>
#include <qdom.h>
#include <qlinkedlist.h>

#include "pageviewutils.h"
#include "annotationtools.h"

class QKeyEvent;
class QMouseEvent;
class QPainter;

namespace Okular
{
class Document;
}

// engines are defined and implemented in the cpp
class AnnotatorEngine;
class PageView;

/**
 * @short PageView object devoted to annotation creation/handling.
 *
 * PageViewAnnotator is the okular class used for visually creating annotations.
 * It uses internal 'engines' for interacting with user events and attaches
 * the newly created annotation to the document when the creation is complete.
 * In the meanwhile all PageView events (actually mouse/paint ones) are routed
 * to this class that performs a rough visual representation of what the
 * annotation will become when finished.
 *
 * m_toolsDefinition is a DOM object that contains Annotations/Engine association
 * for the items placed in the toolbar. The XML is parsed (1) when populating
 * the toolbar and (2)after selecting a toolbar item, in which case an Ann is
 * initialized with the values in the XML and an engine is created to handle
 * that annotation. m_toolsDefinition is created in reparseConfig according to
 * user configuration.
 */
class PageViewAnnotator : public QObject
{
    Q_OBJECT
    public:
        PageViewAnnotator( PageView * parent, Okular::Document * storage );
        ~PageViewAnnotator();

        // called to show/hide the editing toolbar
        void setEnabled( bool enabled );

        // called to toggle the usage of text annotating tools
        void setTextToolsEnabled( bool enabled );

        void setToolsEnabled( bool enabled );

        void setHidingForced( bool forced );
        bool hidingWasForced() const;

        // methods used when creating the annotation
        // @return Is a tool currently selected?
        bool active() const;
        // @return Are we currently annotating (using the selected tool)?
        bool annotating() const;

        // returns the preferred cursor for the current tool. call this only
        // if active() == true
        QCursor cursor() const;

        QRect routeMouseEvent( QMouseEvent * event, PageViewItem * item );
        QRect routeTabletEvent( QTabletEvent * event, PageViewItem * item, const QPoint & localOriginInGlobal );
        QRect performRouteMouseOrTabletEvent( const AnnotatorEngine::EventType & eventType, const AnnotatorEngine::Button & button,
                                              const QPointF & pos, PageViewItem * item );
        bool routeKeyEvent( QKeyEvent * event );
        bool routePaints( const QRect & wantedRect ) const;
        void routePaint( QPainter * painter, const QRect & paintRect );

        void reparseConfig();

        static QString defaultToolName( const QDomElement &toolElement );
        static QPixmap makeToolPixmap( const QDomElement &toolElement );

    private Q_SLOTS:
        void slotToolSelected( int toolID );
        void slotSaveToolbarOrientation( int side );
        void slotToolDoubleClicked( int toolID );

    private:
        void detachAnnotation();

        // global class pointers
        Okular::Document * m_document;
        PageView * m_pageView;
        PageViewToolBar * m_toolBar;
        AnnotatorEngine * m_engine;
        QDomElement m_toolsDefinition;
        QLinkedList<AnnotationToolItem> m_items;
        bool m_textToolsEnabled;
        bool m_toolsEnabled;
        bool m_continuousMode;
        bool m_hidingWasForced;

        // creation related variables
        int m_lastToolID;
        QRect m_lastDrawnRect;
        PageViewItem * m_lockedItem;
        //selected annotation name
        //QString m_selectedAnnotationName;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
