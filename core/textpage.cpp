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
#include <QtAlgorithms>

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

// This class will store the area and TextList of the region in sorted order
class RegionText{

public:
    RegionText(TextList &list,QRect &area)
        : m_region_text(list) ,m_area(area)
    {
    }

    // we are not giving any set method for the texts, we assume it will be set only once
    // at the time of construction
    inline TextList text() const{
        return m_region_text;
    }

    inline QRect area() const{
        return m_area;
    }

private:
    TextList m_region_text;
    QRect m_area;
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
    QRect content = boundingRect.roundedGeometry(scaleX,scaleY);

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
        QRect rect = tiny->area.roundedGeometry(d->m_page->m_page->width(),d->m_page->m_page->height());
        cout << " area: " << rect.top() << "," << rect.left() << " " << rect.bottom() << "," << rect.right() << endl;
    }

}


/** mamun_nightcrawler@gmail.com **/

void printRect(QRect rect){

    cout << "l: " << rect.left() << " r: " << rect.x() + rect.width() << " t: " << rect.top() <<
            " b: " << rect.y() + rect.height() << endl;
}


//remove all the spaces between texts, it will keep all the generators same, whether they save spaces or not
void TextPage::removeSpace(){

    TextList::Iterator it = d->m_words.begin(), itEnd = d->m_words.end();
    QString str(' ');

//    cout << "text before space removal ............................................" << endl;

//    for( ; it != itEnd ; it++){
//        //if TextEntity contains space
//        cout << (*it)->text().toAscii().data();
//    }
//    cout << endl;

    int pageWidth = d->m_page->m_page->width(), pageHeight = d->m_page->m_page->height();

    // find the average space length()
    int maxWordSpace = 0, minWordSpace = pageWidth;
    it = d->m_words.begin(), itEnd = d->m_words.end();
    for( ; it != itEnd ; it++){
        //if TextEntity contains space
        if((*it)->text() == str){

//            cout << "text Space: ";
            QRect area = (*it)->area.roundedGeometry(pageWidth,pageHeight);
//            cout << area.right() - area.left() << " ";
//            printRect(area);

            this->d->m_words.erase(it);

            if(area.width() > maxWordSpace) maxWordSpace = area.width();
            if(area.width() < minWordSpace) minWordSpace = area.width();
        }
    }

    cout << "max Word Spacing " << maxWordSpace << endl;
    cout << "min Word Spacing " << minWordSpace << endl;

//    cout << endl << endl;

}


bool compareTinyTextEntityX(TinyTextEntity* first, TinyTextEntity* second){
    QRect firstArea = first->area.roundedGeometry(1000,1000);
    QRect secondArea = second->area.roundedGeometry(1000,1000);

    return firstArea.left() < secondArea.left();
}

bool compareTinyTextEntityY(TinyTextEntity* first, TinyTextEntity* second){
    QRect firstArea = first->area.roundedGeometry(1000,1000);
    QRect secondArea = second->area.roundedGeometry(1000,1000);

    return firstArea.top() < secondArea.top();
}


void TextPagePrivate::printTextList(int i, TextList list){

    QRect rect = m_line_rects.at(i);
//    cout << "L:" << rect.left() << " R:" << rect.right() << " T:" << rect.top() << " B:" << rect.bottom() << endl;
    cout << "Line " << i << ": ";

    for(int j = 0 ; j < list.length() ; j++){
        TinyTextEntity* ent = list.at(j);
        cout << ent->text().toAscii().data() << " ";
    }
    cout << endl;

}


// if the horizontal arm of one rectangle fully contains the other (example below)
//  --------         ----         -----  first
//    ----         --------       -----  second

// or we can make it overlap of spaces by 80%

bool doesConsumeX(QRect first, QRect second, int threshold){

//    int threshold = 2;

    // if one consumes another fully
    if(first.left() <= second.left() && first.right() >= second.right()){
//        cout << "First Condition " << endl;
        return true;
    }
    if(first.left() >= second.left() && first.right() <= second.right()){
//        cout << "Second Condition " << endl;
        return true;
    }

    //or if there is overlap of space by more than 80%
    // there is overlap

    int overlap;
    if(second.right() >= first.left() && first.right() >= second.left()){

        int percentage;
        if(second.right() >= first.right()) overlap = first.right() - second.left();
        else overlap = second.right() - first.left();

        //we will divide by the smaller rectangle to calculate the overlap
        if( first.width() < second.width()){

            percentage = overlap * 100 / (first.right() - first.left());

//            printRect(first);
//            cout << "overlap: " << overlap << " width: " << first.width() << endl;

        }
        else{
            percentage = overlap * 100 / (second.right() - second.left());
//            printRect(second);
//            cout << "overlap: " << overlap << " width: " << second.width() << endl;
        }

//        cout << "overlap percentage: " << percentage << endl;
        if(percentage >= threshold) return true;

    }

//    cout << "No Condition Matched " << endl;
    return false;
}



bool doesConsumeY(QRect first, QRect second, int threshold){

    // if one consumes another fully
    if(first.top() <= second.top() && first.bottom() >= second.bottom()){
        return true;
    }
    if(first.top() >= second.top() && first.bottom() <= second.bottom()){
        return true;
    }

    //or if there is overlap of space by more than 80%
    // there is overlap

    int overlap;
    if(second.bottom() >= first.top() && first.bottom() >= second.top()){

        int percentage;
        if(second.bottom() >= first.bottom()) overlap = first.bottom() - second.top();
        else overlap = second.bottom() - first.top();

        //we will divide by the smaller rectangle to calculate the overlap
        if( first.width() < second.width()){
            percentage = overlap * 100 / (first.bottom() - first.top());
        }
        else{
            percentage = overlap * 100 / (second.bottom() - second.top());
        }

        if(percentage >= threshold) return true;

    }

    return false;
}

//we are taking now the characters are horizontally next to next in current m_words, it actually is like that
void TextPage::makeWord(){

//    cout << "In makeword ............" << endl;

    TextList tmpList = d->m_words;
    TextList newList;

    TextList::Iterator it = tmpList.begin(), itEnd = tmpList.end();
    int newLeft,newRight,newTop,newBottom,newWidth,newHeight;
    int pageWidth = d->m_page->m_page->width(), pageHeight = d->m_page->m_page->height();
    int index = 0;

    //for every non-space texts(characters/words) in the textList
    for( ; it != itEnd ; it++){

        QString textString = (*it)->text().toAscii().data();
        QString newString;
        QRect lineArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);

//        cout << "first : ";
//        printRect(lineArea) ;

        int space = 0;

        while(space <= 1){

            it++;

            // we must have to put this line before the if condition of it==itEnd
            // otherwise the last character can be missed
            if(textString.length()) newString.append(textString);

            if(it == itEnd) break;

            //the first textEntity area
            QRect elementArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);

            if(!doesConsumeY(elementArea,lineArea,50)){
//                cout << "maybe y coordinates very far";
                it--;
                break;
            }


            int text_y1 = elementArea.top() ,
                    text_x1 = elementArea.left(),
                    text_y2 = elementArea.y() + elementArea.height(),
                    text_x2 = elementArea.x() + elementArea.width();

            int line_y1 = lineArea.top() ,line_x1 = lineArea.left(),
                    line_y2 = lineArea.y() + lineArea.height(),
                    line_x2 = lineArea.x() + lineArea.width();


            //we have found a space


            space = elementArea.left() - lineArea.right();
//            cout << "space " << space << " ";

//            printRect(lineArea);
//            printRect(elementArea) ;


            if(space > 1){
                it--;
                break;
            }

            newLeft = text_x1 < line_x1 ? text_x1 : line_x1;
            newRight = line_x2 > text_x2 ? line_x2 : text_x2;

            newTop = text_y1 > line_y1 ? line_y1 : text_y1;
            newBottom = text_y2 > line_y2 ? text_y2 : line_y2;

            lineArea.setLeft (newLeft);
            lineArea.setTop (newTop);
            lineArea.setWidth( newRight - newLeft );
            lineArea.setHeight( newBottom - newTop );

            textString = (*it)->text().toAscii().data();
        }

        // if newString is not empty, save it
        if(newString.length()){

            NormalizedRect newRect(lineArea,pageWidth,pageHeight);

            newList.append( new TinyTextEntity(newString.normalized
                                        (QString::NormalizationForm_KC), newRect ));

//            printRect(lineArea);

//            TinyTextEntity* ent = newList.at(index);
//            QRect finalRect = ent->area.roundedGeometry(pageWidth,pageHeight);
//            cout << "final:";
//            printRect(finalRect);
//            cout << "text: " << ent->text().toAscii().data()
//                 << " ................................ " << endl << endl;

            index++;
        }

        if(it == itEnd) break;

    }

    cout << endl << " ............................................................ " << endl;

    cout << "words: " << index << endl;

    d->m_words = newList;

//    for(int i = 0 ; i < d->m_words.length() ; i++){
//        TinyTextEntity *ent = d->m_words.at(i);
//        printRect(ent->area.roundedGeometry(pageWidth,pageHeight));
//    }

//    cout << endl;

}



void TextPage::makeAndSortLines(){


    /**
    we cannot assume that the generator will give us texts in the right order. We can only assume
    that we will get texts in the page and their bounding rectangle. The texts can be character, word,
    half-word anything. So, we need to:

    1. Sort rectangles/boxes containing texts by y0(top)
    2. Create textline where there is y overlap between TinyTextEntity 's
    3. Within each line sort the TinyTextEntity 's by x0(left)
    **/

    // Step:1 .......................................

    TextList tmpList = d->m_words;
    qSort(tmpList.begin(),tmpList.end(),compareTinyTextEntityY);

//    d->printTextList(0,tmpList);

    // Step 2: .......................................

    TextList::Iterator it = tmpList.begin(), itEnd = tmpList.end(), tmpIt = it;
    int i = 0, j = 0;
    int newLeft,newRight,newTop,newBottom,newWidth,newHeight;
    int pageWidth = d->m_page->m_page->width(), pageHeight = d->m_page->m_page->height();

    //for every non-space texts(characters/words) in the textList
    for( ; it != itEnd ; it++){

        //the textEntity area
        QRect elementArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);

        //d->m_lines in a QList of TextList and TextList is a QList of TinyTextEntity*
        // see, whether the new text should be inserted to an existing line
        bool found = false;

        //At first there will be no lines
        for( i = 0 ; i < d->m_lines.length() ; i++){

            //the line area which will be expanded
            // d->m_line_rects is only necessary to preserve the topmin and bottommax of all
            // the texts in the line, left and right is not necessary at all
            // it is in no way the actual line rectangle
            QRect lineArea = d->m_line_rects.at(i);

            int text_y1 = elementArea.top() ,
                    text_y2 = elementArea.top() + elementArea.height() ,
                    text_x1 = elementArea.left(),
                    text_x2 = elementArea.left() + elementArea.width();

            int line_y1 = lineArea.top() ,
                    line_y2 = lineArea.top() + lineArea.height(),
                    line_x1 = lineArea.left(),
                    line_x2 = lineArea.left() + lineArea.width();

            // if the new text and the line has y overlapping parts of more than 80%,
            // the text will be added to this line
            int overlap,percentage;

            // if there is overlap
            if(text_y2 >= line_y1 && line_y2 >= text_y1){

                if(text_y2 > line_y2) overlap = line_y2 - text_y1;
                else overlap = text_y2 - line_y1;

                if( (text_y2 - text_y1) > (line_y2 - line_y1) )
                    percentage = overlap * 100 / (line_y2 - line_y1);
                else percentage = overlap * 100 / (text_y2 - text_y1);

                //the overlap percentage is more than 70% of the smaller y
                if(percentage >= 70){

                    TextList tmp = d->m_lines.at(i);
                    tmp.append((*it));

                    d->m_lines.replace(i,tmp);

                    newLeft = line_x1 < text_x1 ? line_x1 : text_x1;
                    newRight = line_x2 > text_x2 ? line_x2 : text_x2;
                    newTop = line_y1 < text_y1 ? line_y1 : text_y1;
                    newBottom = text_y2 > line_y2 ? text_y2 : line_y2;

                    d->m_line_rects.replace( i, QRect( newLeft,newTop, newRight - newLeft, newBottom - newTop ) );
                    found = true;
                }
                else{
//                    cout << " percentage: " << percentage << "   text: " << (*it)->text().toAscii().data() << endl;
                }
            }

        }

        // when we have found a new line
        // create a new TextList containing only one element and append it to the m_lines
        if(!found){
            //(*it) is a TinyTextEntity*
            TextList tmp;
            tmp.append((*it));
            d->m_lines.append(tmp);
            d->m_line_rects.append(elementArea);
        }
    }

    cout << "m_lines length: " << d->m_lines.length() << endl;


    // Step 3: .......................................
    for(i = 0 ; i < d->m_lines.length() ; i++){
        TextList list = d->m_lines.at(i);

        qSort(list.begin(),list.end(),compareTinyTextEntityX);
        d->m_lines.replace(i,list);

//        d->printTextList(i,list);
//        printRect(d->m_line_rects.at(i));

    }

//    cout << endl;


    // This part is not necessary now
    // make the m_line_rects correct if it is not already
//    for(i = 0 ; i < d->m_lines.length() ; i++){
//        TextList list = d->m_lines.at(i);

//        int left = pageWidth,right = 0,top = pageHeight, bottom = 0;
//        // for every line
//        for(j = 0 ; j < list.length() ; j++){

//            TinyTextEntity* tmp = list.at(j);
//            QRect rect = tmp->area.geometry(pageWidth,pageHeight);

//            if(rect.left() < left) left = rect.left();
//            if(rect.right() > right) right = rect.right();
//            if(rect.top() < top) top = rect.top();
//            if(rect.bottom() > bottom) bottom = rect.bottom();

////            cout << "text: " << tmp->text().toAscii().data() << " ";
////            printRect(tmp->area.geometry(pageWidth,pageHeight));
//        }

//        d->m_line_rects.replace(i,QRect(QPoint(left,top),QPoint(right,bottom)));
////        d->printTextList(i,list);
//        printRect(d->m_line_rects.at(i));
//    }

}


void TextPage::createProjectionProfiles(){


}

void TextPage::XYCutForBoundingBoxes(){

    int pageWidth = d->m_page->m_page->width(), pageHeight = d->m_page->m_page->height();

    // proj_on_yaxis will start from 0(rect.left()) to N(rect.right)
    int* proj_on_yaxis, *proj_on_xaxis;  //horizontal and vertical projection respectively

    // RegionText contains a TextList and a QRect
    // The XY Tree, where the node is a RegionText
    QList<RegionText> tree;
    QRect contentRect(d->m_page->m_page->boundingBox().geometry(pageWidth,pageHeight));
    RegionText root(d->m_words,contentRect);

    // start the tree with the root, it is our only region at the start
    tree.push_back(root);

    int i = 0, j, k;

    // while traversing the tree has not been ended
    while(i < tree.length()){
        RegionText node = tree.at(i);
        QRect regionRect = node.area();


/** 1. calculation of projection profiles ................................... **/

        // allocate the size of proj profiles and initialize with 0
        int size_proj_y = node.area().height() ;
        int size_proj_x = node.area().width() ;

        proj_on_yaxis = new int[size_proj_y];
        proj_on_xaxis = new int[size_proj_x];

        cout << "size: " << size_proj_y << " " << size_proj_x << endl;

        for( j = 0 ; j < size_proj_y ; j++ ) proj_on_yaxis[j] = 0;
        for( j = 0 ; j < size_proj_x ; j++ ) proj_on_xaxis[j] = 0;


        TextList list = node.text();
        int maxX = 0 , maxY = 0;

        // for every text in the region
        for( j = 0 ; j < list.length() ; j++ ){

            TinyTextEntity *ent = list.at(j);
            QRect entRect = ent->area.geometry(pageWidth,pageHeight);

            // calculate vertical projection profile proj_on_yaxis
            // for left to right of a entity
            // increase the value of vertical projection profile by 1
            for(k = entRect.left() ; k <= entRect.left() + entRect.width() ; k++){
                proj_on_xaxis[k - regionRect.left()] += entRect.height();
            }

            // calculate vertical projection profile in the same way
            for(k = entRect.top() ; k <= entRect.top() + entRect.height() ; k++){
                proj_on_yaxis[k - regionRect.top()] += entRect.width();
            }

        }

        cout << "regionRect --> ";
        printRect(regionRect);
        cout << "width: " << regionRect.width() << " height: " << regionRect.height() << endl;
//        cout << "total Elements: " << j << endl;

//        cout << "projection on y axis " << endl << endl;
        for( j = 0 ; j < size_proj_y ; j++ ){
            if (proj_on_yaxis[j] > maxY) maxY = proj_on_yaxis[j];
//            cout << "index: " << j << " value: " << proj_on_yaxis[j] << endl;
        }
        cout << endl;

//        cout << "projection on x axis " << endl << endl;
        for( j = 0 ; j < size_proj_x ; j++ ){
            if(proj_on_xaxis[j] > maxX) maxX = proj_on_xaxis[j];
//            cout << "index: " << j << " value: " << proj_on_xaxis[j] << endl;
        }
        cout << endl;


/** 2. Cleanup Boundary White Spaces and removal of noise ..................... **/

        int xbegin = 0, xend = size_proj_x - 1;
        int ybegin = 0, yend = size_proj_y - 1;

        while(xbegin < size_proj_x && proj_on_xaxis[xbegin] <= 0){
            xbegin++;
        }
        while(xend >= 0 && proj_on_xaxis[xend] <= 0){
            xend--;
        }

        while(ybegin < size_proj_y && proj_on_yaxis[ybegin] <= 0){
            ybegin++;
        }
        while(yend >= 0 && proj_on_yaxis[yend] <= 0){
            yend--;
        }

        printRect(regionRect);
        //update the regionRect
        int old_left = regionRect.left(), old_top = regionRect.top();

        regionRect.setLeft(old_left + xbegin);
        regionRect.setRight(old_left + xend);

        regionRect.setTop(old_top + ybegin);
        regionRect.setBottom(old_top + yend);

        printRect(regionRect);

        //removal of noise (subtract from every element 5% of highest)
        int noiseX = (int)(maxX * 5 / 100), noiseY = 0;
        for( j = 0 ; j < size_proj_x ; j++ ){
            proj_on_xaxis[j] -= noiseX;
        }

        cout << "Noise on X axis: " << noiseX << endl;

        cout << "projection on x axis " << endl << endl;
        for( j = 0 ; j < size_proj_x ; j++ ){
            if(proj_on_xaxis[j] > maxX) maxX = proj_on_xaxis[j];
//            cout << "index: " << j << " value: " << proj_on_xaxis[j] << endl;
        }
        cout << endl;


/** 3. Get the Widest gap(<= 0 train)  ........................................ **/

        //find gap in y-axis projection
        int gap_hor = -1, pos_hor = -1;
        int begin = -1, end = -1;

        // find all hor_gaps and find the maximum between them
        for(j = 1 ; j < size_proj_y ; j++){

            //transition from white to black
            if(begin >= 0 && proj_on_yaxis[j-1] <= 0
                    && proj_on_yaxis[j] > 0){
                end = j;
            }

            //transition from black to white
            if(proj_on_yaxis[j-1] > 0 && proj_on_yaxis[j] <= 0)
                begin = j;

            if(begin > 0 && end > 0 && end-begin > gap_hor){
                gap_hor = end - begin;
                pos_hor = (end + begin) / 2;
                begin = -1;
                end = -1;
            }
        }


        begin = -1, end = -1;
        int gap_ver = -1, pos_ver = -1;

        //find all the ver_gaps and find the maximum between them
        for(j = 1 ; j < size_proj_x ; j++){

            //transition from white to black
            if(begin >= 0 && proj_on_xaxis[j-1] <= 0
                    && proj_on_xaxis[j] > 0){
                end = j;
            }

            //transition from black to white
            if(proj_on_xaxis[j-1] > 0 && proj_on_xaxis[j] <= 0)
                begin = j;

            if(begin > 0 && end > 0 && end-begin > gap_ver){
                gap_ver = end - begin;
                pos_ver = (end + begin) / 2;
                begin = -1;
                end = -1;
            }
        }

        int cut_pos_x = pos_ver, cut_pos_y = pos_hor;
        int gap_x = gap_ver, gap_y = gap_hor;

        cout << "gap X: " << gap_x << endl;
        cout << "gap Y: " << gap_y << endl;
        cout << "cut X: " << cut_pos_x << endl;
        cout << "cut Y: " << cut_pos_y << endl;


/** 4. Cut the region and make nodes (left,right) or (up,down) ................ **/

        //these can be calculated according to space characteristics
        int tcx = 0, tcy = 0;
        bool cut_hor = false, cut_ver = false;

        // For horizontal cut
        QRect topRect(regionRect.left(),
            regionRect.top(),
            regionRect.width(),
            cut_pos_y);
        QRect bottomRect(regionRect.left(),
            regionRect.top() + cut_pos_y,
            regionRect.width(),
            regionRect.height() - cut_pos_y);

        // For vertical Cut
        QRect leftRect(regionRect.left(),
            regionRect.top(),
            cut_pos_x,
            regionRect.height());
        QRect rightRect(regionRect.left() + cut_pos_x,
            regionRect.top(),
            regionRect.width() - cut_pos_x,
            regionRect.height());


        // horizontal split (top rect, bottom rect)
        printRect(regionRect);
        if(gap_y >= gap_x && gap_y > tcy){
            printRect(topRect);
            printRect(bottomRect);
            cut_hor = true;
//            goto split_rect;
        }
        //vertical cut (left rect, right rect)
        else if(gap_y >= gap_x && gap_y <= tcy && gap_x > tcx){
            printRect(leftRect);
            printRect(bottomRect);
            cut_ver = true;
        }
        //vertical cut
        else if(gap_x >= gap_y && gap_x > tcx){
            printRect(leftRect);
            printRect(bottomRect);
            cut_ver = true;
        }
        //horizontal cut
        else if(gap_x >= gap_y && gap_x <= tcx && gap_y > tcy){
            printRect(topRect);
            printRect(bottomRect);
            cut_hor = true;
        }
        //no cut possible
        else{
            i++;
        }

        TextList list1,list2;
        // now we need to create two new regionRect
        //horizontal cut, topRect and bottomRect
        if(cut_hor){
            cout << "horizontal cut: " << endl;
            cout << "list: " << list.length() << endl;

            for( j = 0 ; j < list.length() ; j++ ){

//                cout << j << endl;

                TinyTextEntity *ent = list.at(j);
                QRect entRect = ent->area.geometry(pageWidth,pageHeight);

                QString str(ent->text());
                NormalizedRect rect(entRect,pageWidth,pageHeight);

                if(topRect.intersects(entRect)){
//                    list1.append(ent);
                    list1.append( new TinyTextEntity(str.normalized
                        (QString::NormalizationForm_KC), rect ));
                }
                else list2.append( new TinyTextEntity(str.normalized
                        (QString::NormalizationForm_KC), rect ));
            }

            RegionText node1(list1,topRect);
            RegionText node2(list2,bottomRect);

            tree.replace(i,node1);
            tree.insert(i+1,node2);

            list1 = tree.at(i).text();
            list2 = tree.at(i+1).text();

            cout << "list1: " << list1.length() << endl;
            cout << "list2: " << list2.length() << endl;

            cout << "Node1 text: ........................ " << endl << endl;
            for(j = 0 ; j < list1.length() ; j++){
                TinyTextEntity *ent = list1.at(j);
                cout << ent->text().toAscii().data();
            }
            cout << endl;

            cout << "Node2 text: ........................ " << endl << endl;
            for(j = 0 ; j < list2.length() ; j++){
                TinyTextEntity *ent = list2.at(j);
                cout << ent->text().toAscii().data();
            }
            cout << endl;
        }

        //vertical cut, leftRect and rightRect
        else if(cut_ver){
            cout << "vertical cut" << endl;
            for( j = 0 ; j < list.length() ; j++ ){

                TinyTextEntity *ent = list.at(j);
                QRect entRect = ent->area.geometry(pageWidth,pageHeight);

                if(leftRect.intersects(entRect))
                    list1.append(ent);
                else list2.append(ent);
            }
            RegionText node1(list1,leftRect);
            RegionText node2(list2,rightRect);

            tree.replace(i,node1);
            tree.insert(i+1,node2);
        }

        else
            cout << "no cut " << endl;

//        delete []proj_on_yaxis;
//        delete []proj_on_xaxis;

//        i++;

        cout << "i: " << i << " ............................................ " << endl;

    }


}



//correct the textOrder, all layout recognition works here
void TextPage::correctTextOrder(){

    // create words from characters
    makeWord();

    XYCutForBoundingBoxes();

    // create primary lines from words
    makeAndSortLines();

//    cout << "After makeword and makeAndSortLines() ..................................... " << endl;

//    for(int i = 0 ; i < d->m_lines.length() ; i++){
//        TextList list = d->m_lines.at(i);
//        d->printTextList(i,list);
//    }


    /**

    Firt Part: Separate text lines using column detection

    1. Make character analysis to differentiate between word spacing and column spacing.
    2. Break the lines if there is some column spacing somewhere in the line and also calculate
       the column spacing rectangle if necessary.
    3. Find if some line contains more than one lines (it can happend if in the left column there is some
       Big Text like heading and in the right column there is normal texts, so several normal lines from
       right can be erroneously inserted in same line in merged position)

       For those lines first sort them again using yoverlap and then x ordering

    **/

    /** Step 1: ........................................................................ **/

    //we would like to use QMap instead of QHash as it will keep the keys sorted
    QMap<int,int> hor_space_stat;   //this is to find word spacing
    QMap<int,int> col_space_stat;   //this is to find column spacing

    QList< QList<QRect> > space_rects; // to save all the word spacing or column spacing rects
    QList<QRect> max_hor_space_rects;

    int i,j;
    int pageWidth = d->m_page->m_page->width(), pageHeight = d->m_page->m_page->height();

    // space in every line
    for(i = 0 ; i < d->m_lines.length() ; i++){
        // list contains a line
        TextList list = d->m_lines.at(i);
        QList<QRect> line_space_rects;

        int maxSpace = 0, minSpace = pageWidth;

        // for every TinyTextEntity element in the line
        TextList::Iterator it = list.begin(), itEnd = list.end();
        QRect max_area1,max_area2;
        QString before_max, after_max;

//        d->printTextList(i,list);

        // for every line
        for( ; it != itEnd ; it++ ){
//            cout << (*it)->text().toAscii().data() << endl;

            QRect area1 = (*it)->area.roundedGeometry(pageWidth,pageHeight);
            if( it+1 == itEnd ) break;
//            printRect(area1);

            QRect area2 = (*(it+1))->area.roundedGeometry(pageWidth,pageHeight);
            int space = area2.left() - area1.right();
//            printRect(area2);

            if(space > maxSpace){
                max_area1 = area1;
                max_area2 = area2;

                maxSpace = space;

                before_max = (*it)->text();
                after_max = (*(it+1))->text();
            }

//            cout << (*it)->text().toAscii().data() << " " << (*(it+1))->text().toAscii().data();
//            cout << "  space: " << space << endl;

            if(space < minSpace && space != 0) minSpace = space;

            //if we found a real space, whose length is not zero and also less than the pageWidth
            if(space != 0 && space != pageWidth){

                // increase the count of the space amount
                if(hor_space_stat.contains(space)) hor_space_stat[space] = hor_space_stat[space]++;
                else hor_space_stat[space] = 1;

                //if we have found a space, put it in a list of rectangles
                int left,right,top,bottom;

                left = area1.right();
                right = area2.left();

                top = area2.top() < area1.top() ? area2.top() : area1.top();
                bottom = area2.bottom() > area1.bottom() ? area2.bottom() : area1.bottom();

                QRect rect(left,top,right-left,bottom-top);
                line_space_rects.append(rect);

//                cout << space << " ";
            }
//            cout << "space: " << space << " " <<  area1.right() << " " << area2.left() << endl;
        }


//        cout << endl <<  "maxSpace " << maxSpace <<  " ----------------------------------------------- " << endl << endl;

        space_rects.append(line_space_rects);

        if(hor_space_stat.contains(maxSpace)){
            if(hor_space_stat[maxSpace] != 1)
                hor_space_stat[maxSpace] = hor_space_stat[maxSpace]--;
            else hor_space_stat.remove(maxSpace);
        }

        if(maxSpace != 0){
            if (col_space_stat.contains(maxSpace))
                col_space_stat[maxSpace] = col_space_stat[maxSpace]++;
            else col_space_stat[maxSpace] = 1;

            //store the max rect of each line
            int left,right,top,bottom;
            left = max_area1.right();
            right = max_area2.left();

            max_area1.top() > max_area2.top() ? top = max_area2.top() : top = max_area1.top();
            max_area1.bottom() < max_area2.bottom() ? bottom = max_area2.bottom() : bottom = max_area1.bottom();

            QRect rect(left,top,right-left,bottom-top);
            max_hor_space_rects.append(rect);

//            printRect(rect);
//            cout << before_max.toAscii().data() << "    "
//                 << after_max.toAscii().data() << endl;

        }
        else max_hor_space_rects.append(QRect(0,0,0,0));
//        cout << endl;
//        cout << minSpace << " "<< maxSpace << endl;
    }


    // All the between word space counts are in hor_space_stat

    int word_spacing;
    cout << "Word Spacing: " << endl;
    QMapIterator<int, int> iterate(hor_space_stat);
    while (iterate.hasNext()) {
        iterate.next();
        cout << iterate.key() << ": " << iterate.value() << endl;
    }

    int col_spacing = 0;
    cout << "Column Spacing: " << endl;
    QMapIterator<int, int> iterate_col(col_space_stat);
    while (iterate_col.hasNext()) {
        iterate_col.next();
        cout << iterate_col.key() << ": " << iterate_col.value() << endl;
        if(iterate_col.value() > col_spacing) col_spacing = iterate_col.value();
    }


    //show all space rects (between words, word spacing or column spacing)
//    for( i = 0 ; i < space_rects.length() ; i++){

//        QList<QRect> rectList = space_rects.at(i);

//        for( j = 0 ; j < rectList.length() ; j++){

//            QRect rect = rectList.at(j);
//            cout << "space: " << rect.width() << " " << endl;
//            printRect(rect);

//        }
//    }

//    cout << "max_hor_space_rectangle:" << endl;
//    for(j = 0 ; j < max_hor_space_rects.length() ; j++){

//        QRect rect = max_hor_space_rects.at(j);
//        printRect(rect);
//    }



    /** Step 2: ........................................................................ **/

    /**
     We will start with the max whitespace rectangle within the first line, if any. Then, we will
    get the max whitespace rectangle of the second line.

    If both of them are at the same position, we can say, they creates a column.
    Else we will check

        if there is any whitespace rectangle under the previous line's maximum whitespace
        rectangle. In this cae, we can say its a noisy line, which do not preserve the column
        separation. if we find 3(col_threshold) lines of this type consecutively, we can break
        the column separation, and say that these 3 lines fully are in the same column.

        else, the line is a single line in a column. We do not need to separate this.

    **/

//    cout << endl << endl << "Step 2 ............................................... " << endl << endl;
    int length_line_list = d->m_lines.length();
    bool consume12 = false, consume23 = false, consume13 = false;

    for(i = 0 ; i < length_line_list ; i++){

        consume12 = consume23 = consume13 = false;
        int index1, index2, index3;

        index1 = i % length_line_list;
        index2 = (i + 1) % length_line_list;
        index3 = (i + 2) % length_line_list;

        // We will take 3 lines at a time, so that one noisy data do not give wrong idea.
        // We will see whether they creates a column or not
        TextList line1 = d->m_lines.at(index1);
        TextList line2 = d->m_lines.at(index2);
//        TextList line3 = d->m_lines.at(index3);

        // the estimated column space rectangles of those lines
        QRect columnRect1 = max_hor_space_rects.at(index1);
        QRect columnRect2 = max_hor_space_rects.at(index2);
//        QRect columnRect3 = max_hor_space_rects.at(index3);

//        cout << i << ": ";
//        printRect(columnRect1);
//        printRect(columnRect2);

        // if the line itself has no space
        if(columnRect1.isEmpty()){
            continue;
        }

        // if the line following has no space, then see the next line
        if(columnRect2.isEmpty()){
            columnRect2 = max_hor_space_rects.at(index3);
            line2 = d->m_lines.at(index3);
        }

        QRect rect1,rect2,rect3;

        //if the maxRectangle of line1 and line2 are at the same place, they may create a column
        if(doesConsumeX(columnRect1,columnRect2,90)){
            consume12 = true;
            rect1 = columnRect1;
            rect2 = columnRect2;

//            cout << "true !!!!!!!!!!!!!! ---- 1" << endl;

//            d->printTextList(index1,line1);
//            cout << "rect1: " << columnRect1.left() << " , " << columnRect1.right() << endl;

//            d->printTextList(index2,line2);
//            cout << "rect2: " << columnRect2.left() << " , " << columnRect2.right() << endl;

        }
        /** else if one of the lines is noisy and do not maintain column spacing correctly,
            so that, maxSpacing is not column spacing but, some other word spacing, so we search
            if some rectangle smaller than some word spacing rectangle remains which is
            consumed by the other lines maxSpacing rectangle.
        **/
        else{
            //1. see whether maxSpacing of line1 consumes any space rectangle in line2
            rect1 = columnRect1;
            QList<QRect> line2_space_rect = space_rects.at(index2);

            for(j = 0 ; j < line2_space_rect.length() ; j++){
                rect2 = line2_space_rect.at(j);
                if(doesConsumeX(rect1,rect2,90)){
                    consume12 = true;
//                    cout << "true !!!!!!!!!!!!!! ---- 2" << endl;
                    break;
                }
            }

            //2. see whether maxSpacing of line2 consumes any space rectangle in line1
            rect2 = columnRect2;
            QList<QRect> line1_space_rect = space_rects.at(index1);

            for(j = 0 ; j < line1_space_rect.length(); j++){

                if(consume12){
//                    cout << "true !!!!!!!!!!!!!! ---- 3" << endl;
                    break;
                }

                rect1 = line1_space_rect.at(j);
                if(doesConsumeX(rect1,rect2,90)){
                    //we need to update the maxSpace rect, otherwise the cut will be in the wrong place
//                    max_hor_space_rects.replace(index1,rect1);
                    consume12 = true;
                }

            }

        }

    /** if consume12 is still false, then we do not get some column spacing,
    the spacing are random, so, possibly line1 and line2 are not column separated
    lines and we don't need to split them.
    **/

        /** possibly we have got a column separator, so, we break the lines in two parts, and
         1. edit previous lines(delete the part after column separator)
         2. add a new line and append them to the last of the list
        **/
        if(consume12){
            //the separating rectangles are rect1 and rect2
            QRect linerect1 = d->m_line_rects.at(i),linerect2 = linerect1;
            TextList tmp;
            TinyTextEntity* tmp_entity;

//            cout << "cut rectangle: " ;
//            printRect(rect1);
//            printRect(rect2);

            for(j = line1.length() - 1 ; j >= 0 ; j --){

                tmp_entity = line1.at(j);
                QRect area = tmp_entity->area.roundedGeometry(pageWidth,pageHeight);

                // we have got maxSpace rect
                int rect1_right = rect1.left() + rect1.width();
                if(rect1_right == area.left()){
                    linerect1.setRight(rect1.left());
                    linerect2.setLeft(rect1.right());

                    tmp.push_front(tmp_entity);
                    line1.pop_back();

                    break;
                }

                //push in front in the new line and pop from the back of the old line
                tmp.push_front(tmp_entity);
                line1.pop_back();

            }

            d->m_lines.replace(i,line1);
            d->m_line_rects.replace(i,linerect1);

            d->m_lines.append(tmp);
            d->m_line_rects.append(linerect2);

//            d->printTextList(i,d->m_lines.at(i));
//            printRect(d->m_line_rects.at(i));
//            d->printTextList(length_line_list + i,d->m_lines.at(i+length_line_list));
        }

    }


    cout << endl << "After a lot of processing done lines are: ................................ " << endl << endl;

    // copies all elements to a TextList
    TextList tmpList;
    for(i = 0 ; i < d->m_lines.length() ; i++){
        TextList list = d->m_lines.at(i);
//        d->printTextList(i,list);
        for(j = 0 ; j < list.length() ; j++){
            TinyTextEntity* ent = list.at(j);
            tmpList.append(ent);
        }
    }

    cout << "print Done" << endl;

//    d->m_words = tmpList;


    /**
    Second Part: Now we have Text Lines in our hand, we have to find their reading order. We will need to consider both
    the horizontal spacing and vertical spacing here. We need the concept of line spacing here.
    **/

    //Find Line spacing/ Vertical spacing for row separators
    //It will be necessary for reading order detection


}


//add necessary spaces in the text - mainly for copy purpose so that words are visible
void TextPage::addNecessarySpace(){

}
