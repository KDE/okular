/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PDFPARTVIEW_H
#define PDFPARTVIEW_H

#include <qpixmap.h>
#include <qwidget.h>

class QHBoxLayout;
class QPixmap;

namespace KPDF {
class PageWidget;
}

class ThumbnailList;

class PDFPartView : public QWidget
{
    Q_OBJECT

public:
    PDFPartView(QWidget* parent, const char* name);
    ~PDFPartView();
    
    void setCurrentItem(int i);
    
    void setPages(int i, double ar);
    void setThumbnail(int i, const QPixmap *thumbnail);
    
    void showPageList(bool show);

    // TODO make private
    KPDF::PageWidget* outputdev;

signals:
    void clicked(int);

protected:
    QHBoxLayout* PDFPartViewLayout;

private:
    ThumbnailList* pagesList;
};

#endif // PDFPARTVIEW_H
