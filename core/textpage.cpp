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

//On Debugging Purpose
#include <iostream>
using namespace std;

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

/* text comparison functions */

bool CaseInsensitiveCmpFn( const QStringRef & from, const QStringRef & to,
                           int *fromLength, int *toLength )
{
    *fromLength = from.length();
    *toLength = to.length();
    return from.compare( to, Qt::CaseInsensitive ) == 0;
}

bool CaseSensitiveCmpFn( const QStringRef & from, const QStringRef & to,
                           int *fromLength, int *toLength )
{
    *fromLength = from.length();
    *toLength = to.length();
    return from.compare( to, Qt::CaseSensitive ) == 0;
}


/*
  Rationale behind TinyTextEntity:

  instead of storing directly a QString for the text of an entity,
  we store the UTF-16 data and their length. This way, we save about
  4 int's wrt a QString, and we can create a new string from that
  raw data (that's the only penalty of that).
  Even better, if the string we need to store has at most
  MaxStaticChars characters, then we store those in place of the QChar*
  that would be used (with new[] + free[]) for the data.
 */
class TinyTextEntity
{
    static const int MaxStaticChars = sizeof( QChar * ) / sizeof( QChar );

    public:
        TinyTextEntity( const QString &text, const NormalizedRect &rect )
            : area( rect )
        {
            Q_ASSERT_X( !text.isEmpty(), "TinyTextEntity", "empty string" );
            Q_ASSERT_X( sizeof( d ) == sizeof( QChar * ), "TinyTextEntity",
                        "internal storage is wider than QChar*, fix it!" );
            length = text.length();
            switch ( length )
            {
#if QT_POINTER_SIZE >= 8
                case 4:
                    d.qc[3] = text.at( 3 ).unicode();
                    // fall through
                case 3:
                    d.qc[2] = text.at( 2 ).unicode();
                    // fall through
#endif
                case 2:
                    d.qc[1] = text.at( 1 ).unicode();
                    // fall through
                case 1:
                    d.qc[0] = text.at( 0 ).unicode();
                    break;
                default:
                    d.data = new QChar[ length ];
                    std::memcpy( d.data, text.constData(), length * sizeof( QChar ) );
            }
        }

        ~TinyTextEntity()
        {
            if ( length > MaxStaticChars )
            {
                delete [] d.data;
            }
        }

        inline QString text() const
        {
            return length <= MaxStaticChars ? QString::fromRawData( ( const QChar * )&d.qc[0], length )
                                            : QString::fromRawData( d.data, length );
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

        union
        {
            QChar *data;
            ushort qc[MaxStaticChars];
        } d;
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
        d->m_words.append( new TinyTextEntity( text.normalized(QString::NormalizationForm_KC), *area ) );
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

    const QMatrix matrix = d->m_page ? d->m_page->rotationMatrix() : QMatrix();
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

    //if startPoint is right to endPoint just swap them
    NormalizedPoint temp;
    if(startCx > endCx){
        temp = startC;
        startC = endC;
        endC = temp;
    }

    //minX,maxX,minY,maxY gives the bounding rectangle coordinates of the document
    double minX, maxX, minY, maxY;
    double scaleX = this->d->m_page->m_page->width();
    double scaleY = this->d->m_page->m_page->height();

    NormalizedRect boundingRect = this->d->m_page->m_page->boundingBox();
    QRect content = boundingRect.geometry(scaleX,scaleY);

    minX = content.left(), maxX = content.right();
    minY = content.top(), maxY = content.bottom();


    /**
     we will now find out the TinyTextEntity for the startRectangle and TinyTextEntity for
     the endRectangle .. we have four cases

     Case 1(a): both startpoint and endpoint are out of the bounding Rectangle and at one side, so the rectangle made of start
     and endPoint are outof the bounding rect (do not intersect)

     Case 1(b): both startpoint and endpoint are out of bounding rect, but they are in different side, so their rectangle

     Case 2: find the rectangle which contains start and endpoint and having some
     TextEntity

     Case 3(a): the startPoint is in some empty space, which is not under any rectangle
     containing some TinyTextEntity. So, we search the nearest rectangle consisting of some
     TinyTextEntity right to or bottom of the startPoint

     Case 3(b): Same for the endPoint. Here, we have to find the point top of or left to
     start point
    **/

    //Case 1(a) - we know that startC.x > endC.x, we need to decide which is top and which is left

    NormalizedRect start_end;
    if(startC.y < endC.y)
        start_end = NormalizedRect(startC.x, startC.y, endC.x, endC.y);
    else start_end = NormalizedRect(startC.x, endC.y, endC.x, startC.y);

    if(!boundingRect.intersects(start_end)) return ret;
    //case 1(b) ......................................
    else{
        if(startC.x * scaleX < minX) startC.x = minX/scaleX;
        if(endC.x * scaleX > maxX) endC.x = maxX/scaleX;

        if(startC.y * scaleY < minY) startC.y = minY/scaleY;
        if(endC.y * scaleY > maxY) endC.y = maxY/scaleY;
    }


    TextList::ConstIterator it = d->m_words.constBegin(), itEnd = d->m_words.constEnd();
    TextList::ConstIterator start = it, end = itEnd, tmpIt = it;
    const MergeSide side = d->m_page ? (MergeSide)d->m_page->m_page->totalOrientation() : MergeRight;

    //case 2 ......................................
    for ( ; it != itEnd; ++it )
    {
        // (*it) gives a TinyTextEntity*
        tmp = (*it)->area;

        if ( ( tmp.top > startCy || ( tmp.bottom > startCy && tmp.right > startCx ) )
                && ( tmp.bottom < endCy || ( tmp.top < endCy && tmp.left < endCx ) ) )
        {
            // TinyTextEntity NormalizedRect area;
            if(tmp.contains(startCx,startCy)) start = it;
            if(tmp.contains(endCx,endCy)) end = it;
        }
    }


    it = tmpIt;
    // case 3.a .........................................
    if(start == it){
        // we can take that for start we have to increase right, bottom

        bool flagV = false;
        NormalizedRect rect;

        for ( ; it != itEnd; ++it ){

            rect= (*it)->area;
            rect.isBottom(startC) ? flagV = false: flagV = true;


            if(flagV && rect.isLeft(startC)){
                start = it;
                break;
            }
        }
    }


    //case 3.b .............................................
    if(end == itEnd){
        it = tmpIt;
        itEnd = itEnd-1;

        bool flagV = false;
        NormalizedRect rect;

        for ( ; itEnd >= it; itEnd-- ){

            rect= (*itEnd)->area;
            rect.isTop(endC) ? flagV = false: flagV = true;

            if(flagV && rect.isRight(endC)){
                end = itEnd;
                break;
            }
        }
    }



    //if start is less than end swap them
    if(start > end){

        it = start;
        start = end;
        end = it;
    }


    //TinyTextEntity ent;
    //ent.area.geometry(scaleX,scaleY);

    //QString str(' ');
    // Assume that, texts are keep in TextList in the right order
    for( ;start <= end ; ++start){
        ret->appendShape( (*start)->transformedArea( matrix ), side );

//        if((*start)->text() == str){
//                QRect rect;
//                rect = (*start)->area.geometry(scaleX,scaleY);
//                cout << "Text Before:" << (* (start-1) )->text().toAscii().data() << " "
//                     <<"Top:" << rect.top() << " Bottom: " << rect.bottom()
//                    << " Left: " << rect.left() << " Right: " << rect.right() << endl;
//            }
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
    const QMap< int, SearchPoint* >::const_iterator sIt = d->m_searchPoints.constFind( searchID );
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
    const TextComparisonFunction cmpFn = caseSensitivity == Qt::CaseSensitive
                                       ? CaseSensitiveCmpFn : CaseInsensitiveCmpFn;
    if ( forward )
    {
        ret = d->findTextInternalForward( searchID, query, caseSensitivity, cmpFn, start, end );
    }
    else
    {
        ret = d->findTextInternalBackward( searchID, query, caseSensitivity, cmpFn, start, end );
    }
    return ret;
}


RegularAreaRect* TextPagePrivate::findTextInternalForward( int searchID, const QString &_query,
                                                             Qt::CaseSensitivity caseSensitivity,
                                                             TextComparisonFunction comparer,
                                                             const TextList::ConstIterator &start,
                                                             const TextList::ConstIterator &end )
{
    const QMatrix matrix = m_page ? m_page->rotationMatrix() : QMatrix();

    RegularAreaRect* ret=new RegularAreaRect;

    // normalize query search all unicode (including glyphs)
    const QString query = (caseSensitivity == Qt::CaseSensitive)
                            ? _query.normalized(QString::NormalizationForm_KC)
                            : _query.toLower().normalized(QString::NormalizationForm_KC);

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
	kDebug() << str;
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

            int resStrLen = 0, resQueryLen = 0;
            if ( !comparer( str.midRef( offset, min ), query.midRef( j, min ),
                            &resStrLen, &resQueryLen ) )
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
                    // subtract the length of the current entity from
                    // the left length of the query
#ifdef DEBUG_TEXTPAGE
            kDebug(OkularDebug) << "\tmatched";
#endif
                    haveMatch=true;
                    ret->append( curEntity->transformedArea( matrix ) );
                    j += resStrLen;
                    queryLeft -= resQueryLen;
                    if ( it_begin == TextList::ConstIterator() )
                    {
                        it_begin = it;
                    }
            }
        }

        if (haveMatch && queryLeft==0 && j==query.length())
        {
            // save or update the search point for the current searchID
            QMap< int, SearchPoint* >::iterator sIt = m_searchPoints.find( searchID );
            if ( sIt == m_searchPoints.end() )
            {
                sIt = m_searchPoints.insert( searchID, new SearchPoint );
            }
            SearchPoint* sp = *sIt;
            sp->it_begin = it_begin;
            sp->it_end = it - 1;
            sp->offset_begin = j;
            sp->offset_end = j + qMin( queryLeft, len );
            ret->simplify();
            return ret;
        }
    }
    // end of loop - it means that we've ended the textentities
    const QMap< int, SearchPoint* >::iterator sIt = m_searchPoints.find( searchID );
    if ( sIt != m_searchPoints.end() )
    {
        SearchPoint* sp = *sIt;
        m_searchPoints.erase( sIt );
        delete sp;
    }
    delete ret;
    return 0;
}

RegularAreaRect* TextPagePrivate::findTextInternalBackward( int searchID, const QString &_query,
                                                            Qt::CaseSensitivity caseSensitivity,
                                                            TextComparisonFunction comparer,
                                                            const TextList::ConstIterator &start,
                                                            const TextList::ConstIterator &end )
{
    const QMatrix matrix = m_page ? m_page->rotationMatrix() : QMatrix();

    RegularAreaRect* ret=new RegularAreaRect;

    // normalize query to search all unicode (including glyphs)
    const QString query = (caseSensitivity == Qt::CaseSensitive)
                            ? _query.normalized(QString::NormalizationForm_KC)
                            : _query.toLower().normalized(QString::NormalizationForm_KC);

    // j is the current position in our query
    // len is the length of the string in TextEntity
    // queryLeft is the length of the query we have left
    const TinyTextEntity* curEntity = 0;
    int j=query.length() - 1, len=0, queryLeft=query.length();
    bool haveMatch=false;
    bool offsetMoved = false;
    TextList::ConstIterator it = start;
    TextList::ConstIterator it_begin;
    while ( true )
    {
        curEntity = *it;
        const QString &str = curEntity->text();
        if ( !offsetMoved && ( it == start ) )
        {
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
        }
        else
        {
            len=str.length();
            int min=qMin(queryLeft,len);
#ifdef DEBUG_TEXTPAGE
            kDebug(OkularDebug) << str.right(min) << " : " << _query.mid(j-min+1,min);
#endif
            // we have equal (or less than) area of the query left as the length of the current
            // entity

            int resStrLen = 0, resQueryLen = 0;
            if ( !comparer( str.rightRef( min ), query.midRef( j - min + 1, min ),
                            &resStrLen, &resQueryLen ) )
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
                    queryLeft=query.length();
                    it_begin = TextList::ConstIterator();
            }
            else
            {
                    // we have a match
                    // move the current position in the query
                    // to the position after the length of this string
                    // we matched
                    // subtract the length of the current entity from
                    // the left length of the query
#ifdef DEBUG_TEXTPAGE
                    kDebug(OkularDebug) << "\tmatched";
#endif
                    haveMatch=true;
                    ret->append( curEntity->transformedArea( matrix ) );
                    j -= resStrLen;
                    queryLeft -= resQueryLen;
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
    const QMap< int, SearchPoint* >::iterator sIt = m_searchPoints.find( searchID );
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
    return text(area, AnyPixelTextAreaInclusionBehaviour);
}

QString TextPage::text(const RegularAreaRect *area, TextAreaInclusionBehaviour b) const
{
    if ( area && area->isNull() )
        return QString();

    TextList::ConstIterator it = d->m_words.constBegin(), itEnd = d->m_words.constEnd();
    QString ret;
    if ( area )
    {
        for ( ; it != itEnd; ++it )
        {
            if (b == AnyPixelTextAreaInclusionBehaviour)
            {
                if ( area->intersects( (*it)->area ) )
                {
                    ret += (*it)->text();
                }
            }
            else
            {
                NormalizedPoint center = (*it)->area.center();
                if ( area->contains( center.x, center.y ) )
                {
                    ret += (*it)->text();
                }
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

// mamun.nightcrawler@gmail.com
void TextPage::printTextPageContent(){
    // tList is our textList for this text page
    // TextList is of type List<TinyTextEntity* >
    TextList tList = this->d->m_words;

    foreach(TinyTextEntity* tiny, tList){
        cout << tiny->text().toAscii().data();
        QRect rect = tiny->area.geometry(d->m_page->m_page->width(),d->m_page->m_page->height());
        cout << " area: " << rect.top() << "," << rect.left() << " " << rect.bottom() << "," << rect.right() << endl;
    }

}

//remove unEvenSpace, currently removes necessary spaces also :(
void TextPage::removeSpace(){

    TextList::Iterator it = d->m_words.begin(), itEnd = d->m_words.end(), tmpIt = it;
    QString str(' ');

    // find the average space length
    for( ; it != itEnd ; it++){
        //if TextEntity contains space
        if((*it)->text() == str)
            this->d->m_words.erase(it);
    }

}

//correct the textOrder, all layout recognition works here
void TextPage::correctTextOrder(){

}

//add necessary spaces in the text - mainly for copy purpose
void TextPage::addNecessarySpace(){

}
