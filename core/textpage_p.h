/***************************************************************************
 *   Copyright (C) 2006      by Tobias Koenig <tokoe@kde.org>              *
 *   Copyright (C) 2007      by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TEXTPAGE_P_H_
#define _OKULAR_TEXTPAGE_P_H_

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtGui/QMatrix>

class SearchPoint;
class TinyTextEntity;

namespace Okular
{

class PagePrivate;
typedef QList< TinyTextEntity* > TextList;

class TextPagePrivate
{
    public:
        TextPagePrivate();
        ~TextPagePrivate();

        RegularAreaRect * findTextInternalForward( int searchID, const QString &query,
                                                   Qt::CaseSensitivity caseSensitivity,
                                                   const TextList::ConstIterator &start,
                                                   const TextList::ConstIterator &end );
        RegularAreaRect * findTextInternalBackward( int searchID, const QString &query,
                                                    Qt::CaseSensitivity caseSensitivity,
                                                    const TextList::ConstIterator &start,
                                                    const TextList::ConstIterator &end );

        TextList m_words;
        QMap< int, SearchPoint* > m_searchPoints;
        PagePrivate *m_page;
};

}

#endif
