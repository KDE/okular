/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PAGEITEMDELEGATE_H
#define PAGEITEMDELEGATE_H

#include <QItemDelegate>

class PageItemDelegate : public QItemDelegate
{
    Q_OBJECT

    public:
        PageItemDelegate( QObject * parent = nullptr );
        virtual ~PageItemDelegate();

        static const int PageRole = 0x000f0001;
        static const int PageLabelRole = 0x000f0002;

        void paint( QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const Q_DECL_OVERRIDE;

    protected:
        void drawDisplay( QPainter *painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const Q_DECL_OVERRIDE;

    private:
        class Private;
        Private * const d;
};

#endif
