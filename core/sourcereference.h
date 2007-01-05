/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SOURCEREFERENCE_H
#define OKULAR_SOURCEREFERENCE_H

#include <okular/core/okular_export.h>

class QString;

namespace Okular {

/**
 * @short Defines a source reference
 *
 * A source reference is a reference to one of the source(s) of the loaded
 * document.
 */
class OKULAR_EXPORT SourceReference
{
    public:
        /**
         * Creates a reference to the row @p row and column @p column of the
         * source @p fileName
         */
        SourceReference( const QString &fileName, int row, int column = 0 );

        /**
         * Destroys the source reference.
         */
        ~SourceReference();

        /**
         * Returns the filename of the source.
         */
        QString fileName() const;

        /**
         * Returns the row of the position in the source file.
         */
        int row() const;

        /**
         * Returns the column of the position in the source file.
         */
        int column() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( SourceReference )
};

}

#endif

