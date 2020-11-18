/***************************************************************************
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pagesizelabel.h"

#include "core/document.h"

PageSizeLabel::PageSizeLabel(QWidget *parent, Okular::Document *document)
    : KSqueezedTextLabel(parent)
    , m_document(document)
{
    setAlignment(Qt::AlignRight);
}

PageSizeLabel::~PageSizeLabel()
{
    m_document->removeObserver(this);
}

void PageSizeLabel::notifyCurrentPageChanged(int previousPage, int currentPage)
{
    Q_UNUSED(previousPage)

    // if the document is opened
    if (m_document->pages() > 0 && !m_document->allPagesSize().isValid()) {
        setText(m_document->pageSizeString(currentPage));
    }
}

#include "moc_pagesizelabel.cpp"
