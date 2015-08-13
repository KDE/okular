/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_TAGS_H_
#define _OKULAR_GENERATOR_PDF_TAGS_H_

#include <qidentityproxymodel.h>

class PopplerTagsProxy : public QIdentityProxyModel
{
    virtual QVariant data( const QModelIndex & proxyIndex, int role = Qt::DisplayRole ) const;
};

#endif
