/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include <QtGui/QTextDocument>

namespace Txt
{
    class Document : public QTextDocument
    {
        public:
            Document( const QString &fileName );
            ~Document();

        private:
            // TODO: write a better detecter, based on some number of chunks
            QByteArray detectEncoding( const QByteArray &array );
            QString toUnicode( const QByteArray &array );
    };
}
