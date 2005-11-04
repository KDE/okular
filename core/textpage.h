/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_TETXTPAGE_H_
#define _KPDF_TETXTPAGE_H_


#include <qvaluelist.h>
#include <qstringlist.h>
#include "area.h"

/*! @enum SearchDir
 *  The enum holding the direction of searching.
 *! @enum SearchDir FromTop
 *  Searching from top of the page, next result is to be found,
 *  there was no earlier search result.
 *! @enum SearchDir NextRes
 *  Searching for the next result on the page, earlier result should be 
 *  located so we search from the last result not from the beginning of the 
 *  page.
 *! @enum SearchDir PrevRes
 *  Searching for the previous result on the page, earlier result should be 
 *  located so we search from the last result not from the beginning of the 
 *  page.
 */
typedef enum SearchDir{ FromTop, NextRes, PrevRes };

/*! @struct  KPDFTextEntity
 * @short Abstract textentity of KPDF
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

struct KPDFTextEntity
{
    // 
    QString txt;
    NormalizedRect* area;
    double baseline;
    int rotation;
    // after this entity we have an end of line
    bool eol;
    KPDFTextEntity(QString text, NormalizedRect* ar, double base,
        int rot,bool eoline) : txt(text),baseline(base),rotation(rot),eol(eoline)
        { area=ar; };
    ~KPDFTextEntity() { delete area; };
};

class KPDFTextPage {
  public:
    RegularAreaRect* findText(const QString &query, SearchDir & direct, 
        const bool &strictCase, const RegularAreaRect *area);
    QString * getText(const RegularAreaRect *rect);
    KPDFTextPage(QValueList<KPDFTextEntity*> words) : m_words(words) {};
    KPDFTextPage() : m_words() {};
    void append(QString txt, NormalizedRect*  area, 
                double bline, int rot, bool eoline) 
        { m_words.append(new KPDFTextEntity(txt,area,bline,rot,eoline) ); };
    ~KPDFTextPage();
  private:
    RegularAreaRect * findTextInternal(const QString &query, bool forward,
        bool strictCase, const QValueList<KPDFTextEntity*>::Iterator &start, const QValueList<KPDFTextEntity*>::Iterator &end);
    QValueList<KPDFTextEntity*>  m_words;
  };

#endif 
