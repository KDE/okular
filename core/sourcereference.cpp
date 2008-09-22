/***************************************************************************
 *   Copyright (C) 2007,2008 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "sourcereference.h"
#include "sourcereference_p.h"

#include <QtCore/QString>
#include <klocale.h>

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

bool Okular::extractLilyPondSourceReference( const QString &url, QString *file, int *row, int *col )
{
    if ( !url.startsWith( QLatin1String( "textedit://" ) ) )
        return false;

    *row = 0;
    *col = 0;
    int lilyChar = 0;
    typedef int *IntPtr;
    const IntPtr int_data[] = { row, &lilyChar, col };
    int int_index = sizeof( int_data ) / sizeof( int* ) - 1;
    int index_last = -1;
    int index = url.lastIndexOf( QLatin1Char( ':' ), index_last );
    while ( index != -1 && int_index >= 0 )
    {
        // read the current "chunk"
        const QStringRef ref = url.midRef( index + 1, index_last - index - 1 );
        *int_data[ int_index ] = QString::fromRawData( ref.data(), ref.count() ).toInt();
        // find the previous "chunk"
        index_last = index;
        index = url.lastIndexOf( QLatin1Char( ':' ), index_last - 1 );
        --int_index;
    }
    // NOTE: 11 is the length of "textedit://"
    *file = url.mid( 11, index_last != -1 ? index_last - 11 : -1 );
    return true;
}

QString Okular::sourceReferenceToolTip( const QString &source, int row, int col )
{
    Q_UNUSED( row );
    Q_UNUSED( col );
    return i18nc( "'source' is a source file", "Source: %1", source );
}
