/*
    SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "okularsingleton.h"
#include <KPluginMetaData>
#include <QMimeDatabase>
#include <QMimeType>

OkularSingleton::OkularSingleton() = default;

QStringList OkularSingleton::nameFilters() const
{
    QStringList supportedPatterns;

    const auto plugins = KPluginMetaData::findPlugins(QStringLiteral("okular/generators"));
    if (plugins.isEmpty()) {
        qWarning() << "okularpart plugin not found. Required to get nameFilters";
        return supportedPatterns;
    }

    QMimeDatabase md;
    for (const auto &plugin : plugins) {
        const QStringList mimeTypes = plugin.mimeTypes();

        for (const auto &mimeName : mimeTypes) {
            const QStringList suffixes = md.mimeTypeForName(mimeName).suffixes();
            for (const QString &suffix : suffixes) {
                supportedPatterns += QStringLiteral("*.") + suffix;
            }
        }
    }

    return supportedPatterns;
}
