/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "textpage.h"
#include "textpage_p.h"

#include <kdebug.h>

#include "area.h"
#include "debug_p.h"
#include "misc.h"
#include "page.h"
#include "page_p.h"

#include <cstring>

using namespace Okular;

class SearchPoint
{
    public:
        SearchPoint()
            : offset_begin( -1 ), offset_end( -1 )
        {
        }

        TextList::ConstIterator it_begin;
        TextList::ConstIterator it_end;
        int offset_begin;
        int offset_end;
};


/*
  Rationale behind TinyTextEntity:

  instead of storing directly a QString for the text of an entity,
  we store the UTF-16 data and their length. This way, we save about
  4 int's wrt a QString, and we can create a new string from that
  raw data (that's the only penalty of that).
 */
class TinyTextEntity
{
    public:
        TinyTextEntity( const QString &text, const NormalizedRect &rect )
            : area( rect )
        {
            Q_ASSERT_X( !text.isEmpty(), "TinyTextEntity", "empty string" );
            length = text.length();
            data = new QChar[ length ];
            std::memcpy( data, text.constData(), length * sizeof( QChar ) );
        }

        ~TinyTextEntity()
        {
            delete [] data;
        }

        inline QString text() const
        {
            return QString::fromRawData( data, length );
        }

        inline NormalizedRect transformedArea( const QMatrix &matrix ) const
        {
            NormalizedRect transformed_area = area;
            transformed_area.transform( matrix );
            return transformed_area;
        }

        NormalizedRect area;

    private:
        Q_DISABLE_COPY( TinyTextEntity )

        QChar *data;
        int length;
};


TextEntity::TextEntity( const QString &text, NormalizedRect *area )
    : m_text( text ), m_area( area ), d( 0 )
{
}

TextEntity::~TextEntity()
{
    delete m_area;
}

QString TextEntity::text() const
{
    return m_text;
}

NormalizedRect* TextEntity::area() const
{
    return m_area;
}

NormalizedRect TextEntity::transformedArea(const QMatrix &matrix) const
{
    NormalizedRect transformed_area = *m_area;
    transformed_area.transform( matrix );
    return transformed_area;
}


TextPagePrivate::TextPagePrivate()
    : m_page( 0 )
{
}

TextPagePrivate::~TextPagePrivate()
{
    qDeleteAll( m_searchPoints );
    qDeleteAll( m_words );
}


TextPage::TextPage()
    : d( new TextPagePrivate() )
{
}

TextPage::TextPage( const TextEntity::List &words )
    : d( new TextPagePrivate() )
{
    TextEntity::List::ConstIterator it = words.constBegin(), itEnd = words.constEnd();
    for ( ; it != itEnd; ++it )
    {
        TextEntity *e = *it;
        if ( !e->text().isEmpty() )
            d->m_words.append( new TinyTextEntity( e->text(), *e->area() ) );
        delete e;
    }
}

TextPage::~TextPage()
{
    delete d;
}

void TextPage::append( const QString &text, NormalizedRect *area )
{
    if ( !text.isEmpty() )
        d->m_words.append( new TinyTextEntity( text, *area ) );
    delete area;
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

    QMatrix matrix = d->m_page ? d->m_page->rotationMatrix() : QMatrix();
#if 0
    int it = -1;
    int itB = -1;
    int itE = -1;

    // ending cursor is higher than start cursor, we need to find positions in reverse
    NormalizedRect tmp;
    NormalizedRect start;
    NormalizedRect end;

    NormalizedPoint startC = sel->start();
    double startCx = startC.x;
    double startCy = startC.y;

    NormalizedPoint endC = sel->end();
    double endCx = endC.x;
    double endCy = endC.y;

    if ( sel->direction() == 1 || ( sel->itB() == -1 && sel->direction() == 0 ) )
    {
#ifdef DEBUG_TEXTPAGE
        kWarning() << "running first loop";
#endif
        const int count = d->m_words.count();
        for ( it = 0; it < count; it++ )
        {
            tmp = *d->m_words[ it ]->area();
            if ( tmp.contains( startCx, startCy )
                 || ( tmp.top <= startCy && tmp.bottom >= startCy && tmp.left >= startCx )
                 || ( tmp.top >= startCy))
            {
                /// we have found the (rx,ry)x(tx,ty)
                itB = it;
#ifdef DEBUG_TEXTPAGE
                kWarning() << "start is" << itB << "count is" << d->m_words.count();
#endif
                break;
            }
        }
        sel->itB( itB );
    }
    itB = sel->itB();
#ifdef DEBUG_TEXTPAGE
    kWarning() << "direction is" << sel->direction();
    kWarning() << "reloaded start is" << itB << "against" << sel->itB();
#endif
    if ( sel->direction() == 0 || ( sel->itE() == -1 && sel->direction() == 1 ) )
    {
#ifdef DEBUG_TEXTPAGE
        kWarning() << "running second loop";
#endif
        for ( it = d->m_words.count() - 1; it >= itB; it-- )
        {
            tmp = *d->m_words[ it ]->area();
            if ( tmp.contains( endCx, endCy )
                 || ( tmp.top <= endCy && tmp.bottom >= endCy && tmp.right <= endCx )
                 || ( tmp.bottom <= endCy ) )
            {
                /// we have found the (ux,uy)x(vx,vy)
                itE = it;
#ifdef DEBUG_TEXTPAGE
                kWarning() << "ending is" << itE << "count is" << d->m_words.count();
                kWarning() << "conditions" << tmp.contains( endCx, endCy ) << " " 
                  << ( tmp.top <= endCy && tmp.bottom >= endCy && tmp.right <= endCx ) << " " <<
                  ( tmp.top >= endCy);
#endif
                break;
            }
        }
        sel->itE( itE );
    }
#ifdef DEBUG_TEXTPAGE
    kWarning() << "reloaded ending is" << itE << "against" << sel->itE();
#endif

    if ( sel->itB() != -1 && sel->itE() != -1 )
    {
        start = *d->m_words[ sel->itB() ]->area();
        end = *d->m_words[ sel->itE() ]->area();

        NormalizedRect first, second, third;
        /// finding out if there is more than one baseline between them is a hard and discussable task
        /// we will create a rectangle (rx,0)x(tx,1) and will check how many times does it intersect the 
        /// areas, if more than one -> we have a three or over line selection
        first = start;
        second.top = start.bottom;
        first.right = second.right = 1;
        third = end;
        third.left = second.left = 0;
        second.bottom = end.top;
        int selMax = qMax( sel->itB(), sel->itE() );
        for ( it = qMin( sel->itB(), sel->itE() ); it <= selMax; ++it )
        {
            tmp = *d->m_words[ it ]->area();
            if ( tmp.intersects( &first ) || tmp.intersects( &second ) || tmp.intersects( &third ) )
                ret->appendShape( d->m_words.at( it )->transformedArea( matrix ) );
        }
    }
#else
    NormalizedRect tmp;

    NormalizedPoint startC = sel->start();
    double startCx = startC.x;
    double startCy = startC.y;

    NormalizedPoint endC = sel->end();
    double endCx = endC.x;
    double endCy = endC.y;

    TextList::ConstIterator it = d->m_words.constBegin(), itEnd = d->m_words.constEnd();
    MergeSide side = d->m_page ? (MergeSide)d->m_page->m_page->totalOrientation() : MergeRight;
    for ( ; it != itEnd; ++it )
    {
        tmp = (*it)->area;
        if ( ( tmp.top > startCy || ( tmp.bottom > startCy && tmp.right > startCx ) )
             && ( tmp.bottom < endCy || ( tmp.top < endCy && tmp.left < endCx ) ) )
        {
            ret->appendShape( (*it)->transformedArea( matrix ), side );
        }
    }
#endif

    return ret;
}


RegularAreaRect* TextPage::findText( int searchID, const QString &query, SearchDirection direct,
                                     Qt::CaseSensitivity caseSensitivity, const RegularAreaRect *area )
{
    SearchDirection dir=direct;
    // invalid search request
    if ( d->m_words.isEmpty() || query.isEmpty() || ( area && area->isNull() ) )
        return 0;
    TextList::ConstIterator start;
    TextList::ConstIterator end;
    QMap< int, SearchPoint* >::const_iterator sIt = d->m_searchPoints.constFind( searchID );
    if ( sIt == d->m_searchPoints.constEnd() )
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
            start = d->m_words.constBegin();
            end = d->m_words.constEnd();
            break;
        case FromBottom:
            start = d->m_words.constEnd();
            end = d->m_words.constBegin();
            Q_ASSERT( start != end );
            // we can safely go one step back, as we already checked
            // that the list is not empty
            --start;
            forward = false;
            break;
        case NextResult:
            start = (*sIt)->it_end;
            end = d->m_words.constEnd();
            if ( ( start + 1 ) != end )
                ++start;
            break;
        case PreviousResult:
            start = (*sIt)->it_begin;
            end = d->m_words.constBegin();
            if ( start != end )
                --start;
            forward = false;
            break;
    };
    RegularAreaRect* ret = 0;
    if ( forward )
    {
        ret = d->findTextInternalForward( searchID, query, caseSensitivity, start, end );
    }
    else
    {
        ret = d->findTextInternalBackward( searchID, query, caseSensitivity, start, end );
    }
    return ret;
}


RegularAreaRect* TextPagePrivate::findTextInternalForward( int searchID, const QString &_query,
                                                             Qt::CaseSensitivity caseSensitivity,
                                                             const TextList::ConstIterator &start,
                                                             const TextList::ConstIterator &end )
{
    QMatrix matrix = m_page ? m_page->rotationMatrix() : QMatrix();

    RegularAreaRect* ret=new RegularAreaRect;
    QString query = (caseSensitivity == Qt::CaseSensitive) ? _query : _query.toLower();

    // j is the current position in our query
    // len is the length of the string in TextEntity
    // queryLeft is the length of the query we have left
    const TinyTextEntity* curEntity = 0;
    int j=0, len=0, queryLeft=query.length();
    int offset = 0;
    bool haveMatch=false;
    bool offsetMoved = false;
    TextList::ConstIterator it = start;
    TextList::ConstIterator it_begin;
    for ( ; it != end; ++it )
    {
        curEntity = *it;
        const QString &str = curEntity->text();
        if ( !offsetMoved && ( it == start ) )
        {
            if ( m_searchPoints.contains( searchID ) )
            {
                offset = qMax( m_searchPoints[ searchID ]->offset_end, 0 );
            }
            offsetMoved = true;
        }
        {
            len=str.length();
            int min=qMin(queryLeft,len);
#ifdef DEBUG_TEXTPAGE
            kDebug(OkularDebug) << str.mid(offset,min) << ":" << _query.mid(j,min);
#endif
            // we have equal (or less than) area of the query left as the length of the current 
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
            kDebug(OkularDebug) << "\tnot matched";
#endif
                    j=0;
                    offset = 0;
                    queryLeft=query.length();
                    it_begin = TextList::ConstIterator();
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
            kDebug(OkularDebug) << "\tmatched";
#endif
                    haveMatch=true;
                    ret->append( curEntity->transformedArea( matrix ) );
                    j+=min;
                    queryLeft-=min;
                    if ( it_begin == TextList::ConstIterator() )
                    {
                        it_begin = it;
                    }
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
            sp->it_begin = it_begin;
            sp->it_end = it;
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

RegularAreaRect* TextPagePrivate::findTextInternalBackward( int searchID, const QString &_query,
                                                            Qt::CaseSensitivity caseSensitivity,
                                                            const TextList::ConstIterator &start,
                                                            const TextList::ConstIterator &end )
{
    QMatrix matrix = m_page ? m_page->rotationMatrix() : QMatrix();

    RegularAreaRect* ret=new RegularAreaRect;
    const QString query = (caseSensitivity == Qt::CaseSensitive) ? _query : _query.toLower();

    // j is the current position in our query
    // len is the length of the string in TextEntity
    // queryLeft is the length of the query we have left
    const TinyTextEntity* curEntity = 0;
    int j=query.length() - 1, len=0, queryLeft=query.length();
    int offset = 0;
    bool haveMatch=false;
    bool dontIncrement=false;
    bool offsetMoved = false;
    TextList::ConstIterator it = start;
    TextList::ConstIterator it_begin;
    while ( true )
    {
        curEntity = *it;
        const QString &str = curEntity->text();
        if ( !offsetMoved && ( it == start ) )
        {
            if ( m_searchPoints.contains( searchID ) )
            {
                offset = qMax( m_searchPoints[ searchID ]->offset_begin, 0 );
            }
            offsetMoved = true;
        }
        if ( query.at(j).isSpace() )
        {
            // lets match newline as a space
#ifdef DEBUG_TEXTPAGE
            kDebug(OkularDebug) << "newline or space";
#endif
            j--;
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
            kDebug(OkularDebug) << str.right(min) << " : " << _query.mid(j-min+1,min);
#endif
            // we have equal (or less than) area of the query left as the length of the current 
            // entity

            if ((caseSensitivity == Qt::CaseSensitive)
                ? (str.right(min) != query.mid(j-min+1,min))
                : (str.right(min).toLower() != query.mid(j-min+1,min))
                )
            {
                    // we not have matched
                    // this means we do not have a complete match
                    // we need to get back to query start
                    // and continue the search from this place
                    haveMatch=false;
                    ret->clear();
#ifdef DEBUG_TEXTPAGE
                    kDebug(OkularDebug) << "\tnot matched";
#endif
                    j=query.length() - 1;
                    offset = 0;
                    queryLeft=query.length();
                    it_begin = TextList::ConstIterator();
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
                    kDebug(OkularDebug) << "\tmatched";
#endif
                    haveMatch=true;
                    ret->append( curEntity->transformedArea( matrix ) );
                    j-=min;
                    queryLeft-=min;
                    if ( it_begin == TextList::ConstIterator() )
                    {
                        it_begin = it;
                    }
            }
        }

        if (haveMatch && queryLeft==0 && j<0)
        {
            // save or update the search point for the current searchID
            QMap< int, SearchPoint* >::iterator sIt = m_searchPoints.find( searchID );
            if ( sIt == m_searchPoints.end() )
            {
                sIt = m_searchPoints.insert( searchID, new SearchPoint );
            }
            SearchPoint* sp = *sIt;
            sp->it_begin = it;
            sp->it_end = it_begin;
            sp->offset_begin = j;
            sp->offset_end = j + qMin( queryLeft, len );
            ret->simplify();
            return ret;
        }
        if ( it == end )
            break;
        else
            --it;
    }
    // end of loop - it means that we've ended the textentities
    QMap< int, SearchPoint* >::iterator sIt = m_searchPoints.find( searchID );
    if ( sIt != m_searchPoints.end() )
    {
        SearchPoint* sp = *sIt;
        m_searchPoints.erase( sIt );
        delete sp;
    }
    delete ret;
    return 0;
}

QString TextPage::text(const RegularAreaRect *area) const
{
    if ( area && area->isNull() )
        return QString();

    TextList::ConstIterator it = d->m_words.constBegin(), itEnd = d->m_words.constEnd();
    QString ret;
    if ( area )
    {
        for ( ; it != itEnd; ++it )
        {
            if ( area->intersects( (*it)->area ) )
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
