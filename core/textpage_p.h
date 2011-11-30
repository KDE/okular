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

/**
 * Make a line of TextList and store the bounding rectangle of line
 */
typedef QList<TextList> SortedTextList;
typedef QList<QRect> LineRect;

/**
 * A list of RegionText. It keeps a bunch of TextList with their bounding rectangles
 */
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

        /**
         * Copy a TextList to m_words, the pointers of list are adopted
         */
        void setWordList(const TextList &list);

        /**
         * Copy m_words to a new TextList, it is the caller responsability to free the pointers
         */
        TextList duplicateWordList() const;

        /**
         * Make necessary modifications in the TextList to make the text order correct, so
         * that textselection works fine
         */
        void correctTextOrder();

        /**
         * Remove odd spaces which are much bigger than normal spaces from m_words
         */
        void removeSpace();

        /**
         * Create words from characters
         */
        QHash<QRect, RegionText> makeWordFromCharacters();

        /**
         * Create lines from TextList and sort them according to their position
         */
        void makeAndSortLines(const TextList &words, SortedTextList *lines, LineRect *line_rects) const;

        /**
         * Caluclate statistical info like, word spacing, column spacing, line spacing from the Lines
         * we made
         */
        void calculateStatisticalInformation(const SortedTextList &lines, const LineRect &line_rects, int *word_spacing,
                                             int *line_spacing, int *column_spacing) const;

        /**
         * Functions necessary for document file segmentation into text regions for document layout
         * analysis.
         */
        RegionTextList XYCutForBoundingBoxes(int tcx, int tcy);

        /**
         * Add additional spaces between words, if necessary, which can make the words valuable
         * while copying after selection
         */
        void addNecessarySpace(RegionTextList tree);

        /**
         * Break the words into characters, so the text selection wors fine
         */
        void breakWordIntoCharacters(const QHash<QRect, RegionText> &words_char_map);

        // variables those can be accessed directly from TextPage
        TextList m_words;
        QMap< int, SearchPoint* > m_searchPoints;
        PagePrivate *m_page;
};

}

#endif
