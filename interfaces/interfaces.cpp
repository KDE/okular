/***************************************************************************
 *   Copyright (C) 2020 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "configinterface.h"
#include "guiinterface.h"
#include "printinterface.h"
#include "saveinterface.h"
#include "viewerinterface.h"

Okular::ConfigInterface::~ConfigInterface() = default;
Okular::GuiInterface::~GuiInterface() = default;
Okular::PrintInterface::~PrintInterface() = default;
Okular::SaveInterface::~SaveInterface() = default;
Okular::ViewerInterface::~ViewerInterface() = default;
