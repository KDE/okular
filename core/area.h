/***************************************************************************
 *   Copyright (C) 2004-05 by Enrico Ros <eros.kde@email.it>               *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_AREA_H_
#define _KPDF_AREA_H_
#include <qvaluelist.h>
#include <qcolor.h>
class QRect;
class KPDFLink;

/**
 * @short A point in [0,1] coordinates (only used in annotations atm)
 */
class NormalizedPoint
{
    public:
        double x, y;

        NormalizedPoint();
        NormalizedPoint( double dX, double dY );
        NormalizedPoint( int ix, int iy, int xScale, int yScale );
};

/**
 * @short A rect in normalized [0,1] coordinates.
 */
class NormalizedRect
{
    public:
        double left, top, right, bottom;

        NormalizedRect();
        NormalizedRect( double l, double t, double r, double b );
        NormalizedRect( const QRect &r, double xScale, double yScale );
        bool isNull() const;
        bool contains( double x, double y ) const;
        bool intersects( const NormalizedRect & normRect ) const;
        bool intersects( double l, double t, double r, double b ) const;
        bool intersects( const NormalizedRect * r ) const;
        QRect geometry( int xScale, int yScale ) const;
};


/**
 * @short NormalizedRect that contains a reference to an object.
 *
 * These rects contains a pointer to a kpdf object (such as a link or something
 * like that). The pointer is read and stored as 'void pointer' so cast is
 * performed by accessors based on the value returned by objectType(). Objects
 * are reparented to this class.
 *
 * Type / Class correspondency tab:
 *  - Link      : class KPDFLink  : description of a link
 *  - Image     : class KPDFImage : description of an image (n/a)
 */
class ObjectRect : public NormalizedRect
{
    public:
        // definition of the types of storable objects
        enum ObjectType { Link, Image };

        // default constructor: initialize all parameters
        ObjectRect( double l, double t, double r, double b, ObjectType typ, void * obj );
        ~ObjectRect();

        // query type and get a const pointer to the stored object
        inline ObjectType objectType() const { return m_objectType; }
        inline const void * pointer() const { return m_pointer; }

    private:
        ObjectType m_objectType;
        void * m_pointer;
};

/**
 * Internal Storage: normalized colored highlight owned by id
 */
struct HighlightRect : public NormalizedRect
{
    // searchID of the highlight owner
    int s_id;
    // color of the highlight
    QColor color;
};

/**
 * @short A regular area of NormalizedShape which normalizes a Shape
 * 
 * Class NormalizedShape must have the following functions defined: 
 * contains (double, double)
 * intersects(NormalizedShape)
 * isNull()
 * geometry(int,int)
 */

template <class NormalizedShape, class Shape> class RegularArea : 
public  QValueList<NormalizedShape*>
{
	public:
		typedef QValueListIterator<NormalizedShape*> Iterator;
        typedef QValueListConstIterator<NormalizedShape*> ConstIterator;
//        RegularArea<NormalizedShape,Shape> (NormalizedShape* x)  { QValueList(x) ; } ;
// 		class Iterator : public QValueListIterator<NormalizedShape*> {};
		bool contains( double x, double y ) const;
		bool intersects (const RegularArea<NormalizedShape,Shape> * area) const;
		bool intersects (const NormalizedShape * shape) const;
		void appendArea (const RegularArea<NormalizedShape,Shape> *area);
		bool isNull() const;
		QValueList<Shape>* geometry( int xScale, int yScale ) const;
};

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::isNull() const
{
	if (!this)
		return false;

	if (this->isEmpty())
		return false;

	ConstIterator i;
	for (i=this->begin();i!=this->end();++i)
		if (!((*i)->isNull()))
			return false;
	return true;

}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::intersects (const NormalizedShape *rect) const
{
	if (!this)
		return false;

	if (this->isEmpty())
		return false;

	ConstIterator i;
	for (i=this->begin();i!=this->end();++i)
	{
		if(!((*i)->isNull()) && (*i)->intersects (rect))
			return true;
	}
	return false;
}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::intersects 
	(const RegularArea<NormalizedShape,Shape> *area) const
{
	if (!this)
		return false;
	if (this->isEmpty())
		return false;

	Iterator i,j;
	for (i=this->begin();i!=this->end();++i)
	{
		for (j=area->begin();j!=area->end();++j)
		{
			if(!((*i)->isNull) && (*i)->intersects (j))
				return true;
		}
	}
	return false;
}

template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::appendArea
	(const RegularArea<NormalizedShape, Shape> *area) 
{
	if (!this)
		return false;

	ConstIterator j;
	for (j=area->begin();j!=area->end();++j)
	{
        this->append(*j);
	}
}


template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::contains (double x, double y) const
{
	if (!this)
		return false;
	if (this->isEmpty())
		return false;

	ConstIterator i;
	for (i=this->begin();i!=this->end();++i)
	{
		if((*i)->contains (x,y))
			return true;
	}
	return false;
}

template <class NormalizedShape, class Shape>
QValueList<Shape> *
RegularArea<NormalizedShape, Shape>::geometry( int xScale, int yScale ) const
{
	if (!this)
		return false;
	if (this->isEmpty()) 
		return 0;

	ConstIterator i;
	QValueList<Shape>* ret=0;

	for (i=this->begin();i!=this->end();++i)
	{
		ret.append((*i)->geometry(xScale,yScale));
	}

	return ret;
}

typedef RegularArea<NormalizedRect,QRect> RegularAreaRect;


class HighlightAreaRect : public RegularAreaRect {
	public:
		HighlightAreaRect(RegularAreaRect *area);
		// searchID of the highlight owner
		int s_id;
		// color of the highlight
		QColor color;
};

#endif
