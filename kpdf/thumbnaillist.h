/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid                               *
 *   tsdgeos@terra.es                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef THUMBNAILLIST_H
#define THUMBNAILLIST_H

#include <qimage.h> 
#include <qtable.h>

class ThumbnailList : public QTable
{
    Q_OBJECT
public:
    ThumbnailList(QWidget *parent);
    
    void setCurrentItem(int i);
    
    void setPages(int i, double ar);
    void setThumbnail(int i, const QPixmap *thumbnail);

private slots:
    void changeSelected(int i);

signals:
    void clicked(int);

protected:
    void viewportResizeEvent(QResizeEvent *);

private:
    void resizeThumbnails();

    double m_ar;
    int m_selected;
    int m_heightLimit;
};

#endif
