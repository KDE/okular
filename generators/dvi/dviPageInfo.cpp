/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dviPageInfo.h"

dviPageInfo::dviPageInfo()
{
    sourceHyperLinkList.reserve(200);
}

dviPageInfo::~dviPageInfo()
{
}

void dviPageInfo::clear()
{
    sourceHyperLinkList.clear();
}
