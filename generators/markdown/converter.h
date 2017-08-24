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

#include <QDir>

#include <core/textdocumentgenerator.h>

class QTextCursor;
class QTextFrame;

namespace Markdown {

class Converter : public Okular::TextDocumentConverter
{
    Q_OBJECT

    public:
        Converter();
        ~Converter();

        QTextDocument *convert( const QString &fileName ) override;

    private:
        void convertLinks();
        void convertImages();
        
        void convertLinks(QTextFrame *parent);
        void convertLinks(QTextBlock& parent);
        void convertImages(QTextFrame *parent);
        void convertImages(QTextBlock& parent);
        
        QTextDocument *mTextDocument;
        QDir mDir;
};

}

#endif
