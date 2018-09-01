/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _SIDEBAR_H_
#define _SIDEBAR_H_

#include <qwidget.h>
#include "okularpart_export.h"

class QIcon;
class QListWidgetItem;

class OKULARPART_EXPORT Sidebar : public QWidget
{
    Q_OBJECT
    public:
        explicit Sidebar( QWidget *parent = nullptr );
        ~Sidebar();

        int addItem( QWidget *widget, const QIcon &icon, const QString &text );

        void setMainWidget( QWidget *widget );
        void setBottomWidget( QWidget *widget );

        void setItemEnabled( QWidget *widget, bool enabled );
        bool isItemEnabled( QWidget *widget ) const;

        void setItemVisible( QWidget *widget, bool visible );

        enum SetCurrentItemBehaviour { UncollapseIfCollapsed, DoNotUncollapseIfCollapsed };

        void setCurrentItem( QWidget *widget, SetCurrentItemBehaviour b = UncollapseIfCollapsed );
        QWidget *currentItem() const;

        void setSidebarVisibility( bool visible );
        bool isSidebarVisible() const;

        void setCollapsed( bool collapsed );
        bool isCollapsed() const;

        void moveSplitter( int sideWidgetSize );

    Q_SIGNALS:
        void urlsDropped( const QList<QUrl>& urls );

    protected:
        void dragEnterEvent( QDragEnterEvent* event ) override;
        void dropEvent( QDropEvent* event ) override;

    private Q_SLOTS:
        void itemClicked( QListWidgetItem *item );
        void splitterMoved( int pos, int index );
        void listContextMenu( const QPoint & );
        void showTextToggled( bool );
        void iconSizeChanged( QAction *action );

    private:
        void setIndexEnabled( int index, bool enabled );
        void setCurrentIndex( int index, SetCurrentItemBehaviour b = UncollapseIfCollapsed );
        bool isIndexEnabled( int index ) const;
        void itemClicked( QListWidgetItem *item, SetCurrentItemBehaviour b );
        void saveSplitterSize() const;

        // private storage
        class Private;
        Private *d;
};

#endif
