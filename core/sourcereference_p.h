/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SOURCEREFERENCE_P_H
#define OKULAR_SOURCEREFERENCE_P_H

class QString;

namespace Okular
{

bool extractLilyPondSourceReference( const QString &url, QString *file, int *row, int *col );
QString sourceReferenceToolTip( const QString &source, int row, int col );

}

#endif
