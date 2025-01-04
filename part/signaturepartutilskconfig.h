/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef SIGNATUREPARTUTILSKCONFIG_H
#define SIGNATUREPARTUTILSKCONFIG_H

#include <QString>

namespace SignaturePartUtils
{
static inline QString ConfigGroup()
{
    return QStringLiteral("Signature");
}
static inline QString ConfigBackgroundKey()
{
    return QStringLiteral("RecentBackgrounds");
}
static inline QString ConfigLastReason()
{
    return QStringLiteral("Reason");
}
static inline QString ConfigLastLocation()
{
    return QStringLiteral("Location");
}
static inline QString ConfigLastKeyNick()
{
    return QStringLiteral("KeyNick");
}
}

#endif // SIGNATUREPARTUTILSKCONFIG_H
