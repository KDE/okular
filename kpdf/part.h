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

#include <qwidget.h>

class QMutex;

namespace KPDF {
    class PageWidget;
}

class LinkAction;
class PDFDoc;

class ThumbnailList;
class TOC;

class PDFPartView : public QWidget
{
    Q_OBJECT

public:
    PDFPartView(QWidget* parent, const char* name, QMutex *docMutex);
    ~PDFPartView();

    // first page is page 1
    void setCurrentThumbnail(int i);

    void setPages(int i, double ar);
    void generateThumbnails(PDFDoc *doc);
    void stopThumbnailGeneration();

    void generateTOC(PDFDoc *doc);

    void showPageList(bool show);
    void showTOC(bool show);

    // TODO make private
    KPDF::PageWidget* outputdev;

signals:
    void clicked(int);
    void execute(LinkAction*);
    void hasTOC(bool);

private:
    void updateShowTOC();

    ThumbnailList *pagesList;
    TOC *toc;
    bool m_showTOC, m_existsTOC;
};

#endif // PDFPARTVIEW_H
