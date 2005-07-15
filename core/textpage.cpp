/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "textpage.h"
#include "area.h"
#include <kdebug.h>

KPDFTextPage::~KPDFTextPage()
{
    QValueList<KPDFTextEntity*>::Iterator it;
    for (it=m_words.begin();it!=m_words.end();++it)
    {
        delete (*it);
    }
}

RegularAreaRect* KPDFTextPage::findText(const QString &query, SearchDir & direct, 
const bool &strictCase, const RegularAreaRect *area)
{
    SearchDir dir=direct;
    // invalid search request
    if (query.isEmpty() || (area->isNull() && dir!=FromTop))
        return 0;
    QValueList<KPDFTextEntity*>::Iterator start;
    if (dir == FromTop)
    {
        start=m_words.begin();
        dir=NextRes;
    }
    else
    {

        // locate the current search position 
        QString * str=0;
        int j=0, len=0, queryLeft=query.length()-1;
        bool haveMatch=false;
        QValueList<KPDFTextEntity*>::Iterator  it;
        for( it=m_words.begin() ; it != m_words.end();  ++it )
        {
            str= &((*it)->txt);
                continue;
            len=str->length()-1;
            // we have equal (or less then) area of the query left as the lengt of the current 
            // entity
            if (queryLeft<=len)
            {
                if (((strictCase) 
                ? (*str != query) 
                : (str->lower() != query.lower() ))
                || (!(area->intersects((*it)->area))))
                {
                    // we not have matched
                    // this means we dont have a complete match
                    // we need to get back to query start
                    // and continue the search from this place
                    haveMatch=false;
                    j=0;
                    queryLeft=query.length()-1;
                }
                else
                {
                    // we have a match
                    // move the current position in the query
                    // to the position after the length of this string
                    // we matched
                    // substract the length of the current entity from 
                    // the left length of the query
                    haveMatch=true;
                    j+=QMIN(queryLeft,len);
                    queryLeft-=QMIN(queryLeft,len);
                }
            }
            if (haveMatch && queryLeft==0 && j==query.length()-1)
                break;
        }
        start=it;
    }
    switch (dir)
    {
        case PrevRes:
            return findTextInternal(query,false, strictCase, start,m_words.begin());
            break;
        case FromTop:
        case NextRes:
            return findTextInternal(query,true, strictCase, start,m_words.end());
            break;
    }
    return 0;
}


RegularAreaRect* KPDFTextPage::findTextInternal(const QString &query, bool forward,
        bool strictCase, const QValueList<KPDFTextEntity*>::Iterator &start, 
        const QValueList<KPDFTextEntity*>::Iterator &end)
{

	RegularAreaRect* ret=new RegularAreaRect;
    
    // j is the current position in our query
    // len is the length of the string in kpdftextentity
    // queryLeft is the length of the query we have left
    QString str;
    int j=0, len=0, queryLeft=query.length();
    bool haveMatch=false,wasEol=false;
    bool dontIncrement=false;
    QValueList<KPDFTextEntity*>::Iterator it;
    // we dont support backward search yet
	for( it=start ; it != end;  (!dontIncrement) ? (++it) : it )
	{
        // a static cast would be faster?
        str=(*it)->txt;
        if (query.mid(j,1)==" " && wasEol )
        {
            // lets match newline as a space
            //kdDebug(1223) << "Matched, space after eol " << endl;
            j++;
            queryLeft--;
            // since we dont really need to increment this after this
            // run of the loop finishes because we are not comparing it 
            // to any entity, rather we are deducing a situation in a document
            dontIncrement=true;
        }
        else
        {
            dontIncrement=false;
            int min=QMIN(queryLeft,len);
            len=str.length();
            //kdDebug(1223) << str.left(min) << " : " << query.mid(j,min) << endl;
            // we have equal (or less then) area of the query left as the lengt of the current 
            // entity
    
            if ((strictCase)
                ? (str.left(min) != query.mid(j,min))
                : (str.left(min).lower() != query.mid(j,min).lower())
                )
            {
                    // we not have matched
                    // this means we dont have a complete match
                    // we need to get back to query start
                    // and continue the search from this place
                    haveMatch=false;
                    delete ret;
                    ret=new RegularAreaRect;
                   // kdDebug(1223) << "\tNotmatched" << endl;
                    j=0;
                    queryLeft=query.length();
            }
            else
            {
                    // we have a match
                    // move the current position in the query
                    // to the position after the length of this string
                    // we matched
                    // substract the length of the current entity from 
                    // the left length of the query
                  //  kdDebug(1223) << "\tMatched" << endl;
                    haveMatch=true;
                    ret->append( (*it)->area );
                    j+=min;
                    queryLeft-=min;
            }
        }
        wasEol=(*it)->eol;
        /*if (wasEol)
            kdDebug(1223) << "\t\tENDOFLINE"  << endl;*/
        if (haveMatch && queryLeft==0 && j==query.length())
        {
            if (!ret) kdDebug(1223) << "\t\tempty return area, sth very wrong here"  << endl;
            if (ret->isEmpty()) kdDebug(1223) << "\t\tempty return area"  << endl;
            if (ret->isNull()) kdDebug(1223) << "\t\tinvalid return area"  << endl;
            return ret;
        }
	}
    return 0;
}

QString * KPDFTextPage::getText(const RegularAreaRect *area)
{
    if (area->isNull())
        return 0;

    QString* ret = new QString;
    QValueList<KPDFTextEntity*>::Iterator it;
    KPDFTextEntity * last=0;
	for( it=m_words.begin() ; it != m_words.end();  ++it )
	{
        // provide the string FIXME: newline handling
        if (area->intersects((*it)->area))
        {
            *ret += ((*it)->txt);
            if (last)
            {
                if ( (last -> baseline != (*it)-> baseline )
                     && ((*it)->rotation % 2 == 0)
                     && ((*it)->rotation == last->rotation)
                   )
                {
                    // on different  baseline, on a vertical rotation (0 or 180)
                        *ret+='\n';
                }
                // what if horizontal? how do i determine end of line?
                /*if (((*it)->rotation == (*last)->rotation)
                    && ((*it)->rotation % 2 == 1)
                    && ((*
                    {
                        // 90 or 270 degrees rotation
                        // maybe add a tabulation?
                    }

                }*/
            }
            last=*it;
        }
	}
    return ret;
}
 