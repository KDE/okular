/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OOO_DOCUMENT_H
#define OOO_DOCUMENT_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace OOO {

class Document
{
  public:
    Document( const QString &fileName );

    bool open();

    QByteArray content() const;
    QByteArray meta() const;
    QByteArray styles() const;

  private:
    QString mFileName;
    QByteArray mContent;
    QByteArray mMeta;
    QByteArray mStyles;
};

}

#endif
