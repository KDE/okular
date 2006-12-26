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

#define PAGEITEMDELEGATE_SEPARATOR "@@@@@@@@@@"

class PageItemDelegate : public QItemDelegate
{
    Q_OBJECT

    public:
        PageItemDelegate( QObject * parent = 0 );

    protected:
        virtual void drawDisplay( QPainter *painter, const QStyleOptionViewItem & option, const QRect & rect, const QString & text ) const;
};

#endif
