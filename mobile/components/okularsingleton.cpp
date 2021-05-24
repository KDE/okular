/*
    SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "okularsingleton.h"
#include <KServiceTypeTrader>
#include <QMimeDatabase>
#include <QMimeType>

OkularSingleton::OkularSingleton()
{
}

QStringList OkularSingleton::nameFilters() const
{
    QStringList supportedPatterns;

    QString constraint(QStringLiteral("(Library == 'okularpart')"));
    QLatin1String basePartService("KParts/ReadOnlyPart");
    KService::List offers = KServiceTypeTrader::self()->query(basePartService, constraint);
    KService::List::ConstIterator it = offers.constBegin(), itEnd = offers.constEnd();

    QMimeDatabase md;
    for (; it != itEnd; ++it) {
        KService::Ptr service = *it;
        const QStringList mimeTypes = service->mimeTypes();

        for (const auto &mimeName : mimeTypes) {
            for (const auto &suffix : md.mimeTypeForName(mimeName).suffixes())
                supportedPatterns += QStringLiteral("*.") + suffix;
        }
    }

    return supportedPatterns;
}
