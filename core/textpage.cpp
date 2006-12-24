/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QMap>

#include <kdebug.h>

#include "area.h"
#include "misc.h"
#include "textpage.h"

using namespace Okular;

class SearchPoint
{
    public:
        SearchPoint()
            : theIt( 0 ), offset_begin( -1 ), offset_end( -1 )
        {
        }

        TextEntity::List::ConstIterator theIt;
        int offset_begin;
        int offset_end;
};

TextEntity::TextEntity( const QString &text, NormalizedRect *area )
    : m_text( text ), m_area( area ), m_transformed_area( 0 ), d( 0 )
{
    m_transformed_area = new NormalizedRect( *m_area );
}

TextEntity::~TextEntity()
{
    delete m_area;
    delete m_transformed_area;
}

QString TextEntity::text() const
{
    return m_text;
}

NormalizedRect* TextEntity::area() const
{
    return m_area;
}

NormalizedRect* TextEntity::transformedArea() const
{
    return m_transformed_area;
}

void TextEntity::transform( const QMatrix &matrix )
{
    *m_transformed_area = *m_area;
    m_transformed_area->transform( matrix );
}

class TextPage::Private
{
    public:
        Private( const TextEntity::List &words )
            : m_words( words )
        {
        }

        ~Private()
        {
            qDeleteAll( m_searchPoints );
            qDeleteAll( m_words );
        }

        RegularAreaRect * findTextInternalForward( int searchID, const QString &query,
                                                   Qt::CaseSensitivity caseSensitivity,
                                                   const TextEntity::List::ConstIterator &start,
                                                   const TextEntity::List::ConstIterator &end );

        TextEntity::List m_words;
        QMap< int, SearchPoint* > m_searchPoints;
};


TextPage::TextPage()
    : d( new Private( TextEntity::List() ) )
{
}

TextPage::TextPage( const TextEntity::List &words )
    : d( new Private( words ) )
{
}

TextPage::~TextPage()
{
    delete d;
}

void TextPage::append( const QString &text, NormalizedRect *area )
{
    d->m_words.append( new TextEntity( text, area ) );
}

RegularAreaRect * TextPage::textArea ( TextSelection * sel) const
{
    if ( d->m_words.isEmpty() )
        return new RegularAreaRect();

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
        NormalizedPoint startC=sel->start();
        double startCx=startC.x,startCy=startC.y;
        NormalizedPoint endC=sel->end();
        double endCx=endC.x,endCy=endC.y;
        if (sel->dir() == 1 || (sel->itB()==-1 && sel->dir()==0))
        {
#ifdef DEBUG_TEXTPAGE
          kWarning() << "running first loop\n";
#endif
          for (it=0;it<d->m_words.count();it++)
          {
              tmp=d->m_words[it]->transformedArea();
              if (tmp->contains(startCx,startCy) 
                  || ( tmp->top <= startCy && tmp->bottom >= startCy && tmp->left >= startCx )
                  || ( tmp->top >= startCy))
              {
                  /// we have found the (rx,ry)x(tx,ty)   
                  itB=it;
#ifdef DEBUG_TEXTPAGE
                  kWarning() << "start is " << itB << " count is " << d->m_words.count() << endl;
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
          for (it=d->m_words.count()-1; it>=itB;it--)
          {
              tmp=d->m_words[it]->transformedArea();
              if (tmp->contains(endCx,endCy) 
                  || ( tmp->top <= endCy && tmp->bottom >= endCy && tmp->right <= endCx )
                  || ( tmp->bottom <= endCy))
              {
                  /// we have found the (ux,uy)x(vx,vy)   
                  itE=it;
#ifdef DEBUG_TEXTPAGE
                  kWarning() << "ending is " << itE << " count is " << d->m_words.count() << endl;
                  kWarning () << "conditions " << tmp->contains(endCx,endCy) << " " 
                    << ( tmp->top <= endCy && tmp->bottom >= endCy && tmp->right <= endCx ) << " " <<
                    ( tmp->top >= endCy) << endl;
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
          start=d->m_words[sel->itB()]->transformedArea();
          end=d->m_words[sel->itE()]->transformedArea();

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
                tmp=d->m_words[it]->area();
                if (tmp->intersects(&first))
                  ret->appendShape(tmp);
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
            int selMax = qMax( sel->itB(), sel->itE() );
            for ( it = qMin( sel->itB(), sel->itE() ); it <= selMax; ++it )
            {
                tmp=d->m_words[it]->transformedArea();
                if (tmp->intersects(&first) || tmp->intersects(&second) || tmp->intersects(&third))
                  ret->appendShape(*tmp);
            }

//           }
        }

	return ret;
}


RegularAreaRect* TextPage::findText( int searchID, const QString &query, SearchDirection & direct,
                                     Qt::CaseSensitivity caseSensitivity, const RegularAreaRect *area )
{
    SearchDirection dir=direct;
    // invalid search request
    if ( query.isEmpty() || area->isNull() )
        return 0;
    TextEntity::List::ConstIterator start;
    TextEntity::List::ConstIterator end;
    if ( !d->m_searchPoints.contains( searchID ) )
    {
        // if no previous run of this search is found, then set it to start
        // from the beginning (respecting the search direction)
        if ( dir == NextResult )
            dir = FromTop;
        else if ( dir == PreviousResult )
            dir = FromBottom;
    }
    bool forward = true;
    switch ( dir )
    {
        case FromTop:
            start = d->m_words.begin();
            end = d->m_words.end();
            break;
        case FromBottom:
            start = d->m_words.end();
            end = d->m_words.begin();
            if ( !d->m_words.isEmpty() )
            {
                --start;
            }
            forward = false;
            break;
        case NextResult:
            start = d->m_searchPoints[ searchID ]->theIt;
            end = d->m_words.end();
            break;
        case PreviousResult:
            start = d->m_searchPoints[ searchID ]->theIt;
            end = d->m_words.begin();
            forward = false;
            break;
    };
    RegularAreaRect* ret = 0;
    if ( forward )
    {
        ret = d->findTextInternalForward( searchID, query, caseSensitivity, start, end );
    }
    // TODO implement backward search
#if 0
    else
    {
        ret = findTextInternalBackward( searchID, query, caseSensitivity, start, end );
    }
#endif
    return ret;
}


RegularAreaRect* TextPage::Private::findTextInternalForward( int searchID, const QString &_query,
                                                             Qt::CaseSensitivity caseSensitivity,
                                                             const TextEntity::List::ConstIterator &start,
                                                             const TextEntity::List::ConstIterator &end )
{

    RegularAreaRect* ret=new RegularAreaRect;
    QString query = (caseSensitivity == Qt::CaseSensitive) ? _query : _query.toLower();

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
    TextEntity::List::ConstIterator it = start;
    for ( ; it != end; ++it )
    {
        curEntity = *it;
        str = curEntity->text();
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

            if ((caseSensitivity == Qt::CaseSensitive)
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
                    ret->append( *curEntity->transformedArea() );
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

QString TextPage::text(const RegularAreaRect *area) const
{
    if ( area && area->isNull() )
        return QString();

    TextEntity::List::ConstIterator it = d->m_words.begin(), itEnd = d->m_words.end();
    QString ret;
    if ( area )
    {
        for ( ; it != itEnd; ++it )
        {
            if ( area->intersects( *(*it)->transformedArea() ) )
            {
                ret += (*it)->text();
            }
        }
    }
    else
    {
        for ( ; it != itEnd; ++it )
            ret += (*it)->text();
    }
    return ret;
}

void TextPage::transform( const QMatrix &matrix )
{
    for ( int i = 0; i < d->m_words.count(); ++i )
        d->m_words[ i ]->transform( matrix );
}
