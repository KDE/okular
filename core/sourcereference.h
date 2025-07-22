/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SOURCEREFERENCE_H
#define OKULAR_SOURCEREFERENCE_H

#include "okularcore_export.h"

#include <QObject>

namespace Okular
{
/**
 * @short Defines a source reference
 *
 * A source reference is a reference to one of the source(s) of the loaded
 * document.
 */
struct OKULARCORE_EXPORT SourceReference final {
    /**
     * Creates a reference to the row @p row and column @p column of the
     * source @p fileName
     */
    SourceReference(const QString &fileName, int row, int column = 0)
        : fileName(fileName)
        , row(row)
        , column(column)
    {
    }

    QString fileName;
    int row = 0;
    int column = 0;
};

}

#endif
