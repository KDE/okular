/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGEVIEWANNOTATOR_H_
#define _KPDF_PAGEVIEWANNOTATOR_H_

#include <qobject.h>
#include <qdom.h>

// engines are defined and implemented in the cpp
class AnnotatorEngine;

/**
 * @short PageView object devoted to annotation creation/handling.
 *
 * PageViewAnnotator is the kpdf class used for visually creating annotations.
 * It uses internal 'engines' for interacting with user events and attaches
 * the newly created annotation to the document when the creation is complete.
 * In the meanwhile all PageView events (actually mouse/paint ones) are routed
 * to this class that performs a rough visual representation of what the
 * annotation will become when finished.
 *
 * "data/tools.xml" is the file that contains Annotations/Engine association
 * for the items placed in the toolbar. The XML is parsed (1) when populating
 * the toolbar and (2)after selecting a toolbar item, in which case an Ann is
 * initialized with the values in the XML and an engine is created to handle
 * that annotation.
 */
class PageViewAnnotator : public QObject
{
    Q_OBJECT
    public:
        PageViewAnnotator( PageView * parent, KPDFDocument * storage );
        ~PageViewAnnotator();

        // called to show/hide the editing toolbar
        void setEnabled( bool enabled );

        // methods used when creating the annotation
        bool routeEvents() const;
        void routeEvent( QMouseEvent * event, PageViewItem * item );
        bool routePaints( const QRect & wantedRect ) const;
        void routePaint( QPainter * painter, const QRect & paintRect );

    private slots:
        void slotToolSelected( int toolID );
        void slotSaveToolbarOrientation( int side );

    private:
        // global class pointers
        KPDFDocument * m_document;
        PageView * m_pageView;
        PageViewToolBar * m_toolBar;
        AnnotatorEngine * m_engine;
        QDomElement m_toolsDefinition;

        // creation related variables
        int m_lastToolID;
        QRect m_lastDrawnRect;
        PageViewItem * m_lockedItem;
        //selected annotation name
        QString m_selectedAnnotationName;
};

#endif
