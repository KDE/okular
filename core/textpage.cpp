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
    bool haveMatch=false;
    bool dontIncrement=false;
    QValueList<KPDFTextEntity*>::Iterator it;
    // we dont support backward search yet
	for( it=start ; it != end;  (!dontIncrement) ? (++it) : it )
	{
        // a static cast would be faster?
        str=(*it)->txt;
        if (query.mid(j,1)==" " && query.mid(j,1)=="\n")
        {
            // lets match newline as a space
#ifdef DEBUG_TEXTPAGE
            kdDebug(1223) << "newline or space" << endl;
#endif
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
            kdDebug(1223) << str.left(min) << " : " << query.mid(j,min) << endl;
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
#ifdef DEBUG_TEXTPAGE
            kdDebug(1223) << "\tnot matched" << endl;
#endif
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
#ifdef DEBUG_TEXTPAGE
            kdDebug(1223) << "\tmatched" << endl;
#endif
                    haveMatch=true;
                    ret->append( (*it)->area );
                    j+=min;
                    queryLeft-=min;
            }
        }
        if (haveMatch && queryLeft==0 && j==query.length())
        {
//            RegularAreaRect::ConstIterator i=ret->begin(), end=ret->end;
            int end=ret->count(),i=0,x=0;
            QValueList <NormalizedRect*> m_remove;
            for (;i<end;i++)
            {
                if ( i < (end-1) )
                {
                    if ( (*ret)[x]->intersects( (*ret)[i+1] ) )
                    {
                        (*ret)[x]->left=(QMIN((*ret)[x]->left,(*ret)[i+1]->left));
                        (*ret)[x]->top=(QMIN((*ret)[x]->top,(*ret)[i+1]->top));
                        (*ret)[x]->bottom=(QMAX((*ret)[x]->bottom,(*ret)[i+1]->bottom));
                        (*ret)[x]->right=(QMAX((*ret)[x]->right,(*ret)[i+1]->right));
                        m_remove.append( (*ret)[i+1] );
                    }
                    else
                    {
                        x=i;
                   }
                }
            }
            while (!m_remove.isEmpty())
            {
                ret->remove( m_remove.last() );
                m_remove.pop_back();
            }
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
    QValueList<KPDFTextEntity*>::Iterator it,end = m_words.end();
    KPDFTextEntity * last=0;
	for( it=m_words.begin() ; it != end;  ++it )
	{
        // provide the string FIXME?: newline handling
        if (area->intersects((*it)->area))
        {
            *ret += ((*it)->txt);
            last=*it;
        }
	}
    return ret;
}
 
