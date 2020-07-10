/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _TXT_DOCUMENT_H_
#define _TXT_DOCUMENT_H_

#include <QTextDocument>

namespace Txt
{
class Document : public QTextDocument
{
    Q_OBJECT

public:
    explicit Document(const QString &fileName);
    ~Document() override;

private:
    QString toUnicode(const QByteArray &array);
};
}

#endif
