/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef THUMBNAIL_H
#define THUMBNAIL_H
 
#include <qcolor.h>
#include <qimage.h>
#include <qvbox.h>

class QLabel;
class KPDFPage;

class Thumbnail : public QVBox
{
    Q_OBJECT
public:
    Thumbnail(QWidget *parent, const QColor &color, const KPDFPage *page);

    int pageNumber();
    void setImage(const QImage *thumbnail);
    int setThumbnailWidth( int w );
    void setSelected(bool selected);

private:
    QWidget *m_thumbnailW;
    QLabel *m_label;
    const KPDFPage *m_page;
    QColor m_backgroundColor;
    QImage m_original;
};

#endif
