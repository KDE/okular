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

typedef enum SearchDir{ FromTop, NextRes, PrevRes };

/*
 * @short Abstractet Entity of KPDF
 * The idea:
 * Document can provide one of the following information about text areas
 * 1.) provide information for every character  (best)

 * 
 * 2.) provide information for every word 
 * We will use type Word for this kind of entity. It will store the 
 * word and should be followed by a type glyph which will contain some 
 * punctuation marks.
 
 
 * 3.) provide information for every textline
 */

struct KPDFTextEntity
{
// not sure used now might be useful in the future
//    typedef enum Type{ Glyph, Word, Line };
//    Type type;
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
