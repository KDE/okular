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

#include <qwidget.h>

class QLabel;
class KPDFPage;

class Thumbnail : public QWidget
{
public:
    Thumbnail(QWidget *parent, const KPDFPage *page);

    // resize / select commands
    int setThumbnailWidth(int width);
    void setSelected(bool selected);

    // query methods
    int pageNumber() const;
    int previewWidth() const;
    int previewHeight() const;

protected:
    void paintEvent(QPaintEvent *);

private:
    QLabel *m_label;
    const KPDFPage *m_page;
    uint m_previewWidth;
    uint m_previewHeight;
};

#endif
