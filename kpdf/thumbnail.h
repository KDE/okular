/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid                               *
 *   tsdgeos@terra.es                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef THUMBNAIL_H
#define THUMBNAIL_H
 
#include <qcolor.h>
#include <qvbox.h>

class QLabel;

class Thumbnail : public QVBox
{
    Q_OBJECT
public:
    Thumbnail(QWidget *parent, const QString &text, const QColor &color, int height, int width);

public slots:
    void setPixmap(const QPixmap *thumbnail);
    void setPixmapSize(int height, int width);
    int getPixmapHeight() const;
    void setSelected(bool selected);
    int labelSizeHintHeight();

protected:
    void resizeEvent(QResizeEvent *);

private:
    QWidget *m_thumbnailW;
    QLabel *m_label;
    QColor m_backgroundColor;
    QImage m_original;
};

#endif
