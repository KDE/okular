/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef EPUB_CONVERTER_H
#define EPUB_CONVERTER_H

#include <core/document.h>
#include <core/textdocumentgenerator.h>

#include "epubdocument.h"

namespace Epub
{
class Converter : public Okular::TextDocumentConverter
{
    Q_OBJECT

public:
    Converter();
    ~Converter() override;

    QTextDocument *convert(const QString &fileName) override;

private:
    void _emitData(Okular::DocumentInfo::Key key, enum epub_metadata type);
    void _handle_anchors(const QTextBlock &start, const QString &name);
    void _insert_local_links(const QString &key, const QPair<int, int> value);
    EpubDocument *mTextDocument;

    QHash<QString, QTextBlock> mSectionMap;
    QHash<QString, QVector<QPair<int, int>>> mLocalLinks;
};
}

#endif
