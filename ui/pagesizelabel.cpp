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

PageSizeLabel::PageSizeLabel( QWidget * parent, Okular::Document * document )
    : QLabel( parent ), m_document( document ),
    m_antiWidget( NULL )
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

void PageSizeLabel::notifySetup( const QVector< Okular::Page * > & pageVector, int setupFlags )
{
    // only process data when document changes
    if ( !( setupFlags & Okular::DocumentObserver::DocumentChanged ) )
        return;

    // if document is closed or all pages have size hide widget
    int pages = pageVector.count();
    if ( pages < 1 || m_document->allPagesSize().isValid() )
    {
        hide();
        if ( m_antiWidget )
            m_antiWidget->hide();
        return;
    }
    else
    {
        show();
        if ( m_antiWidget )
            m_antiWidget->show();
    }
}

void PageSizeLabel::notifyCurrentPageChanged( int previousPage, int currentPage )
{
    Q_UNUSED( previousPage )

    if (isVisible())
    {
        // if the document is opened
        if ( m_document->pages() > 0 )
        {
            setText( m_document->pageSizeString( currentPage ) );
            m_antiWidget->setFixedSize( sizeHint() );
        }
   }
}

#include "pagesizelabel.moc"
