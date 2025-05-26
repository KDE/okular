/*
    SPDX-FileCopyrightText: 2008 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MOBI_CONVERTER_H
#define MOBI_CONVERTER_H

#include <core/document.h>
#include <core/textdocumentgenerator.h>

#include "mobidocument.h"
#include <qmobipocket/mobipocket.h>

namespace Mobi
{
class Converter : public Okular::TextDocumentConverter
{
    Q_OBJECT
public:
    Converter();
    ~Converter() override;

    QTextDocument *convert(const QString &fileName) override;

private:
#if QMOBIPOCKET_VERSION_MAJOR >= 3
    void handleMetadata(const QMap<Mobipocket::Document::MetaKey, QVariant> &metadata);
#else
    void handleMetadata(const QMap<Mobipocket::Document::MetaKey, QString> &metadata);
#endif
};
}

#endif
