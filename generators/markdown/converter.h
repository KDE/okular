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

class QTextBlock;
class QTextFrame;

namespace Markdown {

class Converter : public Okular::TextDocumentConverter
{
    Q_OBJECT

    public:
        Converter();
        ~Converter();

        QTextDocument *convert( const QString &fileName ) override;

        void convertAgain();

        QTextDocument *convertOpenFile();

    private:
        void extractLinks(QTextFrame *parent);
        void extractLinks(const QTextBlock& parent);
        void convertImages(QTextFrame *parent, const QDir &dir, QTextDocument *textDocument);
        void convertImages(const QTextBlock& parent, const QDir &dir, QTextDocument *textDocument);

        FILE *m_markdownFile;
        QDir m_fileDir;
};

}

#endif
