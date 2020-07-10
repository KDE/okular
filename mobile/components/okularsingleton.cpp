/*
 *   Copyright 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
