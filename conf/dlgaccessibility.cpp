/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgaccessibility.h"

#include "ui_dlgaccessibilitybase.h"

DlgAccessibility::DlgAccessibility( QWidget * parent )
    : QWidget( parent ), m_selected( 0 )
{
    m_dlg = new Ui_DlgAccessibilityBase();
    m_dlg->setupUi( this );

    // ### not working yet, hide for now
    m_dlg->kcfg_HighlightImages->hide();

    m_color_pages.append( m_dlg->page_invert );
    m_color_pages.append( m_dlg->page_paperColor );
    m_color_pages.append( m_dlg->page_darkLight );
    m_color_pages.append( m_dlg->page_bw );
    foreach ( QWidget * page, m_color_pages )
        page->hide();
    m_color_pages[ m_selected ]->show();

    connect( m_dlg->kcfg_RenderMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotColorMode(int)) );
}

DlgAccessibility::~DlgAccessibility()
{
    delete m_dlg;
}

void DlgAccessibility::slotColorMode( int mode )
{
    m_color_pages[ m_selected ]->hide();
    m_color_pages[ mode ]->show();

    m_selected = mode;
}

#include "dlgaccessibility.moc"
