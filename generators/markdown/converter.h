/***************************************************************************
 *   Copyright (C) 2017 by Julian Wolff <wolff@julianwolff.de>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MARKDOWN_CONVERTER_H
#define MARKDOWN_CONVERTER_H

#include <core/textdocumentgenerator.h>

#include <QDir>
#include <QTextFragment>

class QTextBlock;
class QTextFrame;

namespace Markdown
{
class Converter : public Okular::TextDocumentConverter
{
    Q_OBJECT

public:
    Converter();
    ~Converter() override;

    QTextDocument *convert(const QString &fileName) override;

    void convertAgain();

    void setFancyPantsEnabled(bool b)
    {
        m_isFancyPantsEnabled = b;
    }
    bool isFancyPantsEnabled() const
    {
        return m_isFancyPantsEnabled;
    }

    QTextDocument *convertOpenFile();

private:
    void extractLinks(QTextFrame *parent, QHash<QString, QTextFragment> &internalLinks, QHash<QString, QTextBlock> &documentAnchors);
    void extractLinks(const QTextBlock &parent, QHash<QString, QTextFragment> &internalLinks, QHash<QString, QTextBlock> &documentAnchors);
    void convertImages(QTextFrame *parent, const QDir &dir, QTextDocument *textDocument);
    void convertImages(const QTextBlock &parent, const QDir &dir, QTextDocument *textDocument);
    void setImageSize(QTextImageFormat &format, const qreal specifiedWidth, const qreal specifiedHeight, const qreal originalWidth, const qreal originalHeight);

    FILE *m_markdownFile;
    QDir m_fileDir;
    bool m_isFancyPantsEnabled;
};

}

#endif
