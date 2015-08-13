/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tags.h"
#include "core/generator.h"

#include <poppler-structtree.h>

QVariant PopplerTagsProxy::data( const QModelIndex & proxyIndex, int role) const
{
    switch( role )
    {
	case Okular::Generator::TagsItemBoundingRects:
	    return sourceModel()->data( proxyIndex, Poppler::StructTreeModel::ItemBoundingRects );
	default:
	    return sourceModel()->data( proxyIndex, role );
    }
}
