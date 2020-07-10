/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
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
    void handleMetadata(const QMap<Mobipocket::Document::MetaKey, QString> &metadata);
};
}

#endif
