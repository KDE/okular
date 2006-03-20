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
#include "misc.h"
#include <kdebug.h>

KPDFTextPage::~KPDFTextPage()
{
    QLinkedList<KPDFTextEntity*>::Iterator it;
    for (it=m_words.begin();it!=m_words.end();++it)
    {
        delete (*it);
    }
}

RegularAreaRect * KPDFTextPage::getTextArea ( TextSelection * sel) const
{
/**
  It works like this:
  There are two cursors, we need to select all the text between them. The coordinates are normalised, leftTop is (0,0)
  rightBottom is (1,1), so for cursors start (sx,sy) and end (ex,ey) we start with finding text rectangles under those 
  points, if not we search for the first that is to the right to it in the same baseline, if none found, then we search
  for the first rectangle with a baseline under the cursor, having two points that are the best rectangles to both 
  of the cursors: (rx,ry)x(tx,ty) for start and (ux,uy)x(vx,vy) for end, we do a 
  1. (rx,ry)x(1,ty)
  2. (0,ty)x(1,uy)
  3. (0,uy)x(vx,vy)

  To find the closest rectangle to cursor (cx,cy) we search for a rectangle that either contains the cursor
  or that has a left border >= cx and bottom border >= cy. 
*/
	RegularAreaRect * ret= new RegularAreaRect;
	int it=-1,itB=-1,itE=-1;
//         if (sel->itB==-1)
          
        // ending cursor is higher then start cursor, we need to find positions in reverse
        NormalizedRect *tmp=0,*start=0,*end=0;
        const NormalizedPoint * startC=sel->start();
        const NormalizedPoint * endC=sel->end();
        if (sel->dir() == 1 || (sel->itB()==-1 && sel->dir()==0))
        {
          kWarning() << "running first loop\n";
          for (it=0;it<m_words.count();it++)
          {
              tmp=m_words[it]->area;
              if (tmp->contains(startC->x,startC->y) 
                  || ( tmp->top <= startC->y && tmp->bottom >= startC->y && tmp->left >= startC->x )
                  || ( tmp->top >= startC->y))
              {
                  /// we have found the (rx,ry)x(tx,ty)   
                  itB=it;
                  kWarning() << "start is " << itB << " count is " << m_words.count() << endl;
                  break;
              }
  
          }
          sel->itB(itB);
        }
        itB=sel->itB();
        kWarning() << "direction is " << sel->dir() << endl;
        kWarning() << "reloaded start is " << itB << " against " << sel->itB() << endl;
        if (sel->dir() == 0 || (sel->itE() == -1 && sel->dir()==1))
        {
          kWarning() << "running second loop\n";
          for (it=m_words.count()-1; it>=itB;it--)
          {
              tmp=m_words[it]->area;
              if (tmp->contains(endC->x,endC->y) 
                  || ( tmp->top <= endC->y && tmp->bottom >= endC->y && tmp->right <= endC->x )
                  || ( tmp->bottom <= endC->y))
              {
                  /// we have found the (ux,uy)x(vx,vy)   
                  itE=it;
                  kWarning() << "ending is " << itE << " count is " << m_words.count() << endl;
                  kWarning () << "conditions " << tmp->contains(endC->x,endC->y) << " " 
                    << ( tmp->top <= endC->y && tmp->bottom >= endC->y && tmp->right <= endC->x ) << " " <<
                    ( tmp->top >= endC->y) << endl;

                  break;
              }
          }
          sel->itE(itE);
        }
        kWarning() << "reloaded ending is " << itE << " against " << sel->itE() << endl;

        if (sel->itB()!=-1 && sel->itE()!=-1)
        {
          start=m_words[sel->itB()]->area;
          end=m_words[sel->itE()]->area;

          NormalizedRect first,second,third;/*
          first.right=1;
          /// if (rx,ry)x(1,ty) intersects the end cursor, there is only one line
          bool sameBaseline=end->intersects(first);
          kWarning() << "sameBaseline : " << sameBaseline << endl;
          if (sameBaseline)
          {
              first=*start;
              first.right=end->right;
              first.bottom=end->bottom;
              for (it=QMIN(sel->itB(),sel->itE()); it<=QMAX(sel->itB(),sel->itE());it++)
              {
                tmp=m_words[it]->area;
                if (tmp->intersects(&first))
                  ret->append(tmp);
              }
          }
          else*/
          /// finding out if there are more then one baseline between them is a hard and discussable task
          /// we will create a rectangle (rx,0)x(tx,1) and will check how many times does it intersect the 
          /// areas, if more than one -> we have a three or over line selection
//           {
            first=*start;
            second.top=start->bottom;
            first.right=second.right=1;
            third=*end;
            third.left=second.left=0;
            second.bottom=end->top;
            for (it=QMIN(sel->itB(),sel->itE()); it<=QMAX(sel->itB(),sel->itE());it++)
            {
                tmp=m_words[it]->area;
                if (tmp->intersects(&first) || tmp->intersects(&second) || tmp->intersects(&third))
                  ret->append(tmp);
            }

//           }
        }

        ret->simplify();
	return ret;
}


RegularAreaRect* KPDFTextPage::findText(const QString &query, SearchDir & direct, 
const bool &strictCase, const RegularAreaRect *area)
{
    SearchDir dir=direct;
    // invalid search request
    if (query.isEmpty() || (area->isNull() && dir!=FromTop))
        return 0;
    QLinkedList<KPDFTextEntity*>::Iterator start;
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
        QLinkedList<KPDFTextEntity*>::Iterator  it;
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
        bool strictCase, const QLinkedList<KPDFTextEntity*>::Iterator &start, 
        const QLinkedList<KPDFTextEntity*>::Iterator &end)
{

    RegularAreaRect* ret=new RegularAreaRect;

    // j is the current position in our query
    // len is the length of the string in kpdftextentity
    // queryLeft is the length of the query we have left
    QString str;
    int j=0, len=0, queryLeft=query.length();
    bool haveMatch=false;
    bool dontIncrement=false;
    QLinkedList<KPDFTextEntity*>::Iterator it;
    // we dont support backward search yet
	for( it=start ; it != end;  (!dontIncrement) ? (++it) : it )
	{
        // a static cast would be faster?
        str=(*it)->txt;
        if (query.mid(j,1)==" " && query.mid(j,1)=="\n")
        {
            // lets match newline as a space
#ifdef DEBUG_TEXTPAGE
            kDebug(1223) << "newline or space" << endl;
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
            kDebug(1223) << str.left(min) << " : " << query.mid(j,min) << endl;
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
            kDebug(1223) << "\tnot matched" << endl;
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
            kDebug(1223) << "\tmatched" << endl;
#endif
                    haveMatch=true;
                    ret->append( (*it)->area );
                    j+=min;
                    queryLeft-=min;
            }
        }

	if (haveMatch && queryLeft==0 && j==query.length())
        {
            ret->simplify();
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
    QLinkedList<KPDFTextEntity*>::Iterator it,end = m_words.end();
    KPDFTextEntity * last=0;
	for( it=m_words.begin() ; it != end;  ++it )
	{
        // provide the string FIXME?: newline handling
        if (area->intersects((*it)->area))
        {
//           kDebug()<< "[" << (*it)->area->left << "," << (*it)->area->top << "]x["<< (*it)->area->right << "," << (*it)->area->bottom << "]\n";
            *ret += ((*it)->txt);
            last=*it;
        }
	}
    return ret;
}
 
