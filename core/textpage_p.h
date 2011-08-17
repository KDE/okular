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
        /**
         * Prints a line from m_lines
         */
        void printTextList(int i, TextList list);

        /**
         * Copy a TextList to m_words
         */
        void copyTo(TextList &list);

        /**
         * Copy m_words to a TextList
         */
        void copyFrom(TextList &list);

        /**
         * Print the textpage contents with area (text and bounding rect)
         */
        void printTextPageContent();

        /**
         * Remove odd spaces which are much bigger than normal spaces from m_words
         */
        void removeSpace();

        /**
         * Create words from characters
         */
        void makeWordFromCharacters();

        /**
         * Create lines from TextList and sort them according to their position
         */
        void makeAndSortLines(TextList &words,SortedTextList &lines,LineRect &line_rects);

        /**
         * Caluclate statistical info like, word spacing, column spacing, line spacing from the Lines
         * we made
         */
        void calculateStatisticalInformation(SortedTextList &lines, LineRect line_rects,int& word_spacing,
                                             int& line_spacing, int& column_spacing);

        /**
         * Functions necessary for document file segmentation into text regions for document layout
         * analysis.
         */
        void XYCutForBoundingBoxes(int tcx,int tcy);


        /**
         * Add additional spaces between words, if necessary, which can make the words valuable
         * while copying after selection
         */
        void addNecessarySpace();

        /**
         * Break the words into characters, so the text selection wors fine
         */
        void breakWordIntoCharacters();



        // variables those can be accessed directly from TextPage
        QMap<int, RegionText> m_word_chars_map;
        RegionTextList m_XY_cut_tree;
        TextList m_spaces;
        TextList m_words;
        QMap< int, SearchPoint* > m_searchPoints;
        PagePrivate *m_page;
        SortedTextList m_lines;
        LineRect m_line_rects;
};

}

#endif
