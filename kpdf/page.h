/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGE_H_
#define _KPDF_PAGE_H_

#include <qobject.h>
#include <qsize.h>
#include <qmutex.h>
#include "document.h"

class QPixmap;
class QString;
class QRect;

class TextOutputDev;
class PageOverlay{ /*fake temp*/ };

/**
 * @short Collector for all the data belonging to a page.
 *
 * The Page class contains its pixmap, the thumbnail, a search page (a class
 * used internally for searching data) the modifiers descriptors (for overlay
 * graphics) and more.
 * It provides accessor methods for all those operations too.
 * 
 * Note: add stuff this class contains is destroyed automatically when the
 * class is destroyed.
 */

class KPDFPage : public QObject, public KPDFDocumentObserver
{
    Q_OBJECT
    
public:
    KPDFPage( uint number, float width, float height );
    ~KPDFPage();

    // query methods
    uint number() const { return m_number; }
    float width() const { return m_width; }
    float height() const { return m_height; }
    float ratio() const { return m_height / m_width; }

    float currentZoom() const { return m_zoom; }
    const QSize & currentSize() const { return m_size; }
    void drawPixmap( QPainter * p, const QRect & rect );
    void drawThumbnail( QPainter * p );

    // find related methods
    bool hasText( QString & text );
    //const QRect & textPosition();

signals:
    void changed( KPDFPage * thisPage );

private slots:
    void slotSetZoom( float scale );
    void slotSetContents( QPixmap * );
    void slotSetThumbnail( QPixmap * );
    void slotSetOverlay( /*..DOMdescription..*/ );

private:
    QMutex threadLock;

    uint m_number;
    float m_width, m_height;
    float m_zoom;
    QSize m_size;
    QPixmap * m_pixmap;
    QPixmap * m_thumbnail;
    TextOutputDev * m_text;
    PageOverlay * m_overlay;
};

#endif
