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

#include "kpdf_pagewidget.h"
#include "thumbnaillist.h"

/*
 *  Constructs a PDFPartView as a child of 'parent', with the
 *  name 'name' 
 */
PDFPartView::PDFPartView(QWidget* parent, const char* name, QMutex *docMutex) : QWidget(parent, name)
{
    PDFPartViewLayout = new QHBoxLayout( this, 3, 3, "PDFPartViewLayout"); 

    pagesList = new ThumbnailList(this, docMutex);
    pagesList->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)7, 0, 0, pagesList->sizePolicy().hasHeightForWidth() ) );
    pagesList->setMaximumSize( QSize( 75, 32767 ) );
    pagesList->setColumnWidth(0, 75);
    PDFPartViewLayout->addWidget( pagesList );

    outputdev = new KPDF::PageWidget( this, "outputdev", docMutex );
    PDFPartViewLayout->addWidget( outputdev );
    resize( QSize(623, 381).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
    
    connect(pagesList, SIGNAL(clicked(int)), this, SIGNAL(clicked(int)));
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
    pagesList->setCurrentItem(i);
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

void PDFPartView::showPageList(bool show)
{
    if (show) pagesList->show();
    else pagesList->hide();
}

#include "part.moc"
