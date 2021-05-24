/*
    SPDX-FileCopyrightText: 2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
