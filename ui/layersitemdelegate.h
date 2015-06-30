/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_LAYERSITEMDELEGATE_H_
#define _OKULAR_LAYERSITEMDELEGATE_H_

#include <QStyledItemDelegate>

#include "okular_part_export.h"

class OKULAR_PART_EXPORT LayersItemDelegate : public QStyledItemDelegate
{
Q_OBJECT
    public:
	LayersItemDelegate( QWidget * parent);

	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

#endif