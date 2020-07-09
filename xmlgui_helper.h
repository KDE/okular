/***************************************************************************
 *   Copyright (C) 2020 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QString>

namespace Okular
{
void removeRCFileIfVersionSmallerThan(const QString &filePath, int version);
}
