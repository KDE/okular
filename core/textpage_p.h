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

#include <QtCore/QMap>
#include <QtGui/QMatrix>

#include "textpage.h"

class SearchPoint;

namespace Okular
{

class PagePrivate;

class TextPagePrivate
{
    public:
        TextPagePrivate( const TextEntity::List &words );
        ~TextPagePrivate();

        RegularAreaRect * findTextInternalForward( int searchID, const QString &query,
                                                   Qt::CaseSensitivity caseSensitivity,
                                                   const TextEntity::List::ConstIterator &start,
                                                   const TextEntity::List::ConstIterator &end );
        RegularAreaRect * findTextInternalBackward( int searchID, const QString &query,
                                                    Qt::CaseSensitivity caseSensitivity,
                                                    const TextEntity::List::ConstIterator &start,
                                                    const TextEntity::List::ConstIterator &end );

        TextEntity::List m_words;
        QMap< int, SearchPoint* > m_searchPoints;
        PagePrivate *m_page;
};

}

#endif
