/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_SIDE_REVIEWS_H_
#define _OKULAR_SIDE_REVIEWS_H_

#include <QtCore/QVector>
#include <QtGui/QWidget>

#include "core/observer.h"

class QModelIndex;
class QTreeView;

namespace Okular {
class Annotation;
class Document;
class Page;
}

class AnnotationModel;
class PageFilterProxyModel;
class PageGroupProxyModel;

/**
 * @short ...
 */
class Reviews : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        Reviews( QWidget * parent, Okular::Document * document );

        // [INHERITED] from DocumentObserver
        uint observerId() const { return REVIEWS_ID; }
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );

    public Q_SLOTS:
        void slotPageEnabled( bool );
        void slotAuthorEnabled( bool );
        void slotCurrentPageOnly( bool );

    Q_SIGNALS:
        void setAnnotationWindow( Okular::Annotation *annotation );
        void removeAnnotationWindow( Okular::Annotation *annotation );

    private Q_SLOTS:
        void activated( const QModelIndex& );
        void contextMenuRequested( const QPoint& );

    private:
        // data fields (GUI)
        QTreeView * m_view;
        // internal storage
        Okular::Document * m_document;
        AnnotationModel * m_model;
        PageFilterProxyModel * m_filterProxy;
        PageGroupProxyModel * m_groupProxy;
};

#endif
