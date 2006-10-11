/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kdebug.h>

#include "area.h"
#include "misc.h"
#include "textpage.h"

using namespace Okular;

struct Okular::SearchPoint
{
    SearchPoint() : theIt( 0 ), offset_begin( -1 ), offset_end( -1 ) {}
    QList<TextEntity*>::Iterator theIt;
    int offset_begin;
    int offset_end;
};


TextPage::~TextPage()
{
    qDeleteAll(m_words);
    qDeleteAll(m_searchPoints);
}

RegularAreaRect * TextPage::getTextArea ( TextSelection * sel) const
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
#ifdef DEBUG_TEXTPAGE
          kWarning() << "running first loop\n";
#endif
          for (it=0;it<m_words.count();it++)
          {
              tmp=m_words[it]->area;
              if (tmp->contains(startC->x,startC->y) 
                  || ( tmp->top <= startC->y && tmp->bottom >= startC->y && tmp->left >= startC->x )
                  || ( tmp->top >= startC->y))
              {
                  /// we have found the (rx,ry)x(tx,ty)   
                  itB=it;
#ifdef DEBUG_TEXTPAGE
                  kWarning() << "start is " << itB << " count is " << m_words.count() << endl;
#endif
                  break;
              }
  
          }
          sel->itB(itB);
        }
        itB=sel->itB();
#ifdef DEBUG_TEXTPAGE
        kWarning() << "direction is " << sel->dir() << endl;
        kWarning() << "reloaded start is " << itB << " against " << sel->itB() << endl;
#endif
        if (sel->dir() == 0 || (sel->itE() == -1 && sel->dir()==1))
        {
#ifdef DEBUG_TEXTPAGE
          kWarning() << "running second loop\n";
#endif
          for (it=m_words.count()-1; it>=itB;it--)
          {
              tmp=m_words[it]->area;
              if (tmp->contains(endC->x,endC->y) 
                  || ( tmp->top <= endC->y && tmp->bottom >= endC->y && tmp->right <= endC->x )
                  || ( tmp->bottom <= endC->y))
              {
                  /// we have found the (ux,uy)x(vx,vy)   
                  itE=it;
#ifdef DEBUG_TEXTPAGE
                  kWarning() << "ending is " << itE << " count is " << m_words.count() << endl;
                  kWarning () << "conditions " << tmp->contains(endC->x,endC->y) << " " 
                    << ( tmp->top <= endC->y && tmp->bottom >= endC->y && tmp->right <= endC->x ) << " " <<
                    ( tmp->top >= endC->y) << endl;
#endif

                  break;
              }
          }
          sel->itE(itE);
        }
#ifdef DEBUG_TEXTPAGE
        kWarning() << "reloaded ending is " << itE << " against " << sel->itE() << endl;
#endif

        if (sel->itB()!=-1 && sel->itE()!=-1)
        {
          start=m_words[sel->itB()]->area;
          end=m_words[sel->itE()]->area;

          NormalizedRect first,second,third;/*
          first.right=1;
          /// if (rx,ry)x(1,ty) intersects the end cursor, there is only one line
          bool sameBaseline=end->intersects(first);
#ifdef DEBUG_TEXTPAGE
          kWarning() << "sameBaseline : " << sameBaseline << endl;
#endif
          if (sameBaseline)
          {
              first=*start;
              first.right=end->right;
              first.bottom=end->bottom;
              for (it=qMin(sel->itB(),sel->itE()); it<=qMax(sel->itB(),sel->itE());it++)
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
            for (it=qMin(sel->itB(),sel->itE()); it<=qMax(sel->itB(),sel->itE());it++)
            {
                tmp=m_words[it]->area;
                if (tmp->intersects(&first) || tmp->intersects(&second) || tmp->intersects(&third))
                  ret->append(new NormalizedRect(*tmp));
            }

//           }
        }

        ret->simplify();
	return ret;
}


RegularAreaRect* TextPage::findText(int searchID, const QString &query, SearchDir & direct,
    bool strictCase, const RegularAreaRect *area)
{
    SearchDir dir=direct;
    // invalid search request
    if ( query.isEmpty() || area->isNull() )
        return 0;
    QList<TextEntity*>::Iterator start;
    QList<TextEntity*>::Iterator end;
    if ( !m_searchPoints.contains( searchID ) )
    {
        // if no previous run of this search is found, then set it to start
        // from the beginning (respecting the search direction)
        if ( dir == NextRes )
            dir = FromTop;
        else if ( dir == PrevRes )
            dir = FromBottom;
    }
    bool forward = true;
    switch ( dir )
    {
        case FromTop:
            start = m_words.begin();
            end = m_words.end();
            break;
        case FromBottom:
            start = m_words.end();
            end = m_words.begin();
            if ( !m_words.isEmpty() )
            {
                --start;
            }
            forward = false;
            break;
        case NextRes:
            start = m_searchPoints[ searchID ]->theIt;
            end = m_words.end();
            break;
        case PrevRes:
            start = m_searchPoints[ searchID ]->theIt;
            end = m_words.begin();
            forward = false;
            break;
    };
    RegularAreaRect* ret = 0;
    if ( forward )
    {
        ret = findTextInternalForward( searchID, query, strictCase, start, end );
    }
    // TODO implement backward search
#if 0
    else
    {
        ret = findTextInternalBackward( searchID, query, strictCase, start, end );
    }
#endif
    return ret;
}


RegularAreaRect* TextPage::findTextInternalForward(int searchID, const QString &_query,
        bool strictCase, const QList<TextEntity*>::Iterator &start,
        const QList<TextEntity*>::Iterator &end)
{

    RegularAreaRect* ret=new RegularAreaRect;
    QString query = strictCase ? _query : _query.toLower();

    // j is the current position in our query
    // len is the length of the string in TextEntity
    // queryLeft is the length of the query we have left
    QString str;
    TextEntity* curEntity = 0;
    int j=0, len=0, queryLeft=query.length();
    int offset = 0;
    bool haveMatch=false;
    bool dontIncrement=false;
    bool offsetMoved = false;
    QList<TextEntity*>::Iterator it = start;
    for ( ; it != end; ++it )
    {
        curEntity = *it;
        str = curEntity->txt;
        if ( !offsetMoved && ( it == start ) )
        {
            if ( m_searchPoints.contains( searchID ) )
            {
                offset = qMax( m_searchPoints[ searchID ]->offset_end, 0 );
            }
            offsetMoved = true;
        }
        if ( query.at(j).isSpace() )
        {
            // lets match newline as a space
#ifdef DEBUG_TEXTPAGE
            kDebug(1223) << "newline or space" << endl;
#endif
            j++;
            queryLeft--;
            // since we do not really need to increment this after this
            // run of the loop finishes because we are not comparing it 
            // to any entity, rather we are deducing a situation in a document
            dontIncrement=true;
        }
        else
        {
            dontIncrement=false;
            len=str.length();
            int min=qMin(queryLeft,len);
#ifdef DEBUG_TEXTPAGE
            kDebug(1223) << str.mid(offset,min) << " : " << _query.mid(j,min) << endl;
#endif
            // we have equal (or less then) area of the query left as the lengt of the current 
            // entity

            if ((strictCase)
                ? (str.mid(offset,min) != query.mid(j,min))
                : (str.mid(offset,min).toLower() != query.mid(j,min))
                )
            {
                    // we not have matched
                    // this means we do not have a complete match
                    // we need to get back to query start
                    // and continue the search from this place
                    haveMatch=false;
                    ret->clear();
#ifdef DEBUG_TEXTPAGE
            kDebug(1223) << "\tnot matched" << endl;
#endif
                    j=0;
                    offset = 0;
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
                    ret->append( curEntity->area );
                    j+=min;
                    queryLeft-=min;
            }
        }

        if (haveMatch && queryLeft==0 && j==query.length())
        {
            // save or update the search point for the current searchID
            if ( !m_searchPoints.contains( searchID ) )
            {
                SearchPoint* newsp = new SearchPoint;
                m_searchPoints.insert( searchID, newsp );
            }
            SearchPoint* sp = m_searchPoints[ searchID ];
            sp->theIt = it;
            sp->offset_begin = j;
            sp->offset_end = j + qMin( queryLeft, len );
            ret->simplify();
            return ret;
        }
    }
    // end of loop - it means that we've ended the textentities
    if ( m_searchPoints.contains( searchID ) )
    {
        SearchPoint* sp = m_searchPoints[ searchID ];
        m_searchPoints.remove( searchID );
        delete sp;
    }
    delete ret;
    return 0;
}

QString TextPage::getText(const RegularAreaRect *area) const
{
    if (!area || area->isNull())
        return QString();

    QString ret = "";
    QList<TextEntity*>::ConstIterator it,end = m_words.end();
    TextEntity * last=0;
    for ( it = m_words.begin(); it != end; ++it )
    {
        // provide the string FIXME?: newline handling
        if (area->intersects((*it)->area))
        {
//           kDebug()<< "[" << (*it)->area->left << "," << (*it)->area->top << "]x["<< (*it)->area->right << "," << (*it)->area->bottom << "]\n";
            ret += (*it)->txt;
            last=*it;
        }
    }
    return ret;
}

