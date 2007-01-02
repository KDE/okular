/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QString>

#include "sourcereference.h"

using namespace Okular;

class SourceReference::Private
{
    public:
        Private()
            : row( 0 ), column( 0 )
        {
        }

        QString filename;
        int row;
        int column;
};

SourceReference::SourceReference( const QString &fileName, int row, int column )
    : d( new Private )
{
    d->filename = fileName;
    d->row = row;
    d->column = column;
}

SourceReference::~SourceReference()
{
    delete d;
}

QString SourceReference::fileName() const
{
    return d->filename;
}

int SourceReference::row() const
{
    return d->row;
}

int SourceReference::column() const
{
    return d->column;
}

