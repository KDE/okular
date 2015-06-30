/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "layersitemdelegate.h"

#include <kicon.h>
#include <QPainter>
#include <QDebug>

Q_DECLARE_METATYPE( Qt::CheckState );

LayersItemDelegate::LayersItemDelegate( QWidget * parent )
    : QStyledItemDelegate( parent )
{

}

void LayersItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Qt::CheckState state = (Qt::CheckState ) qvariant_cast<int>( index.data( Qt::CheckStateRole ) );
    if( option.state & QStyle::State_Selected )
	painter->fillRect( option.rect, option.palette.highlight() );
    KIcon icon = state == Qt::Checked ? KIcon( "layer-visible-on" ) : KIcon( "layer-visible-off" );
    QRect iconRect = option.rect;
    iconRect.setWidth( iconRect.height() );
    painter->drawPixmap( iconRect, icon.pixmap( iconRect.size() ) );

    QString text = qvariant_cast<QString>( index.data( Qt::DisplayRole ) );
    QRect textRect = QRect( iconRect.topRight() + QPoint( 5, 0 ), QSize( option.rect.width() - iconRect.width(), option.rect.height() ) );
    painter->drawText( textRect, Qt::AlignVCenter, text );
}