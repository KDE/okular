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
#include <QVarLengthArray>

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

static int qHash(const QRect &r)
{
    return r.left() * r.top() + r.right() * r.bottom();
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

/**
 * If the horizontal arm of one rectangle fully contains the other (example below)
 *  --------         ----         -----  first
 *    ----         --------       -----  second
 * or we can make it overlap of spaces by threshold%
*/
static bool doesConsumeX(const QRect& first, const QRect& second, int threshold)
{
    // if one consumes another fully
    if(first.left() <= second.left() && first.right() >= second.right())
        return true;

    if(first.left() >= second.left() && first.right() <= second.right())
        return true;

    // or if there is overlap of space by more than threshold%
    // there is overlap
    if(second.right() >= first.left() && first.right() >= second.left())
    {
        const int overlap = (second.right() >= first.right()) ? first.right() - second.left()
                                                              : second.right() - first.left();
        // we will divide by the smaller rectangle to calculate the overlap
        const int percentage = (first.width() < second.width()) ? overlap * 100 / (first.right() - first.left())
                                                                : overlap * 100 / (second.right() - second.left());
        if(percentage >= threshold) return true;
    }

    return false;
}

/**
 * Same concept of doesConsumeX but in this case we calculate on y axis
 */
static bool doesConsumeY(const QRect& first, const QRect& second, int threshold)
{
    // if one consumes another fully
    if(first.top() <= second.top() && first.bottom() >= second.bottom())
        return true;

    if(first.top() >= second.top() && first.bottom() <= second.bottom())
        return true;

    // or if there is overlap of space by more than 80%
    // there is overlap
    if(second.bottom() >= first.top() && first.bottom() >= second.top())
    {
        const int overlap = (second.bottom() >= first.bottom()) ? first.bottom() - second.top()
                                                                : second.bottom() - first.top();
        //we will divide by the smaller rectangle to calculate the overlap
        const int percentage = (first.width() < second.width()) ? overlap * 100 / (first.bottom() - first.top())
                                                                : overlap * 100 / (second.bottom() - second.top());

        if(percentage >= threshold) return true;
    }

    return false;
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

/**
 * We will divide the whole page in some regions depending on the horizontal and
 * vertical spacing among different regions. Each region will have an area and an
 * associated TextList in sorted order.
*/
class RegionText
{

public:
    RegionText()
    {
    };

    RegionText(const TextList &list,const QRect &area)
        : m_region_text(list) ,m_area(area)
    {
    }
    
    inline QString string() const
    {
        QString res;
        foreach(TinyTextEntity *te, m_region_text)
            res += te->text();
        return res;
    }

    inline TextList text() const
    {
        return m_region_text;
    }

    inline QRect area() const
    {
        return m_area;
    }

    inline void setArea(const QRect &area)
    {
        m_area = area;
    }

    inline void setText(const TextList &text)
    {
        m_region_text = text;
    }

private:
    TextList m_region_text;
    QRect m_area;
};

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
    const double scaleX = d->m_page->m_page->width();
    const double scaleY = d->m_page->m_page->height();

    NormalizedPoint startC = sel->start();
    NormalizedPoint endC = sel->end();
    NormalizedPoint temp;

    // if startPoint is right to endPoint swap them
    if(startC.x > endC.x)
    {
        temp = startC;
        startC = endC;
        endC = temp;
    }

    // minX,maxX,minY,maxY gives the bounding rectangle coordinates of the document
    const NormalizedRect boundingRect = d->m_page->m_page->boundingBox();
    const QRect content = boundingRect.geometry(scaleX,scaleY);
    const double minX = content.left();
    const double maxX = content.right();
    const double minY = content.top();
    const double maxY = content.bottom();

    /**
     * We will now find out the TinyTextEntity for the startRectangle and TinyTextEntity for
     * the endRectangle. We have four cases:
     *
     * Case 1(a): both startpoint and endpoint are out of the bounding Rectangle and at one side, so the rectangle made of start
     * and endPoint are outof the bounding rect (do not intersect)
     *
     * Case 1(b): both startpoint and endpoint are out of bounding rect, but they are in different side, so is their rectangle
     *
     * Case 2(a): find the rectangle which contains start and endpoint and having some
     * TextEntity
     *
     * Case 2(b): if 2(a) fails (if startPoint and endPoint both are unchanged), then we check whether there is any
     * TextEntity within the rect made by startPoint and endPoint
     *
     * Case 3: Now, we may have two type of selection.
     * 1. startpoint is left-top of start_end and endpoint is right-bottom
     * 2. startpoint is left-bottom of start_end and endpoint is top-right
     *
     * Also, as 2(b) is passed, we might have it,itEnd or both unchanged, but the fact is that we have
     * text within them. so, we need to search for the best suitable textposition for start and end.
     *
     * Case 3(a): We search the nearest rectangle consisting of some
     * TinyTextEntity right to or bottom of the startPoint for selection 01.
     * And, for selection 02, we have to search for right and top
     *
     * Case 3(b): For endpoint, we have to find the point top of or left to
     * endpoint if we have selection 01.
     * Otherwise, the search will be left and bottom
     */

    // we know that startC.x > endC.x, we need to decide which is top and which is bottom
    const NormalizedRect start_end = (startC.y < endC.y) ? NormalizedRect(startC.x, startC.y, endC.x, endC.y)
                                                         : NormalizedRect(startC.x, endC.y, endC.x, startC.y);

    // Case 1(a)
    if(!boundingRect.intersects(start_end)) return ret;

    // case 1(b)
    /**
        note that, after swapping of start and end, we know that,
        start is always left to end. but, we cannot say start is
        positioned upper than end.
    **/
    else
    {
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
    TextList::ConstIterator start = it, end = itEnd, tmpIt = it; //, tmpItEnd = itEnd;
    const MergeSide side = d->m_page ? (MergeSide)d->m_page->m_page->totalOrientation() : MergeRight;

    NormalizedRect tmp;
    //case 2(a)
    for ( ; it != itEnd; ++it )
    {
        tmp = (*it)->area;
        if(tmp.contains(startC.x,startC.y)){
            start = it;
        }
        if(tmp.contains(endC.x,endC.y)){
            end = it;
        }
    }

    //case 2(b)
    it = tmpIt;
    if(start == it && end == itEnd)
    {
        for ( ; it != itEnd; ++it )
        {
            // is there any text reactangle within the start_end rect
            tmp = (*it)->area;
            if(start_end.intersects(tmp))
                break;
        }

        // we have searched every text entities, but none is within the rectangle created by start and end
        // so, no selection should be done
        if(it == itEnd)
        {
            return ret;
        }
    }
    it = tmpIt;
    bool selection_two_start = false;

    //case 3.a
    if(start == it)
    {
        bool flagV = false;
        NormalizedRect rect;

        // selection type 01
        if(startC.y <= endC.y)
        {
            for ( ; it != itEnd; ++it )
            {
                rect= (*it)->area;
                rect.isBottom(startC) ? flagV = false: flagV = true;

                if(flagV && rect.isRight(startC))
                {
                    start = it;
                    break;
                }
            }
        }

        //selection type 02
        else
        {
            selection_two_start = true;
            int distance = scaleX + scaleY + 100;
            int count = 0;

            for ( ; it != itEnd; ++it )
            {
                rect= (*it)->area;

                if(rect.isBottomOrLevel(startC) && rect.isRight(startC))
                {
                    count++;
                    QRect entRect = rect.geometry(scaleX,scaleY);
                    int xdist, ydist;
                    xdist = entRect.center().x() - startC.x * scaleX;
                    ydist = entRect.center().y() - startC.y * scaleY;

                    //make them positive
                    if(xdist < 0) xdist = -xdist;
                    if(ydist < 0) ydist = -ydist;

                    if( (xdist + ydist) < distance)
                    {
                        distance = xdist+ ydist;
                        start = it;
                    }
                }
            }
        }
    }

    //case 3.b
    if(end == itEnd)
    {
        it = tmpIt;
        itEnd = itEnd-1;

        bool flagV = false;
        NormalizedRect rect;

        if(startC.y <= endC.y)
        {
            for ( ; itEnd >= it; itEnd-- )
            {
                rect= (*itEnd)->area;
                rect.isTop(endC) ? flagV = false: flagV = true;

                if(flagV && rect.isLeft(endC))
                {
                    end = itEnd;
                    break;
                }
            }
        }

        else
        {
            int distance = scaleX + scaleY + 100;
            for ( ; itEnd >= it; itEnd-- )
            {
                rect= (*itEnd)->area;

                if(rect.isTopOrLevel(endC) && rect.isLeft(endC))
                {
                    QRect entRect = rect.geometry(scaleX,scaleY);
                    int xdist, ydist;
                    xdist = entRect.center().x() - endC.x * scaleX;
                    ydist = entRect.center().y() - endC.y * scaleY;

                    //make them positive
                    if(xdist < 0) xdist = -xdist;
                    if(ydist < 0) ydist = -ydist;

                    if( (xdist + ydist) < distance)
                    {
                        distance = xdist+ ydist;
                        end = itEnd;
                    }

                }
            }
        }
    }

    /* if start and end in selection 02 are in the same column, and we
     start at an empty space we have to remove the selection of last
     character
    */
    if(selection_two_start)
    {
        if(start > end)
        {
            start = start - 1;
        }
    }

    // if start is less than end swap them
    if(start > end)
    {
        it = start;
        start = end;
        end = it;
    }

    // removes the possibility of crash, in case none of 1 to 3 is true
    if(end == d->m_words.constEnd()) end--;

    for( ;start <= end ; start++)
    {
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

// hyphenated '-' must be at the end of a word, so hyphenation means
// we have a '-' just followed by a '\n' character
// check if the string contains a '-' character
// if the '-' is the last entry
static int stringLengthAdaptedWithHyphen(const QString &str, const TextList::ConstIterator &it, const TextList::ConstIterator &end, PagePrivate *page)
{
    int len = str.length();
    
    // hyphenated '-' must be at the end of a word, so hyphenation means
    // we have a '-' just followed by a '\n' character
    // check if the string contains a '-' character
    // if the '-' is the last entry
    if ( str.endsWith( '-' ) )
    {
        // validity chek of it + 1
        if ( ( it + 1 ) != end )
        {
            // 1. if the next character is '\n'
            const QString &lookahedStr = (*(it+1))->text();
            if (lookahedStr.startsWith('\n'))
            {
                len -= 1;
            }
            else
            {
                // 2. if the next word is in a different line or not
                const int pageWidth = page->m_page->width();
                const int pageHeight = page->m_page->height();

                const QRect hyphenArea = (*it)->area.roundedGeometry(pageWidth, pageHeight);
                const QRect lookaheadArea = (*(it + 1))->area.roundedGeometry(pageWidth, pageHeight);

                // lookahead to check whether both the '-' rect and next character rect overlap
                if( !doesConsumeY( hyphenArea, lookaheadArea, 70 ) )
                {
                    len -= 1;
                }
            }
        }
    }
    // else if it is the second last entry - for example in pdf format
    else if (str.endsWith("-\n"))
    {
        len -= 2;
    }
    
    return len;
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
            len = stringLengthAdaptedWithHyphen(str, it, end, m_page);
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
            len = stringLengthAdaptedWithHyphen(str, it, end, m_page);
            int min=qMin(queryLeft,len);
#ifdef DEBUG_TEXTPAGE
            kDebug(OkularDebug) << str.right(min) << " : " << _query.mid(j-min+1,min);
#endif
            // we have equal (or less than) area of the query left as the length of the current 
            // entity

            int resStrLen = 0, resQueryLen = 0;
            // Note len is not str.length() so we can't use rightRef here
            const int offset = len - min;
            if ( !comparer( str.midRef(offset, min ), query.midRef( j - min + 1, min ),
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

static bool compareTinyTextEntityX(TinyTextEntity* first, TinyTextEntity* second)
{
    QRect firstArea = first->area.roundedGeometry(1000,1000);
    QRect secondArea = second->area.roundedGeometry(1000,1000);

    return firstArea.left() < secondArea.left();
}

static bool compareTinyTextEntityY(TinyTextEntity* first, TinyTextEntity* second)
{
    QRect firstArea = first->area.roundedGeometry(1000,1000);
    QRect secondArea = second->area.roundedGeometry(1000,1000);

    return firstArea.top() < secondArea.top();
}

/**
 * Copies a TextList to m_words with the same pointer
 */
void TextPagePrivate::setWordList(const TextList &list)
{
    qDeleteAll(m_words);
    m_words = list;
}

/**
 * Copies from m_words to list with distinct pointers
 */
TextList TextPagePrivate::duplicateWordList() const
{
    TextList list;
    for(int i = 0 ; i < m_words.length() ; i++)
    {
        TinyTextEntity* ent = m_words.at(i);
        list.append( new TinyTextEntity( ent->text(),ent->area ) );
    }
    return list;
}

/**
 * Remove all the spaces in between texts. It will make all the generators
 * same, whether they save spaces(like pdf) or not(like djvu).
 */
void TextPagePrivate::removeSpace()
{
    TextList::Iterator it = m_words.begin(), itEnd = m_words.end();
    const QString str(' ');

    it = m_words.begin(), itEnd = m_words.end();
    while ( it != itEnd )
    {
        if((*it)->text() == str)
        {
            delete *it;
            it = m_words.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

/**
 * We will the TinyTextEntity from m_words and try to create
 * words from there.
 */
QHash<QRect, RegionText> TextPagePrivate::makeWordFromCharacters()
{
    /**
     * At first we will copy m_words to tmpList. Then, we will traverse the
     * tmpList and try to create words from the TinyTextEntities in tmpList.
     * We will search TinyTextEntity blocks and merge them until we get a
     * space between two consecutive TinyTextEntities. When we get a space
     * we can take it as a end of word. Then we store the word as a TinyTextEntity
     * and keep it in newList.

     * We also keep a mapping between every element in newList and word. We create a
     * RegionText named regionWord and create a hash key from the TinyTextEntity
     * rectangle area of the element in newList. So, we can get the TinyTextEntities from
     * which every element(word) of newList is generated. It will be necessary later
     * when we will divide the word into characters.

     * Finally we copy the newList to m_words.
     */

    QHash<QRect, RegionText> word_chars_map;
    const TextList tmpList = m_words;
    TextList newList;

    TextList::ConstIterator it = tmpList.begin(), itEnd = tmpList.end(), tmpIt;
    int newLeft,newRight,newTop,newBottom;
    const int pageWidth = m_page->m_page->width();
    const int pageHeight = m_page->m_page->height();
    int index = 0;

    for( ; it != itEnd ; it++)
    {
        QString textString = (*it)->text();
        QString newString;
        QRect lineArea = (*it)->area.roundedGeometry(pageWidth,pageHeight),elementArea;
        TextList word;
        tmpIt = it;
        int space = 0;

        while(!space )
        {
            if(textString.length())
            {
                newString.append(textString);

                // when textString is the start of the word
                if(tmpIt == it)
                {
                    NormalizedRect newRect(lineArea,pageWidth,pageHeight);
                    word.append(new TinyTextEntity(textString.normalized
                                                   (QString::NormalizationForm_KC), newRect));
                }
                else
                {
                    NormalizedRect newRect(elementArea,pageWidth,pageHeight);
                    word.append(new TinyTextEntity(textString.normalized
                                                   (QString::NormalizationForm_KC), newRect));
                }
            }

            it++;

            /*
             we must have to put this line before the if condition of it==itEnd
             otherwise the last character can be missed
             */
            if(it == itEnd) break;
            elementArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);
            if(!doesConsumeY(elementArea,lineArea,60))
            {
                it--;
                break;
            }

            const int text_y1 = elementArea.top() ,
                      text_x1 = elementArea.left(),
                      text_y2 = elementArea.y() + elementArea.height(),
                      text_x2 = elementArea.x() + elementArea.width();
            const int line_y1 = lineArea.top() ,line_x1 = lineArea.left(),
                      line_y2 = lineArea.y() + lineArea.height(),
                      line_x2 = lineArea.x() + lineArea.width();

            space = elementArea.left() - lineArea.right();

            if(space > 0 || space < 0)
            {
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

            textString = (*it)->text();
        }

        // if newString is not empty, save it
        if(newString.length())
        {
            const NormalizedRect newRect(lineArea,pageWidth,pageHeight);
            newList.append(new TinyTextEntity(newString.normalized
                                              (QString::NormalizationForm_KC), newRect ));
            const QRect rect = newRect.geometry(pageWidth,pageHeight);
            const RegionText regionWord(word,rect);

            // there may be more than one element in the same key
            word_chars_map.insertMulti(rect,regionWord);

            index++;
        }

        if(it == itEnd) break;
    }

    setWordList(newList);

    return word_chars_map;
}

/**
 * Create Lines from the words and sort them
 */
void TextPagePrivate::makeAndSortLines(const TextList &wordsTmp, SortedTextList *lines, LineRect *line_rects)
{
    /**
     * We cannot assume that the generator will give us texts in the right order.
     * We can only assume that we will get texts in the page and their bounding
     * rectangle. The texts can be character, word, half-word anything.
     * So, we need to:
     **
     * 1. Sort rectangles/boxes containing texts by y0(top)
     * 2. Create textline where there is y overlap between TinyTextEntity 's
     * 3. Within each line sort the TinyTextEntity 's by x0(left)
     */

    /*
     Make a new copy of the TextList in the words, so that the wordsTmp and lines do
     not contain same pointers for all the TinyTextEntity.
     */
    TextList words;
    for(int i = 0 ; i < wordsTmp.length() ; i++)
    {
        TinyTextEntity* ent = wordsTmp.at(i);
        words.append( new TinyTextEntity( ent->text(),ent->area ) );
    }

    // Step 1
    qSort(words.begin(),words.end(),compareTinyTextEntityY);

    // Step 2
    TextList::Iterator it = words.begin(), itEnd = words.end();
    const int pageWidth = m_page->m_page->width();
    const int pageHeight = m_page->m_page->height();

    //for every non-space texts(characters/words) in the textList
    for( ; it != itEnd ; it++)
    {
        const QRect elementArea = (*it)->area.roundedGeometry(pageWidth,pageHeight);
        bool found = false;

        for( int i = 0 ; i < lines->length() ; i++)
        {
            /* the line area which will be expanded
               line_rects is only necessary to preserve the topmin and bottommax of all
               the texts in the line, left and right is not necessary at all
            */
            QRect &lineArea = (*line_rects)[i];
            const int text_y1 = elementArea.top() ,
                      text_y2 = elementArea.top() + elementArea.height() ,
                      text_x1 = elementArea.left(),
                      text_x2 = elementArea.left() + elementArea.width();
            const int line_y1 = lineArea.top() ,
                      line_y2 = lineArea.top() + lineArea.height(),
                      line_x1 = lineArea.left(),
                      line_x2 = lineArea.left() + lineArea.width();

            /*
               if the new text and the line has y overlapping parts of more than 70%,
               the text will be added to this line
             */
            if(doesConsumeY(elementArea,lineArea,70))
            {
                TextList &line = (*lines)[i];
                line.append(*it);

                const int newLeft = line_x1 < text_x1 ? line_x1 : text_x1;
                const int newRight = line_x2 > text_x2 ? line_x2 : text_x2;
                const int newTop = line_y1 < text_y1 ? line_y1 : text_y1;
                const int newBottom = text_y2 > line_y2 ? text_y2 : line_y2;

                lineArea = QRect( newLeft,newTop, newRight - newLeft, newBottom - newTop );
                found = true;
            }

            if(found) break;
        }

        /* when we have found a new line create a new TextList containing
           only one element and append it to the lines
         */
        if(!found)
        {
            TextList tmp;
            tmp.append((*it));
            lines->append(tmp);
            line_rects->append(elementArea);
        }
    }

    // Step 3
    for(int i = 0 ; i < lines->length() ; i++)
    {
        TextList &list = (*lines)[i];
        qSort(list.begin(),list.end(),compareTinyTextEntityX);
    }
}

/**
 * Calculate Statistical information from the lines we made previously
 */
void TextPagePrivate::calculateStatisticalInformation(const SortedTextList &lines, const LineRect &line_rects, int *word_spacing, int *line_spacing, int *col_spacing)
{
    /**
     * For the region, defined by line_rects and lines
     * 1. Make line statistical analysis to find the line spacing
     * 2. Make character statistical analysis to differentiate between
     *   word spacing and column spacing.
     */

    /**
     * Step 1
     */
    QMap<int,int> line_space_stat;
    for(int i = 0 ; i < line_rects.length(); i++)
    {
        const QRect rectUpper = line_rects.at(i);

        if(i+1 == line_rects.length()) break;
        const QRect rectLower = line_rects.at(i+1);

        int linespace = rectLower.top() - (rectUpper.top() + rectUpper.height());
        if(linespace < 0) linespace =-linespace;

        if(line_space_stat.contains(linespace))
            line_space_stat[linespace]++;
        else line_space_stat[linespace] = 1;
    }

    *line_spacing = 0;
    int weighted_count = 0;
    QMapIterator<int, int> iterate_linespace(line_space_stat);

    while(iterate_linespace.hasNext())
    {
        iterate_linespace.next();
        *line_spacing += iterate_linespace.value() * iterate_linespace.key();
        weighted_count += iterate_linespace.value();
    }
    if (*line_spacing != 0)
        *line_spacing = (int) ( (double)*line_spacing / (double) weighted_count + 0.5);

    /**
     * Step 2
     */
    // We would like to use QMap instead of QHash as it will keep the keys sorted
    QMap<int,int> hor_space_stat;
    QMap<int,int> col_space_stat;
    QList< QList<QRect> > space_rects;
    QList<QRect> max_hor_space_rects;

    int pageWidth = m_page->m_page->width(), pageHeight = m_page->m_page->height();

    // Space in every line
    for(int i = 0 ; i < lines.length() ; i++)
    {
        TextList list = lines.at(i);
        QList<QRect> line_space_rects;
        int maxSpace = 0, minSpace = pageWidth;

        // for every TinyTextEntity element in the line
        TextList::Iterator it = list.begin(), itEnd = list.end();
        QRect max_area1,max_area2;
        QString before_max, after_max;

        // for every line
        for( ; it != itEnd ; it++ )
        {
            const QRect area1 = (*it)->area.roundedGeometry(pageWidth,pageHeight);
            if( it+1 == itEnd ) break;

            const QRect area2 = (*(it+1))->area.roundedGeometry(pageWidth,pageHeight);
            int space = area2.left() - area1.right();

            if(space > maxSpace)
            {
                max_area1 = area1;
                max_area2 = area2;
                maxSpace = space;
                before_max = (*it)->text();
                after_max = (*(it+1))->text();
            }

            if(space < minSpace && space != 0) minSpace = space;

            //if we found a real space, whose length is not zero and also less than the pageWidth
            if(space != 0 && space != pageWidth)
            {
                // increase the count of the space amount
                if(hor_space_stat.contains(space)) hor_space_stat[space] = hor_space_stat[space]++;
                else hor_space_stat[space] = 1;

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

        if(hor_space_stat.contains(maxSpace))
        {
            if(hor_space_stat[maxSpace] != 1)
                hor_space_stat[maxSpace] = hor_space_stat[maxSpace]--;
            else hor_space_stat.remove(maxSpace);
        }

        if(maxSpace != 0)
        {
            if (col_space_stat.contains(maxSpace))
                col_space_stat[maxSpace] = col_space_stat[maxSpace]++;
            else col_space_stat[maxSpace] = 1;

            //store the max rect of each line
            const int left = max_area1.right();
                const int right = max_area2.left();
            const int top = (max_area1.top() > max_area2.top()) ? max_area2.top() :
                                                                  max_area1.top();
            const int bottom = (max_area1.bottom() < max_area2.bottom()) ? max_area2.bottom() :
                                                                           max_area1.bottom();

            const QRect rect(left,top,right-left,bottom-top);
            max_hor_space_rects.append(rect);
        }
        else max_hor_space_rects.append(QRect(0,0,0,0));
    }

    // All the between word space counts are in hor_space_stat
    *word_spacing = 0;
    weighted_count = 0;
    QMapIterator<int, int> iterate(hor_space_stat);

    while (iterate.hasNext())
    {
        iterate.next();

        if(iterate.key() > 0)
        {
            *word_spacing += iterate.value() * iterate.key();
            weighted_count += iterate.value();
        }
    }
    if(weighted_count)
        *word_spacing = (int) ((double)*word_spacing / (double)weighted_count + 0.5);

    *col_spacing = 0;
    QMapIterator<int, int> iterate_col(col_space_stat);

    while (iterate_col.hasNext())
    {
        iterate_col.next();
        if(iterate_col.value() > *col_spacing) *col_spacing = iterate_col.value();
    }
    *col_spacing = col_space_stat.key(*col_spacing);

    // if there is just one line in a region, there is no point in dividing it
    if(lines.length() == 1)
        *word_spacing = *col_spacing;
}

/**
 * Implements the XY Cut algorithm for textpage segmentation
 */
RegionTextList TextPagePrivate::XYCutForBoundingBoxes(int tcx, int tcy)
{
    const int pageWidth = m_page->m_page->width();
    const int pageHeight = m_page->m_page->height();
    RegionTextList tree;
    QRect contentRect(m_page->m_page->boundingBox().geometry(pageWidth,pageHeight));
    const TextList words = duplicateWordList();
    const RegionText root(words,contentRect);

    // start the tree with the root, it is our only region at the start
    tree.push_back(root);

    int i = 0, j, k;
    int countLoop = 0;

    // while traversing the tree has not been ended
    while(i < tree.length())
    {
        const RegionText node = tree.at(i);
        QRect regionRect = node.area();

        /**
         * 1. calculation of projection profiles
         */
        // allocate the size of proj profiles and initialize with 0
        int size_proj_y = node.area().height() ;
        int size_proj_x = node.area().width() ;
        //dynamic memory allocation
        QVarLengthArray<int> proj_on_xaxis(size_proj_x);
        QVarLengthArray<int> proj_on_yaxis(size_proj_y);

        for( j = 0 ; j < size_proj_y ; j++ ) proj_on_yaxis[j] = 0;
        for( j = 0 ; j < size_proj_x ; j++ ) proj_on_xaxis[j] = 0;

        TextList list = node.text();

        // Calculate tcx and tcy locally for each new region
        if(countLoop++)
        {
            SortedTextList lines;
            LineRect line_rects;
            int word_spacing, line_spacing, column_spacing;

            makeAndSortLines(list, &lines, &line_rects);
            calculateStatisticalInformation(lines, line_rects, &word_spacing, &line_spacing, &column_spacing);
            for(int i = 0 ; i < lines.length() ; i++)
            {
                qDeleteAll(lines.at(i));
            }   
            lines.clear();

            tcx = word_spacing * 2;
            tcy = line_spacing * 2;
        }

        int maxX = 0 , maxY = 0;
        int avgX = 0 ;
        int count;

        // for every text in the region
        for( j = 0 ; j < list.length() ; j++ )
        {
            TinyTextEntity *ent = list.at(j);
            QRect entRect = ent->area.geometry(pageWidth,pageHeight);

            // calculate vertical projection profile proj_on_xaxis1
            for(k = entRect.left() ; k <= entRect.left() + entRect.width() ; k++)
            {
                if( ( k-regionRect.left() ) < size_proj_x && ( k-regionRect.left() ) >= 0 )
                    proj_on_xaxis[k - regionRect.left()] += entRect.height();
            }

            // calculate horizontal projection profile in the same way
            for(k = entRect.top() ; k <= entRect.top() + entRect.height() ; k++)
            {
                if( ( k-regionRect.top() ) < size_proj_y && ( k-regionRect.top() ) >= 0 )
                    proj_on_yaxis[k - regionRect.top()] += entRect.width();
            }
        }

        for( j = 0 ; j < size_proj_y ; j++ )
        {
            if (proj_on_yaxis[j] > maxY)
                maxY = proj_on_yaxis[j];
        }

        avgX = count = 0;
        for( j = 0 ; j < size_proj_x ; j++ )
        {
            if(proj_on_xaxis[j] > maxX) maxX = proj_on_xaxis[j];
            if(proj_on_xaxis[j])
            {
                count++;
                avgX+= proj_on_xaxis[j];
            }
        }
        if(count) avgX /= count;


        /**
         * 2. Cleanup Boundary White Spaces and removal of noise
         */
        int xbegin = 0, xend = size_proj_x - 1;
        int ybegin = 0, yend = size_proj_y - 1;
        while(xbegin < size_proj_x && proj_on_xaxis[xbegin] <= 0)
            xbegin++;
        while(xend >= 0 && proj_on_xaxis[xend] <= 0)
            xend--;
        while(ybegin < size_proj_y && proj_on_yaxis[ybegin] <= 0)
            ybegin++;
        while(yend >= 0 && proj_on_yaxis[yend] <= 0)
            yend--;

        //update the regionRect
        int old_left = regionRect.left(), old_top = regionRect.top();
        regionRect.setLeft(old_left + xbegin);
        regionRect.setRight(old_left + xend);
        regionRect.setTop(old_top + ybegin);
        regionRect.setBottom(old_top + yend);

        int tnx = (int)((double)avgX * 10.0 / 100.0 + 0.5), tny = 0;
        for( j = 0 ; j < size_proj_x ; j++ )
            proj_on_xaxis[j] -= tnx;
        for(j = 0 ; j < size_proj_y ; j++)
            proj_on_yaxis[j] -= tny;

        /**
         * 3. Find the Widest gap
         */
        int gap_hor = -1, pos_hor = -1;
        int begin = -1, end = -1;

        // find all hor_gaps and find the maximum between them
        for(j = 1 ; j < size_proj_y ; j++)
        {
            //transition from white to black
            if(begin >= 0 && proj_on_yaxis[j-1] <= 0
                    && proj_on_yaxis[j] > 0)
                end = j;

            //transition from black to white
            if(proj_on_yaxis[j-1] > 0 && proj_on_yaxis[j] <= 0)
                begin = j;

            if(begin > 0 && end > 0 && end-begin > gap_hor)
            {
                gap_hor = end - begin;
                pos_hor = (end + begin) / 2;
                begin = -1;
                end = -1;
            }
        }


        begin = -1, end = -1;
        int gap_ver = -1, pos_ver = -1;

        //find all the ver_gaps and find the maximum between them
        for(j = 1 ; j < size_proj_x ; j++)
        {
            //transition from white to black
            if(begin >= 0 && proj_on_xaxis[j-1] <= 0
                    && proj_on_xaxis[j] > 0){
                end = j;
            }

            //transition from black to white
            if(proj_on_xaxis[j-1] > 0 && proj_on_xaxis[j] <= 0)
                begin = j;

            if(begin > 0 && end > 0 && end-begin > gap_ver)
            {
                gap_ver = end - begin;
                pos_ver = (end + begin) / 2;
                begin = -1;
                end = -1;
            }
        }

        int cut_pos_x = pos_ver, cut_pos_y = pos_hor;
        int gap_x = gap_ver, gap_y = gap_hor;

        /**
         * 4. Cut the region and make nodes (left,right) or (up,down)
         */
        bool cut_hor = false, cut_ver = false;

        // For horizontal cut
        const int topHeight = cut_pos_y - (regionRect.top() - old_top);
        const QRect topRect(regionRect.left(),
                            regionRect.top(),
                            regionRect.width(),
                            topHeight);
        const QRect bottomRect(regionRect.left(),
                               regionRect.top() + topHeight,
                               regionRect.width(),
                               regionRect.height() - topHeight );

        // For vertical Cut
        const int leftWidth = cut_pos_x - (regionRect.left() - old_left);
        const QRect leftRect(regionRect.left(),
                             regionRect.top(),
                             leftWidth,
                             regionRect.height());
        const QRect rightRect(regionRect.left() + leftWidth,
                              regionRect.top(),
                              regionRect.width() - leftWidth,
                              regionRect.height());

        if(gap_y >= gap_x && gap_y >= tcy)
            cut_hor = true;
        else if(gap_y >= gap_x && gap_y <= tcy && gap_x >= tcx)
            cut_ver = true;
        else if(gap_x >= gap_y && gap_x >= tcx)
            cut_ver = true;
        else if(gap_x >= gap_y && gap_x <= tcx && gap_y >= tcy)
            cut_hor = true;
        // no cut possible
        else
        {
            // we can now update the node rectangle with the shrinked rectangle
            RegionText tmpNode = tree.at(i);
            tmpNode.setArea(regionRect);
            tree.replace(i,tmpNode);
            i++;
            continue;
        }

        TextList list1,list2;
        TinyTextEntity* ent;
        QRect entRect;

        // horizontal cut, topRect and bottomRect
        if(cut_hor)
        {
            for( j = 0 ; j < list.length() ; j++ )
            {
                ent = list.at(j);
                entRect = ent->area.geometry(pageWidth,pageHeight);

                if(topRect.intersects(entRect))
                    list1.append(ent);
                else
                    list2.append(ent);
            }

            RegionText node1(list1,topRect);
            RegionText node2(list2,bottomRect);

            tree.replace(i,node1);
            tree.insert(i+1,node2);
        }

        //vertical cut, leftRect and rightRect
        else if(cut_ver)
        {
            for( j = 0 ; j < list.length() ; j++ )
            {
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
        }
    }

    TextList tmp;
    for(i = 0 ; i < tree.length() ; i++)
    {
        tmp += tree.at(i).text();
    }
    // set tmp as new m_words
    setWordList(tmp);

    return tree;
}

/**
 * Add spaces in between words in a line
 */
void TextPagePrivate::addNecessarySpace(RegionTextList tree)
{
    /**
     * 1. Call makeAndSortLines before adding spaces in between words in a line
     * 2. Now add spaces between every two words in a line
     * 3. Finally, extract all the space separated texts from each region and
     *    make m_words nice again.
     */

    const int pageWidth = m_page->m_page->width();
    const int pageHeight = m_page->m_page->height();

    // Only change the texts under RegionTexts, not the area
    for(int j = 0 ; j < tree.length() ; j++)
    {
        RegionText &tmpRegion = tree[j];
        SortedTextList lines;
        LineRect line_rects;

        // Step 01
        makeAndSortLines(tmpRegion.text(), &lines, &line_rects);

        // Step 02
        for(int i = 0 ; i < lines.length() ; i++)
        {
            TextList &list = lines[i];
            for(int k = 0 ; k < list.length() ; k++ )
            {
                const QRect area1 = list.at(k)->area.roundedGeometry(pageWidth,pageHeight);
                if( k+1 >= list.length() ) break;

                const QRect area2 = list.at(k+1)->area.roundedGeometry(pageWidth,pageHeight);
                const int space = area2.left() - area1.right();

                if(space != 0)
                {
                    // Make a TinyTextEntity of string space and push it between it and it+1
                    const int left = area1.right();
                    const int right = area2.left();
                    const int top = area2.top() < area1.top() ? area2.top() : area1.top();
                    const int bottom = area2.bottom() > area1.bottom() ? area2.bottom() : area1.bottom();

                    const QString spaceStr(" ");
                    const QRect rect(QPoint(left,top),QPoint(right,bottom));
                    const NormalizedRect entRect(rect,pageWidth,pageHeight);
                    TinyTextEntity *ent = new TinyTextEntity(spaceStr,entRect);

                    list.insert(k+1,ent);

                    // Skip the space
                    k++;
                }
            }
        }

        TextList tmpList;
        for(int i = 0 ; i < lines.length() ; i++)
        {
            tmpList += lines.at(i);
        }
        tmpRegion.setText(tmpList);
    }

    // Step 03
    TextList tmp;
    for(int i = 0 ; i < tree.length() ; i++)
    {
        tmp += tree.at(i).text();
    }
    setWordList(tmp);
}

/**
 * Break Words into Characters, takes Entities from m_words and for each of
 * them insert the character entities in tmp. Finally, copies tmp back to m_words
 */
void TextPagePrivate::breakWordIntoCharacters(const QHash<QRect, RegionText> &word_chars_map)
{
    const QString spaceStr(" ");
    TextList tmp;
    const int pageWidth = m_page->m_page->width();
    const int pageHeight = m_page->m_page->height();

    for(int i = 0 ; i < m_words.length() ; i++)
    {
        TinyTextEntity *ent = m_words.at(i);
        const QRect rect = ent->area.geometry(pageWidth,pageHeight);

        // the spaces contains only one character, so we can skip them
        if(ent->text() == spaceStr)
            tmp.append( new TinyTextEntity(ent->text(),ent->area) );
        else
        {
            RegionText word_text;

            QHash<QRect, RegionText>::const_iterator it = word_chars_map.find(rect);
            while( it != word_chars_map.end() && it.key() == rect )
            {
                word_text = it.value();
                    
                if (ent->text() == word_text.string())
                    break;
                    
                ++it;
            }
            tmp.append(word_text.text());
        }
    }
    setWordList(tmp);
}


/**
 * Correct the textOrder, all layout recognition works here
 */
void TextPagePrivate::correctTextOrder()
{
    /**
     * Remove spaces from the text
     */
    removeSpace();

    /**
     * Construct words from characters
     */
    const QHash<QRect, RegionText> word_chars_map = makeWordFromCharacters();

    SortedTextList lines;
    LineRect line_rects;
    /**
     * Create arbitrary lines from words and sort them according to X and Y position
     */
    makeAndSortLines(m_words, &lines, &line_rects);

    /**
     * Calculate statistical information which will be needed later for algorithm implementation
     */
    int word_spacing, line_spacing, col_spacing;
    calculateStatisticalInformation(lines, line_rects, &word_spacing, &line_spacing, &col_spacing);
    for(int i = 0 ; i < lines.length() ; i++)
    {
       qDeleteAll(lines.at(i));
    }
    lines.clear();

    /**
     * Make a XY Cut tree for segmentation of the texts
     */
    const RegionTextList tree = XYCutForBoundingBoxes(word_spacing * 2, line_spacing * 2);

    /**
     * Add spaces to the word
     */
    addNecessarySpace(tree);

    /**
     * Break the words into characters
     */
    breakWordIntoCharacters(word_chars_map);
}

TextEntity::List TextPage::words(const RegularAreaRect *area, TextAreaInclusionBehaviour b) const
{
    if ( area && area->isNull() )
        return TextEntity::List();

    TextEntity::List ret;
    if ( area )
    {
        foreach (TinyTextEntity *te, d->m_words)
        {
            if (b == AnyPixelTextAreaInclusionBehaviour)
            {
                if ( area->intersects( te->area ) )
                {
                    ret.append( new TextEntity( te->text(), new Okular::NormalizedRect( te->area) ) );
                }
            }
            else
            {
                const NormalizedPoint center = te->area.center();
                if ( area->contains( center.x, center.y ) )
                {
                    ret.append( new TextEntity( te->text(), new Okular::NormalizedRect( te->area) ) );
                }
            }
        }
    }
    else
    {
        foreach (TinyTextEntity *te, d->m_words)
        {
            ret.append( new TextEntity( te->text(), new Okular::NormalizedRect( te->area) ) );
        }
    }
    return ret;
}
