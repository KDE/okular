/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "extensions.h"

// local includes
#include "part.h"

#include <KParts/Part>

namespace Okular
{
/*
 * BrowserExtension class
 */
BrowserExtension::BrowserExtension(Part *parent)
    : NavigationExtension(parent)
    , m_part(parent)
{
    Q_EMIT enableAction("print", true);
    setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
    m_part->slotPrint();
}

}

#include "moc_extensions.cpp"

/* kate: replace-tabs on; indent-width 4; */
