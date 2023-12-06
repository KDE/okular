/*
    SPDX-FileCopyrightText: 2013 Azat Khuzhin <a3at.mail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "document.h"

#include <QDataStream>
#include <QFile>

#include <QDebug>
#include <QStringDecoder>

#include "debug_txt.h"

using namespace Txt;

Document::Document(const QString &fileName)
{
#ifdef TXT_DEBUG
    qCDebug(OkularTxtDebug) << "Opening file" << fileName;
#endif

    QFile plainFile(fileName);
    if (!plainFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(OkularTxtDebug) << "Can't open file" << plainFile.fileName();
        return;
    }

    const QByteArray buffer = plainFile.readAll();
    setPlainText(toUnicode(buffer));
}

Document::~Document()
{
}

QString Document::toUnicode(const QByteArray &array)
{
    auto encoding = QStringConverter::encodingForHtml(array);
    QStringDecoder decoder {encoding.value_or(QStringConverter::Encoding::Utf8)};
    return decoder.decode(array);
}

Q_LOGGING_CATEGORY(OkularTxtDebug, "org.kde.okular.generators.txt", QtWarningMsg)
