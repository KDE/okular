/***************************************************************************
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "core/document.h"
#include "pagesizelabel.h"

PageSizeLabel::PageSizeLabel( QWidget * parent, Okular::Document * document )
    : QLabel( parent ), m_document( document ),
    m_currentPage( -1 ), m_antiWidget( NULL )
{
}

PageSizeLabel::~PageSizeLabel()
{
    m_document->removeObserver( this );
}

QWidget *PageSizeLabel::antiWidget()
{
    if (!m_antiWidget)
    {
        m_antiWidget = new QWidget(qobject_cast<QWidget*>(parent()));
        m_antiWidget->resize(0, 0);
    }
    return m_antiWidget;
}

void PageSizeLabel::notifySetup( const QVector< Okular::Page * > & pageVector, bool changed )
{
    // only process data when document changes
    if ( !changed )
        return;

    // if document is closed or all pages have size hide widget
    int pages = pageVector.count();
    if ( pages < 1 || m_document->allPagesSize().isValid() )
    {
        hide();
        return;
    }
    else show();
}

void PageSizeLabel::notifyViewportChanged( bool /*smoothMove*/ )
{
    if (isVisible())
    {
        // get current page number
        int page = m_document->viewport().pageNumber;
        int pages = m_document->pages();

        // if the document is opened and page is changed
        if ( page != m_currentPage && pages > 0 )
        {
            m_currentPage = page;
            setText( m_document->pageSizeString(page) );
            m_antiWidget->setFixedSize(sizeHint());
        }
   }
}

#include "pagesizelabel.moc"
