/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TETXTPAGE_H_
#define _OKULAR_TETXTPAGE_H_


#include <QtCore/QMap>
#include <QtCore/QStringList>

#include "area.h"

namespace Okular {

class TextSelection;

/*! @enum SearchDirection
 *  The enum holding the direction of searching.
 *! @enum SearchDirection FromTop
 *  Searching from top of the page, next result is to be found,
 *  there was no earlier search result.
 *! @enum SearchDirection FromBottom
 *  Searching from bottom of the page, next result is to be found,
 *  there was no earlier search result.
 *! @enum SearchDirection NextResult
 *  Searching for the next result on the page, earlier result should be 
 *  located so we search from the last result not from the beginning of the 
 *  page.
 *! @enum SearchDirection PreviousResult
 *  Searching for the previous result on the page, earlier result should be 
 *  located so we search from the last result not from the beginning of the 
 *  page.
 */
typedef enum SearchDirection{ FromTop, FromBottom, NextResult, PreviousResult };

/*! @struct  TextEntity
 * @short Abstract textentity of Okular
 * @par The context
 * A document can provide different forms of information about textual representation
 * of its contents. It can include information about positions of every character on the 
 * page, this is the best possibility. 
 * 
 * But also it can provide information only about positions of every word on the page (not the character). 
 * Furthermore it can provide information only about the position of the whole page's text on the page.
 * 
 * Also some document types have glyphes - sets of characters rendered as one, so in search they should 
 * appear as a text but are only one character when drawn on screen. We need to allow this.
 * @par The idea
 * We need several
 */

struct TextEntity
{
    // 
    QString txt;
    NormalizedRect* area;
    TextEntity(QString text, NormalizedRect* ar) : txt(text) 
        { area=ar; };
    ~TextEntity() { delete area; };
};

struct SearchPoint;

class TextPage
{
  public:
    RegularAreaRect* findText( int id, const QString &query, SearchDirection & direct,
                               Qt::CaseSensitivity caseSensitivity, const RegularAreaRect *area);
    QString getText(const RegularAreaRect *rect) const;
    RegularAreaRect * getTextArea ( TextSelection* ) const;
    TextPage(QList<TextEntity*> words) : m_words(words) {};
    TextPage() : m_words() {};
    void append(QString txt, NormalizedRect*  area) 
        { m_words.append(new TextEntity(txt,area) ); };
    ~TextPage();
  private:
    RegularAreaRect * findTextInternalForward(int searchID, const QString &query,
        Qt::CaseSensitivity caseSensitivity, const QList<TextEntity*>::ConstIterator &start,
        const QList<TextEntity*>::ConstIterator &end);
    QList<TextEntity*>  m_words;
    QMap<int, Okular::SearchPoint*> m_searchPoints;
};

}

#endif 
