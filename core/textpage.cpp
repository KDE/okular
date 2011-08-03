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

/** mamun.nightcrawler@gmail.com **/
void printRect(QRect rect){

    cout << "l: " << rect.left() << " r: " << rect.x() + rect.width() << " t: " << rect.top() <<
            " b: " << rect.y() + rect.height() << endl;
}


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
    RegionText(){};

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

    inline void setArea(QRect area){
        m_area = area;
    }

    inline void setText(TextList text){
        m_region_text = text;
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
    qDeleteAll( m_spaces );
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
    //minX,maxX,minY,maxY gives the bounding rectangle coordinates of the document
    double minX, maxX, minY, maxY;
    double scaleX = this->d->m_page->m_page->width();
    double scaleY = this->d->m_page->m_page->height();

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


    NormalizedRect boundingRect = d->m_page->m_page->boundingBox();
    QRect content = boundingRect.geometry(scaleX,scaleY);

    minX = content.left(), maxX = content.right();
    minY = content.top(), maxY = content.bottom();


    /**
     we will now find out the TinyTextEntity for the startRectangle and TinyTextEntity for
     the endRectangle .. we have four cases

     Case 1(a): both startpoint and endpoint are out of the bounding Rectangle and at one side, so the rectangle made of start
     and endPoint are outof the bounding rect (do not intersect)

     Case 1(b): both startpoint and endpoint are out of bounding rect, but they are in different side, so is their rectangle

     Case 2(a): find the rectangle which contains start and endpoint and having some
     TextEntity

     Case 2(b): if 2(a) fails (if startPoint and endPoint both are unchanged), then we check whether there is any
     TextEntity within the rect made by startPoint and endPoint

     Case 3:
     Now, we may have two type of selection.
     1. startpoint is left-top of start_end and endpoint is right-bottom
     2. startpoint is left-bottom of start_end and endpoint is top-right

     Also, as 2(b) is passed, we might have it,itEnd or both unchanged, but the fact is that we have
     text within them. so, we need to search for the best suitable textposition for start and end.

     Case 3(a): We search the nearest rectangle consisting of some
     TinyTextEntity right to or bottom of the startPoint for selection 01.
     And, for selection 02, we have to search for right and top

     Case 3(b): For endpont, we have to find the point top of or left to
     endpoint if we have selection 01.
     Otherwise, the search will be left and bottom
    **/

    // we know that startC.x > endC.x, we need to decide which is top and which is bottom

    NormalizedRect start_end;
    if(startC.y < endC.y)
        start_end = NormalizedRect(startC.x, startC.y, endC.x, endC.y);
    else start_end = NormalizedRect(startC.x, endC.y, endC.x, startC.y);

    //Case 1(a) .......................................
    if(!boundingRect.intersects(start_end)) return ret;

    // case 1(b) ......................................
    // Move the points to boundary
    /**
        note that, after swapping of start and end, we know that,
        start is always left to end. but, we cannot say start is
        positioned upper than end.
    **/
    else{
        // if start is left to content rect take it to content rect boundary
        if(startC.x * scaleX < minX) startC.x = minX/scaleX;
        if(endC.x * scaleX > maxX) endC.x = maxX/scaleX;

        // if start is top to end (selection type 01)
        if(startC.y * scaleY < minY) startC.y = minY/scaleY;
        if(endC.y * scaleY > maxY) endC.y = maxY/scaleY;

        // if start is bottom to end (selection type 02)
        if(startC.y * scaleY > maxY) startC.y = maxY/scaleY;
        if(endC.y * scaleY < minY) endC.y = minY/scaleY;

    }


    TextList::ConstIterator it = d->m_words.constBegin(), itEnd = d->m_words.constEnd();
    TextList::ConstIterator start = it, end = itEnd, tmpIt = it, tmpItEnd = itEnd;
    const MergeSide side = d->m_page ? (MergeSide)d->m_page->m_page->totalOrientation() : MergeRight;

    //case 2(a) ......................................
    for ( ; it != itEnd; ++it )
    {
        // (*it) gives a TinyTextEntity*
        tmp = (*it)->area;
        if(tmp.contains(startCx,startCy)) start = it;
        if(tmp.contains(endCx,endCy)) end = it;
    }

//    if(it != start && end != itEnd) goto POST_PROCESSING;

    //case 2(b) ......................................
    it = tmpIt;
    if(start == it && end == itEnd){

        for ( ; it != itEnd; ++it )
        {
            // is there any text reactangle within the start_end rect
            tmp = (*it)->area;
            if(start_end.intersects(tmp))
                break;
        }

        // we have searched every text entities, but none is within the rectangle created by start and end
        // so, no selection should be done
        if(it == itEnd){
            return ret;
        }

    }

    cout << "startPoint: " << startC.x * scaleX << "," << startC.y * scaleY << endl;
    cout << "endPoint: " << endC.x * scaleX << "," << endC.y * scaleY << endl;

    //case 3.a 01
    if(start == it){

        it = tmpIt;
        bool flagV = false;
        NormalizedRect rect;

        // selection type 01
        if(startC.y <= endC.y){

            cout << "start First .... "  << endl;

            for ( ; it != itEnd; ++it ){

                rect= (*it)->area;
                rect.isBottom(startC) ? flagV = false: flagV = true;

                if(flagV && rect.isRight(startC)){
                    start = it;
                    break;
                }
            }

            cout << "startText: " << (*start)->text().toAscii().data() << endl;
        }

        //selection type 02
        else{

            //                TextList::ConstIterator tmpStart = end, tmpEnd = start;
            cout << "start Second .... "  << endl;
            int distance = scaleX + scaleY + 100;

            for ( ; it != itEnd; ++it ){

                rect= (*it)->area;

//                if(rect.isTop(startC)) break;
                if(rect.isBottom(startC) && rect.isLeft(startC)){

                    QRect entRect = rect.geometry(scaleX,scaleY);

                    int xdist, ydist;
                    xdist = entRect.center().x() - startC.x * scaleX;
                    ydist = entRect.center().y() - startC.y * scaleY;

                    //make them positive
                    if(xdist < 0) xdist = -xdist;
                    if(ydist < 0) ydist = -ydist;

                    if( (xdist + ydist) < distance){
                        distance = xdist+ ydist;
                        start = it;
                    }

                }

            }

            cout << "startText: " << (*start)->text().toAscii().data() << endl;

        }

    }

    //case 3.b 01
    if(end == itEnd){
        it = tmpIt; //start
        itEnd = itEnd-1;

        bool flagV = false;
        NormalizedRect rect;

        if(startC.y <= endC.y){

            for ( ; itEnd >= it; itEnd-- ){

                rect= (*itEnd)->area;
                rect.isTop(endC) ? flagV = false: flagV = true;

                if(flagV && rect.isLeft(endC)){
                    end = itEnd;
                    break;
                }

            }
            cout << "end First Text: " << (*end)->text().toAscii().data() << endl;
        }

        else{

            int distance = scaleX + scaleY + 100;
            for ( ; itEnd >= it; itEnd-- ){

                rect= (*itEnd)->area;

                if(rect.isTop(endC) && rect.isRight(endC)){

                    QRect entRect = rect.geometry(scaleX,scaleY);

                    int xdist, ydist;
                    xdist = entRect.center().x() - endC.x * scaleX;
                    ydist = entRect.center().y() - endC.y * scaleY;

                    //make them positive
                    if(xdist < 0) xdist = -xdist;
                    if(ydist < 0) ydist = -ydist;

                    if( (xdist + ydist) < distance){
                        distance = xdist+ ydist;
                        end = itEnd;
                    }

                }
            }

            cout << "end second Text: " << (*end)->text().toAscii().data() << endl;

        }

    }

    POST_PROCESSING:

    //if start is less than end swap them
    if(start > end){

        it = start;
        start = end;
        end = it;
    }

    cout << "start: " << (*start)->text().toAscii().data() << endl;
    cout << "end: " << (*end)->text().toAscii().data() << endl;


    //removes the possibility of crash, in case none of 1 to 3 is true
    if(end == d->m_words.constEnd()) end--;

    for( ;start <= end ; start++){
        ret->appendShape( (*start)->transformedArea( matrix ), side );
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

void TextPagePrivate::printTextPageContent(){
    // tList is our textList for this text page
    // TextList is of type List<TinyTextEntity* >
    TextList tList = m_words;

    foreach(TinyTextEntity* tiny, tList){
        cout << tiny->text().toAscii().data();
        QRect rect = tiny->area.roundedGeometry(m_page->m_page->width(),m_page->m_page->height());
        cout << " area: " << rect.top() << "," << rect.left() << " " << rect.bottom() << "," << rect.right() << endl;
    }

}


/** mamun_nightcrawler@gmail.com **/

//remove all the spaces between texts, it will keep all the generators same, whether they save spaces or not
void TextPagePrivate::removeSpace(){

    TextList::Iterator it = m_words.begin(), itEnd = m_words.end();
    QString str(' ');

    it = m_words.begin(), itEnd = m_words.end();
    for( ; it != itEnd ; it++){
        //if TextEntity contains space
        if((*it)->text() == str){

            // create new Entity, otherwise there might be possible memory leakage
            m_spaces.append( new TinyTextEntity( (*it)->text(),(*it)->area ) );
            this->m_words.erase(it);

        }
    }

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

bool compareRegionTextY(RegionText first, RegionText second){
    return first.area().top() < second.area().top();
}

bool compareRegionTextX(RegionText first, RegionText second){
    return first.area().left() < second.area().left();
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

//copies a TextList to m_words with the same pointer
void TextPagePrivate::copyTo(TextList &list){

    TextList::Iterator it = m_words.begin(), itEnd = m_words.end();
    for( ; it != itEnd ; it++){
        m_words.erase(it);
    }

    for(int i = 0 ; i < list.length() ; i++){
        TinyTextEntity *ent = list.at(i);
        m_words.append( new TinyTextEntity(ent->text(),ent->area) );
    }

}

// copies from m_words to list with distince pointers
void TextPagePrivate::copyFrom(TextList &list){

    TextList::Iterator it = list.begin(), itEnd = list.end();
    for( ; it != itEnd ; it++){
        list.erase(it);
    }

    for(int i = 0 ; i < m_words.length() ; i++){
        TinyTextEntity* ent = m_words.at(i);
        list.append( new TinyTextEntity( ent->text(),ent->area ) );
    }
}

// if the horizontal arm of one rectangle fully contains the other (example below)
//  --------         ----         -----  first
//    ----         --------       -----  second
// or we can make it overlap of spaces by threshold%

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

    //or if there is overlap of space by more than threshold%
    // there is overlap

    int overlap;
    if(second.right() >= first.left() && first.right() >= second.left()){

        int percentage;
        if(second.right() >= first.right()) overlap = first.right() - second.left();
        else overlap = second.right() - first.left();

        //we will divide by the smaller rectangle to calculate the overlap
        if( first.width() < second.width()){

            percentage = overlap * 100 / (first.right() - first.left());

        }
        else{
            percentage = overlap * 100 / (second.right() - second.left());
        }

        if(percentage >= threshold) return true;

    }

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
void TextPagePrivate::makeWordFromCharacters(){

    TextList tmpList;
    TextList newList;

    // we are making a new copy from m_words to tmpList before using it
    copyFrom(tmpList);

    TextList::Iterator it = tmpList.begin(), itEnd = tmpList.end(), tmpIt;
    int newLeft,newRight,newTop,newBottom;
    int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();
    int index = 0;


    //for every non-space texts(characters/words) in the textList
    for( ; it != itEnd ; it++){

        QString textString = (*it)->text().toAscii().data();
        QString newString;
        QRect lineArea = (*it)->area.roundedGeometry(pageWidth,pageHeight),elementArea;

        TextList word;  //It will contain all the TextEntities in a simple word
        tmpIt = it;
        int space = 0;

        while(!space ){

            if(textString.length()){
                newString.append(textString);

                // when textString is the start of the word, it contains the lineArea
                if(tmpIt == it){
                    NormalizedRect newRect(lineArea,pageWidth,pageHeight);
                    word.append(new TinyTextEntity(textString.normalized
                                                   (QString::NormalizationForm_KC), newRect));
                }
                else{
                    NormalizedRect newRect(elementArea,pageWidth,pageHeight);
                    word.append(new TinyTextEntity(textString.normalized
                                                   (QString::NormalizationForm_KC), newRect));
                }
            }

            it++;

            // we must have to put this line before the if condition of it==itEnd
            // otherwise the last character can be missed
            if(it == itEnd) break;

            //the first textEntity area
            elementArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);

            if(!doesConsumeY(elementArea,lineArea,60)){
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


            space = elementArea.left() - lineArea.right();
//            cout << "space " << space << " ";

            if(space > 0 || space < 0){
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
            newList.append(new TinyTextEntity(newString.normalized
                                              (QString::NormalizationForm_KC), newRect ));


            QRect rect = newRect.geometry(pageWidth,pageHeight);
            RegionText regionWord(word,rect);
            int keyRect = rect.left() * rect.top()
                    + rect.right() * rect.bottom();

            // there may be more than one element in the same key
            m_word_chars_map.insertMulti(keyRect,regionWord);

            index++;
        }

        if(it == itEnd) break;

    }

    cout << "words: " << index << endl;

    copyTo(newList);

//    for(int i = 0 ; i < m_words.length() ; i++){

//        TinyTextEntity *ent = m_words.at(i);
//        QRect entArea = ent->area.geometry(pageWidth,pageHeight);
//        int key = entArea.top() * entArea.left() + entArea.right() * entArea.bottom();

//        RegionText text_list = m_word_chars_map.value(key);
//        TextList list = text_list.text();

//        cout << "key: " << key << " text: ";
//        for( int l = 0 ; l < list.length() ; l++){
//            ent = list.at(l);
//            cout << ent->text().toAscii().data();
//        }
//        cout << endl;
//    }
    // Pointers to element in tmpList and newList are different
    qDeleteAll(tmpList);
    qDeleteAll(newList);
}


void TextPagePrivate::makeAndSortLines(TextList &wordsTmp, SortedTextList &lines, LineRect &line_rects){

    /**
    we cannot assume that the generator will give us texts in the right order. We can only assume
    that we will get texts in the page and their bounding rectangle. The texts can be character, word,
    half-word anything. So, we need to:

    1. Sort rectangles/boxes containing texts by y0(top)
    2. Create textline where there is y overlap between TinyTextEntity 's
    3. Within each line sort the TinyTextEntity 's by x0(left)
    **/

    // Make a new copy of the TextList in the words, so that the wordsTmp and lines do not contain
    // same pointers for all the TinyTextEntity
    TextList words;
    for(int i = 0 ; i < wordsTmp.length() ; i++){
        TinyTextEntity* ent = wordsTmp.at(i);
        words.append( new TinyTextEntity( ent->text(),ent->area ) );
    }

    // Step:1 .......................................
    qSort(words.begin(),words.end(),compareTinyTextEntityY);


    // Step 2: .......................................

    TextList::Iterator it = words.begin(), itEnd = words.end();
    int i = 0;
    int newLeft,newRight,newTop,newBottom;
    int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();


    //for every non-space texts(characters/words) in the textList
    for( ; it != itEnd ; it++){

        //the textEntity area
        QRect elementArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);

        //lines in a QList of TextList and TextList is a QList of TinyTextEntity*
        // see, whether the new text should be inserted to an existing line
        bool found = false;

        //At first there will be no lines
        for( i = 0 ; i < lines.length() ; i++){

            //the line area which will be expanded
            // line_rects is only necessary to preserve the topmin and bottommax of all
            // the texts in the line, left and right is not necessary at all
            // it is in no way the actual line rectangle
            QRect lineArea = line_rects.at(i);

            int text_y1 = elementArea.top() ,
                    text_y2 = elementArea.top() + elementArea.height() ,
                    text_x1 = elementArea.left(),
                    text_x2 = elementArea.left() + elementArea.width();

            int line_y1 = lineArea.top() ,
                    line_y2 = lineArea.top() + lineArea.height(),
                    line_x1 = lineArea.left(),
                    line_x2 = lineArea.left() + lineArea.width();


            // if the font sizes vary very much, they will not make a line
            if(lineArea.height() > 2 * elementArea.height()) continue;

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
                if(percentage >= 80){

                    TextList tmp = lines.at(i);
                    tmp.append((*it));

                    lines.replace(i,tmp);

                    newLeft = line_x1 < text_x1 ? line_x1 : text_x1;
                    newRight = line_x2 > text_x2 ? line_x2 : text_x2;
                    newTop = line_y1 < text_y1 ? line_y1 : text_y1;
                    newBottom = text_y2 > line_y2 ? text_y2 : line_y2;

                    line_rects.replace( i, QRect( newLeft,newTop, newRight - newLeft, newBottom - newTop ) );
                    found = true;
                }

            }

        }

        // when we have found a new line
        // create a new TextList containing only one element and append it to the lines
        if(!found){
            //(*it) is a TinyTextEntity*
            TextList tmp;
            tmp.append((*it));
            lines.append(tmp);
            line_rects.append(elementArea);
        }
    }


    // Step 3: .......................................
    for(i = 0 ; i < lines.length() ; i++){
        TextList list = lines.at(i);

        qSort(list.begin(),list.end(),compareTinyTextEntityX);
        lines.replace(i,list);

//        printTextList(i,list);
    }

    //we cannot delete words here, as lines contains the same pointers as words does

}


void TextPagePrivate::XYCutForBoundingBoxes(int tcx, int tcy){

    int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();

    // proj_on_yaxis will start from 0(rect.left()) to N(rect.right)
    int proj_on_yaxis[5000], proj_on_xaxis[5000];  //horizontal and vertical projection respectively

    // RegionText contains a TextList and a QRect
    // The XY Tree, where the node is a RegionText
    RegionTextList tree;
    QRect contentRect(m_page->m_page->boundingBox().geometry(pageWidth,pageHeight));

    //creating a copy of m_words in words so that we do not have same pointers
    TextList words;

    copyFrom(words);
    RegionText root(words,contentRect);

    // start the tree with the root, it is our only region at the start
    tree.push_back(root);

    int i = 0, j, k;
    int countLoop = 0;

    cout << "content Rect: ";
    printRect(contentRect);

    // while traversing the tree has not been ended
    while(i < tree.length()){

        RegionText node = tree.at(i);
        QRect regionRect = node.area();

        cout << "i: " << i << " .......................... " << endl;

/** 1. calculation of projection profiles ................................... **/

        // allocate the size of proj profiles and initialize with 0
        int size_proj_y = node.area().height() ;
        int size_proj_x = node.area().width() ;

        for( j = 0 ; j < size_proj_y ; j++ ) proj_on_yaxis[j] = 0;
        for( j = 0 ; j < size_proj_x ; j++ ) proj_on_xaxis[j] = 0;


        TextList list = node.text();
        int word_spacing = 0,line_spacing = 0, column_spacing = 0;
        SortedTextList lines;
        LineRect line_rects;

        // Calculate tcx and tcy locally for each new region
        if(countLoop++){
            makeAndSortLines(list,lines,line_rects);
            calculateStatisticalInformation(lines,line_rects,word_spacing,line_spacing,column_spacing);
            tcx = word_spacing * 2, tcy = line_spacing * 2;
        }

        int maxX = 0 , maxY = 0;
        int avgX = 0, avgY = 0;
        int count;

        cout << "Noise: tcx: " << tcx << " tcy: " << tcy << endl;

        // for every text in the region
        for( j = 0 ; j < list.length() ; j++ ){

            TinyTextEntity *ent = list.at(j);
            QRect entRect = ent->area.geometry(pageWidth,pageHeight);

            // calculate vertical projection profile proj_on_xaxis1
            for(k = entRect.left() ; k <= entRect.left() + entRect.width() ; k++){
                proj_on_xaxis[k - regionRect.left()] += entRect.height();
            }

            // calculate horizontal projection profile in the same way
            for(k = entRect.top() ; k <= entRect.top() + entRect.height() ; k++){
                proj_on_yaxis[k - regionRect.top()] += entRect.width();
            }

        }


        cout << "width: " << regionRect.width() << " height: " << regionRect.height() << endl;
//        cout << "total Elements: " << j << endl;

//        cout << "projection on y axis " << endl << endl;
        for( j = 0 ; j < size_proj_y ; j++ ){
            if (proj_on_yaxis[j] > maxY) maxY = proj_on_yaxis[j];
//            cout << "index: " << j << " value: " << proj_on_yaxis[j] << endl;
        }

//        cout << "projection on x axis " << endl << endl;
        avgX = count = 0;
        for( j = 0 ; j < size_proj_x ; j++ ){
            if(proj_on_xaxis[j] > maxX) maxX = proj_on_xaxis[j];
            if(proj_on_xaxis[j]){
                count++;
                avgX+= proj_on_xaxis[j];
            }
//            cout << "index: " << j << " value: " << proj_on_xaxis[j] << endl;
        }
        if(count)
            avgX /= count;


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

        //update the regionRect
        int old_left = regionRect.left(), old_top = regionRect.top();

        regionRect.setLeft(old_left + xbegin);
        regionRect.setRight(old_left + xend);

        regionRect.setTop(old_top + ybegin);
        regionRect.setBottom(old_top + yend);


        int tnx = (int)((double)avgX * 10.0 / 100.0 + 0.5), tny = 0;

//        cout << "noise on x_axis: " << avgX << " " << tnx << endl;

//        cout << endl << "projection on x axis ............." << endl << endl;
        for( j = 0 ; j < size_proj_x ; j++ ){
            proj_on_xaxis[j] -= tnx;
//            cout << "index: " << j << " value: " << proj_on_xaxis[j] << endl;
        }

//        cout << endl << "projection on y axis ............ " << endl << endl;
        for(j = 0 ; j < size_proj_y ; j++){
            proj_on_yaxis[j] -= tny;
//            cout << "index: " << j << " value: " << proj_on_yaxis[j] << endl;
        }



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

//        cout << "gap X: " << gap_x << endl;
//        cout << "gap Y: " << gap_y << endl;
//        cout << "cut X: " << cut_pos_x << endl;
//        cout << "cut Y: " << cut_pos_y << endl;


/** 4. Cut the region and make nodes (left,right) or (up,down) ................ **/

        //these can be calculated according to space characteristics
        bool cut_hor = false, cut_ver = false;

        // For horizontal cut
        int topHeight = cut_pos_y - (regionRect.top() - old_top);
        QRect topRect(regionRect.left(),
            regionRect.top(),
            regionRect.width(),
            topHeight);
        QRect bottomRect(regionRect.left(),
            regionRect.top() + topHeight,
            regionRect.width(),
            regionRect.height() - topHeight );

        // For vertical Cut

        //cut position respective to regionRect.left()
        int leftWidth = cut_pos_x - (regionRect.left() - old_left);

        QRect leftRect(regionRect.left(),
            regionRect.top(),
            leftWidth,
            regionRect.height());
        QRect rightRect(regionRect.left() + leftWidth,
            regionRect.top(),
            regionRect.width() - leftWidth,
            regionRect.height());


        if(gap_y >= gap_x && gap_y >= tcy){
            cut_hor = true;
        }
        //vertical cut (left rect, right rect)
        else if(gap_y >= gap_x && gap_y <= tcy && gap_x >= tcx){
            cut_ver = true;
        }
        //vertical cut
        else if(gap_x >= gap_y && gap_x >= tcx){
            cut_ver = true;
        }
        //horizontal cut
        else if(gap_x >= gap_y && gap_x <= tcx && gap_y >= tcy){
            cut_hor = true;
        }
        //no cut possible
        else{

            // we can now update the node rectangle with the shrinked rectangle
            RegionText tmpNode = tree.at(i);
            tmpNode.setArea(regionRect);
            tree.replace(i,tmpNode);

            i++;
            cout << "no cut possible :( :( :(" << endl;

            continue;
        }

        TextList list1,list2;
        TinyTextEntity* ent;
        QRect entRect;

        cout << "previous: ";
        printRect(regionRect);

        // now we need to create two new regionRect
        //horizontal cut, topRect and bottomRect
        if(cut_hor){
            cout << "horizontal cut, list length: " << list.length() << endl;

            printRect(topRect);
            printRect(bottomRect);

            for( j = 0 ; j < list.length() ; j++ ){

                ent = list.at(j);
                entRect = ent->area.geometry(pageWidth,pageHeight);

//                printRect(entRect);

                if(topRect.intersects(entRect)){
                    list1.append(ent);
                }
                else{
                    list2.append(ent);
                }

            }

            RegionText node1(list1,topRect);
            RegionText node2(list2,bottomRect);

            tree.replace(i,node1);
            tree.insert(i+1,node2);

            list1 = tree.at(i).text();
            list2 = tree.at(i+1).text();

        }

        //vertical cut, leftRect and rightRect
        else if(cut_ver){

            cout << "vertical cut, list length: " << list.length() << endl;

            printRect(leftRect);
            printRect(rightRect);

            for( j = 0 ; j < list.length() ; j++ ){

                ent = list.at(j);
                entRect = ent->area.geometry(pageWidth,pageHeight);

                if(leftRect.intersects(entRect))
                    list1.append(ent);
                else list2.append(ent);

            }
            RegionText node1(list1,leftRect);
            RegionText node2(list2,rightRect);

            tree.replace(i,node1);
            tree.insert(i+1,node2);

            list1 = tree.at(i).text();
            list2 = tree.at(i+1).text();
        }

        else { };

        if(cut_hor || cut_ver){

            cout << "list1: " << list1.length() << endl;
            cout << "list2: " << list2.length() << endl;

//            cout << "Node1 text: ........................ " << endl << endl;
            for(j = 0 ; j < list1.length() ; j++){
                 TinyTextEntity *ent = list1.at(j);
//                 cout << ent->text().toAscii().data();
            }
            cout << endl;

//            cout << "Node2 text: ........................ " << endl << endl;
            for(j = 0 ; j < list2.length() ; j++){
                 TinyTextEntity *ent = list2.at(j);
//                 cout << ent->text().toAscii().data();
            }
            cout << endl;

        }

    }

    TextList tmp;

    for(i = 0 ; i < tree.length() ; i++){
        TextList list = tree.at(i).text();

//        cout << "Node: " << i << endl;

        for(j = 0 ; j < list.length() ; j++){
            TinyTextEntity *ent = list.at(j);
            tmp.append(ent);

//            cout << ent->text().toAscii().data();

        }

//        cout << endl << endl;

    }
    //copying elements of tmp to m_words
    copyTo(tmp);

    // we are not removing tmp because, the elements of tmp are in m_XY_cut_tree, we will finally free from m_XY_cut_tree
    m_XY_cut_tree = tree;
}


void TextPagePrivate::addNecessarySpace(){

    /**
     1. We will sort all the texts in the region by Y
     2. After that, we will create a line containing all overlapping Y
     3. Now, we will sort texts in every line by X
     4. We will now add spaces between two words in a line
     5. And, then we will extract all the space separated texts from each region and
        make m_words nice again.
    **/

        RegionTextList tree = m_XY_cut_tree;
        int i,j,k;
        int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();

        // we will only change the texts under RegionTexts, not the area
        for(j = 0 ; j < tree.length() ; j++){
            RegionText tmp = tree.at(j);

            TextList tmpList = tmp.text();
            SortedTextList lines;
            LineRect line_rects;


            makeAndSortLines(tmpList,lines,line_rects);


            // 4. Now, we add space in between texts in a region
            for(i = 0 ; i < lines.length() ; i++){

                TextList list = lines.at(i);

                for( k = 0 ; k < list.length() ; k++ ){

                    QRect area1 = list.at(k)->area.roundedGeometry(pageWidth,pageHeight);
                    if( k+1 >= list.length() ) break;

                    QRect area2 = list.at(k+1)->area.roundedGeometry(pageWidth,pageHeight);
                    int space = area2.left() - area1.right();

                    if(space != 0){

                        // Make a TinyTextEntity of string space and push it between it and it+1
                        int left,right,top,bottom;

                        left = area1.right();
                        right = area2.left();
                        top = area2.top() < area1.top() ? area2.top() : area1.top();
                        bottom = area2.bottom() > area1.bottom() ? area2.bottom() : area1.bottom();

                        QString spaceStr(" ");
                        QRect rect(QPoint(left,top),QPoint(right,bottom));
                        NormalizedRect entRect(rect,pageWidth,pageHeight);
                        TinyTextEntity *ent = new TinyTextEntity(spaceStr,entRect);

                        list.insert(k+1,ent);

                        // we want to skip the space
                         k++;

                    }
                }
                lines.replace(i,list);
            }

            // 5. extract all text and make a TextList
            // now we have all the texts in sorted order in the lines

            while(tmpList.length()) tmpList.pop_back();

            for( i = 0 ; i < lines.length() ; i++){

                TextList list = lines.at(i);
                for( k = 0 ; k < list.length() ; k++){
                    TinyTextEntity *ent = list.at(k);
                    tmpList.append(ent);
                }

            }

            tmp.setText(tmpList);
            tree.replace(j,tmp);
        }


        // Merge all the texts from each region
        TextList tmp;
        for(i = 0 ; i < tree.length() ; i++){
            TextList list = tree.at(i).text();

            for(j = 0 ; j < list.length() ; j++){
                TinyTextEntity *ent = list.at(j);
                //creating new Entities
                tmp.append(new TinyTextEntity(ent->text(),ent->area));
            }
        }

        copyTo(tmp);

}

// Break Words into Characters, takes Entities from m_words and for each of them insert in tmp the character entities
void TextPagePrivate::breakWordIntoCharacters(){

    QString spaceStr(" ");
    TextList tmp;
    int count = 0, i;
    int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();

    for(i = 0 ; i < m_words.length() ; i++){

        TinyTextEntity *ent = m_words.at(i);
        QRect rect = ent->area.geometry(pageWidth,pageHeight);

        // the spaces contains only one character, so we can skip them
        if(ent->text() == spaceStr){
            tmp.append(ent);
        }
        else{

            int key = rect.left() * rect.top()
                    + rect.right() * rect.bottom();

            RegionText word_text = m_word_chars_map.value(key);
            TextList list = word_text.text();

            count = m_word_chars_map.count(key);

            if(count > 1){
                cout << "count : " << count << endl;

                QMap<int, RegionText>::iterator it = m_word_chars_map.find(key);
                while( it != m_word_chars_map.end() && it.key() == key ){

                    word_text = it.value();
                    it++;

                    list = word_text.text();
                    QRect regionRect = word_text.area();

                    if(regionRect.left() == rect.left() && regionRect.top() == rect.top())
                        break;
                }

            }

            tmp.append(list);
        }
    }

    copyTo(tmp);


    // print the final text
//    for( i = 0 ; i < m_words.length() ; i++){
//        TinyTextEntity* ent = m_words.at(i);
//        cout << ent->text().toAscii().data();
//    }

}


void TextPagePrivate::calculateStatisticalInformation(SortedTextList &lines, LineRect line_rects,int &word_spacing,
                                                      int &line_spacing,int &col_spacing){

    /**

    For the region, defined by line_rects and lines

    1. Make line statistical analysis to find the line spacing
    2. Make character statistical analysis to differentiate between
        word spacing and column spacing.

    **/

    /** Step 1: ........................................................................ **/
    QMap<int,int> line_space_stat;
    for(int i = 0 ; i < line_rects.length(); i++){
        QRect rectUpper = line_rects.at(i);

        if(i+1 == line_rects.length()) break;
        QRect rectLower = line_rects.at(i+1);

        int linespace = rectLower.top() - (rectUpper.top() + rectUpper.height());
        if(linespace < 0) linespace =-linespace;

        if(line_space_stat.contains(linespace))
            line_space_stat[linespace]++;
        else line_space_stat[linespace] = 1;
    }

    line_spacing = 0;
    int weighted_count = 0;
    QMapIterator<int, int> iterate_linespace(line_space_stat);

    while(iterate_linespace.hasNext()){
        iterate_linespace.next();
//        cout << iterate_linespace.key() << ":" << iterate_linespace.value() << endl;
        line_spacing += iterate_linespace.value() * iterate_linespace.key();
        weighted_count += iterate_linespace.value();
    }

    if(line_spacing)
        line_spacing = (int) ( (double)line_spacing / (double) weighted_count + 0.5);
//    cout << "average line spacing: " << line_spacing << endl;


    /** Step 2: ........................................................................ **/

    //we would like to use QMap instead of QHash as it will keep the keys sorted
    QMap<int,int> hor_space_stat;   //this is to find word spacing
    QMap<int,int> col_space_stat;   //this is to find column spacing

    QList< QList<QRect> > space_rects; // to save all the word spacing or column spacing rects
    QList<QRect> max_hor_space_rects;

    int i;
    int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();

    // space in every line
    for(i = 0 ; i < lines.length() ; i++){
        // list contains a line
        TextList list = lines.at(i);
        QList<QRect> line_space_rects;

        int maxSpace = 0, minSpace = pageWidth;

        // for every TinyTextEntity element in the line
        TextList::Iterator it = list.begin(), itEnd = list.end();
        QRect max_area1,max_area2;
        QString before_max, after_max;


        // for every line
        for( ; it != itEnd ; it++ ){

            QRect area1 = (*it)->area.roundedGeometry(pageWidth,pageHeight);
            if( it+1 == itEnd ) break;

            QRect area2 = (*(it+1))->area.roundedGeometry(pageWidth,pageHeight);
            int space = area2.left() - area1.right();

            if(space < 0){
//                cout << "space: " << space << endl;
//                cout << "text: " << (*it)->text().toAscii().data() << " "
//                << (*(it+1))->text().toAscii().data() << endl;
            }

            if(space > maxSpace){
                max_area1 = area1;
                max_area2 = area2;

                maxSpace = space;

                before_max = (*it)->text();
                after_max = (*(it+1))->text();
            }

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

            }
        }


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

    word_spacing = 0;
    weighted_count = 0;
    QMapIterator<int, int> iterate(hor_space_stat);

    while (iterate.hasNext()) {
        iterate.next();
//        cout << iterate.key() << ": " << iterate.value() << endl;

        if(iterate.key() > 0){
            word_spacing += iterate.value() * iterate.key();
            weighted_count += iterate.value();
        }
    }
    if(weighted_count)
        word_spacing = (int) ((double)word_spacing / (double)weighted_count + 0.5);
//    cout << "Word Spacing: " << word_spacing << endl;


    col_spacing = 0;
    QMapIterator<int, int> iterate_col(col_space_stat);

    while (iterate_col.hasNext()) {
        iterate_col.next();
//        cout << iterate_col.key() << ": " << iterate_col.value() << endl;
        if(iterate_col.value() > col_spacing) col_spacing = iterate_col.value();
    }
    col_spacing = col_space_stat.key(col_spacing);
//    cout << "Column Spacing: " << col_spacing << endl;

}



//correct the textOrder, all layout recognition works here
void TextPage::correctTextOrder(){

    // remove spaces from the text
    d->removeSpace();

    // make words from characters
    d->makeWordFromCharacters();

    // create arbitrary lines from words and sort them according to X and Y position
    d->makeAndSortLines(d->m_words,d->m_lines,d->m_line_rects);

    // calculate statistical information
    int word_spacing = 0,line_spacing = 0,col_spacing = 0;
    d->calculateStatisticalInformation(d->m_lines,d->m_line_rects,word_spacing,line_spacing, col_spacing);

    // Make a XY Cut tree for segmentation
    d->XYCutForBoundingBoxes(word_spacing * 2,line_spacing * 2);

    // add spaces to the word
    d->addNecessarySpace();

    // break the words into characters
    d->breakWordIntoCharacters();

}
