/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PIXMAPWIDGET_H_
#define _PIXMAPWIDGET_H_

#include <qwidget.h>

class KPDFPage;

/**
 * @short Graphical representation of a page.
 * ...
 */
class PixmapWidget : public QWidget
{
public:
    PixmapWidget( QWidget * parent, const KPDFPage * page );

    // internal size/placements evaluators
    void setZoomFixed( double magFactor = 1.0 );
    void setZoomFitWidth( int width );
    void setZoomFitRect( int rectWidth, int rectHeight );
    float zoomFactor() const { return m_zoomFactor; }

    // full size (for resizing) and inner pixmap size
    int widthHint() const;
    int heightHint() const;
    int pixmapWidth() const { return m_pixmapWidth; }
    int pixmapHeight() const { return m_pixmapHeight; }

    // other queries
    int pageNumber() const;

protected:
    void setPixmapMargins( int left, int top, int right, int bottom );

    const KPDFPage * m_page;
    int m_marginLeft, m_marginTop, m_marginRight, m_marginBottom;
    int m_pixmapWidth, m_pixmapHeight;
    float m_zoomFactor;
};


/**
 * @short ThumbnailList's selectable page.
 * ...
 */
class ThumbnailWidget : public PixmapWidget
{
public:
    ThumbnailWidget( QWidget *parent, const KPDFPage *page );

    // set the thumbnail selected state
    void setSelected( bool selected );

protected:
    void paintEvent(QPaintEvent *);

private:
    bool m_selected;
    int m_labelNumber;
    int m_labelHeight;
};


/**
 * @short PageView's editable page. Renders page, outline and overlay GFX.
 * ...
 */
class PageWidget : public PixmapWidget
{
public:
    PageWidget( QWidget * parent, const KPDFPage * page );

protected:
    void paintEvent(QPaintEvent *);
};

#endif
