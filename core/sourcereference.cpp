/*
    SPDX-FileCopyrightText: 2007, 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sourcereference.h"
#include "sourcereference_p.h"

#include <KLocalizedString>
#include <QString>
#include <QUrl>

std::optional<Okular::SourceReference> Okular::extractLilyPondSourceReference(const QUrl &url)
{
    // Example URL is: textedit:///home/foo/bar.ly:42:42:42
    // The three numbers are apparently: line:beginning of column:end of column

    if (url.scheme() != QStringLiteral("textedit")) {
        return std::nullopt;
    }

    // There can be more, in case the filename contains :
    if (url.fileName().count(QLatin1Char(':')) < 3) {
        return std::nullopt;
    }

    QStringList parts(url.path().split(QLatin1Char(':')));

    bool ok;
    // Take out the things we need
    int columnEnd = parts.takeLast().toInt(&ok); // apparently we don't use this
    Q_UNUSED(columnEnd);
    if (!ok) {
        return std::nullopt;
    }

    auto column = parts.takeLast().toInt(&ok);
    if (!ok) {
        return std::nullopt;
    }

    auto row = parts.takeLast().toInt(&ok);
    if (!ok) {
        return std::nullopt;
    }

    // In case the path itself contains :, we need to reconstruct it after removing all the numbers
    auto fileName = parts.join(QLatin1Char(':'));
    if (fileName.isEmpty()) {
        return std::nullopt;
    }

    return std::make_optional<SourceReference>(fileName, row, column);
}

QString Okular::sourceReferenceToolTip(const SourceReference &sourceReference)
{
    return i18nc("'source' is a source file", "Source: %1", sourceReference.fileName);
}
