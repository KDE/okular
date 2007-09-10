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

namespace Okular {
class Annotation;
class Document;
}

class AnnotationModel;
class AuthorGroupProxyModel;
class PageFilterProxyModel;
class PageGroupProxyModel;
class TreeView;

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
        void notifyViewportChanged( bool smoothMove );

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
        TreeView * m_view;
        // internal storage
        Okular::Document * m_document;
        AnnotationModel * m_model;
        AuthorGroupProxyModel * m_authorProxy;
        PageFilterProxyModel * m_filterProxy;
        PageGroupProxyModel * m_groupProxy;
};

#endif
