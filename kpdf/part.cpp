/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "part.h"

#include <qlayout.h>
#include <qsplitter.h>
#include <qtable.h>

#include "kpdf_pagewidget.h"
#include "thumbnaillist.h"
#include "toc.h"

/*
 *  Constructs a PDFPartView as a child of 'parent', with the
 *  name 'name'
 */
PDFPartView::PDFPartView(QWidget* parent, const char* name, QMutex *docMutex) : QWidget(parent, name)
{
    QHBoxLayout* PDFPartViewLayout = new QHBoxLayout(this, 3, 3);

    pagesList = new ThumbnailList(this, docMutex);
    pagesList->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)7, 0, 0, pagesList->sizePolicy().hasHeightForWidth() ) );
    pagesList->setMaximumSize(75, 32767);
    pagesList->setColumnWidth(0, 75);
    PDFPartViewLayout->addWidget( pagesList );

    QSplitter *split = new QSplitter(this);
    PDFPartViewLayout->addWidget(split);

    toc = new TOC(split);
    toc->hide();

    outputdev = new KPDF::PageWidget( split, "outputdev", docMutex );

    m_existsTOC = false;

    connect(pagesList, SIGNAL(clicked(int)), this, SIGNAL(clicked(int)));
    connect(toc, SIGNAL(execute(LinkAction*)), this, SIGNAL(execute(LinkAction*)));
}

/*
 *  Destroys the object and frees any allocated resources
 */
PDFPartView::~PDFPartView()
{
    // no need to delete child widgets, Qt does it all for us
}

void PDFPartView::setCurrentThumbnail(int i)
{
    // ThumbnailList is 0 based
    pagesList->setCurrentItem(i-1);
}

void PDFPartView::setPages(int i, double ar)
{
    pagesList->setPages(i, ar);
}

void PDFPartView::generateThumbnails(PDFDoc *doc)
{
    pagesList->generateThumbnails(doc);
}

void PDFPartView::stopThumbnailGeneration()
{
    pagesList->stopThumbnailGeneration();
}

void PDFPartView::generateTOC(PDFDoc *doc)
{
    m_existsTOC = toc->generate(doc);
    emit hasTOC(m_existsTOC);
    updateShowTOC();
}

void PDFPartView::showPageList(bool show)
{
    if (show) pagesList->show();
    else pagesList->hide();
}

void PDFPartView::showTOC(bool show)
{
    m_showTOC = show;
    updateShowTOC();
}

void PDFPartView::updateShowTOC()
{
    if (m_showTOC && m_existsTOC) toc->show();
    else toc->hide();
}

#include "part.moc"
