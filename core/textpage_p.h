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
class RegionText;

namespace Okular
{

class PagePrivate;
typedef QList< TinyTextEntity* > TextList;

typedef bool ( *TextComparisonFunction )( const QStringRef & from, const QStringRef & to,
                                          int *fromLength, int *toLength );

//mamun.nightcrawler@gmail.com

/**
We will make a line of TextList and also store the bounding rectangle of line
**/
typedef QList<TextList> SortedTextList;
typedef QList<QRect> LineRect;

/** list of RegionText -- keeps a bunch of TextList with their bounding rectangles **/
typedef QList<RegionText> RegionTextList;

class TextPagePrivate
{
    public:
        TextPagePrivate();
        ~TextPagePrivate();

        RegularAreaRect * findTextInternalForward( int searchID, const QString &query,
                                                   Qt::CaseSensitivity caseSensitivity,
                                                   TextComparisonFunction comparer,
                                                   const TextList::ConstIterator &start,
                                                   const TextList::ConstIterator &end );
        RegularAreaRect * findTextInternalBackward( int searchID, const QString &query,
                                                    Qt::CaseSensitivity caseSensitivity,
                                                    TextComparisonFunction comparer,
                                                    const TextList::ConstIterator &start,
                                                    const TextList::ConstIterator &end );
        /** prints a line from m_lines **/
        void printTextList(int i, TextList list);

        /** copy a TextList to m_words **/
        void copy(TextList &list);

        /**
        add additional spaces between words, if necessary, which can make the words valuable
        while copying after selection
        **/
        void addNecessarySpace(RegionTextList tree);


        QMap<int, RegionText> m_word_chars_map;
        RegionTextList m_region_words;
        TextList m_spaces;
        TextList m_words;
        TextList m_tmp_words;
        QMap< int, SearchPoint* > m_searchPoints;
        PagePrivate *m_page;
        SortedTextList m_lines;
        LineRect m_line_rects;
};

}

#endif
