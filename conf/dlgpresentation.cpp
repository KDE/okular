/***************************************************************************
 *   Copyright (C) 2006,2008 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgpresentation.h"

#include "ui_dlgpresentationbase.h"

#include <qapplication.h>
#include <qdesktopwidget.h>

#include <klocale.h>

#include "settings.h"

DlgPresentation::DlgPresentation( QWidget * parent )
    : QWidget( parent )
{
    m_dlg = new Ui_DlgPresentationBase();
    m_dlg->setupUi( this );

    QStringList choices;
    choices.append( i18nc( "@label:listbox The current screen, for the presentation mode", "Current Screen" ) );
    choices.append( i18nc( "@label:listbox The default screen for the presentation mode", "Default Screen" ) );
    const int screenCount = QApplication::desktop()->numScreens();
    for ( int i = 0; i < screenCount; ++i )
    {
        choices.append( i18nc( "@label:listbox %1 is the screen number (0, 1, ...)", "Screen %1", i ) );
    }
    m_dlg->screenCombo->addItems( choices );

    const int screen = Okular::Settings::slidesScreen();
    if ( screen >= -2 && screen < screenCount )
    {
        m_dlg->screenCombo->setCurrentIndex( screen + 2 );
    }
    else
    {
        m_dlg->screenCombo->setCurrentIndex( 0 );
        Okular::Settings::setSlidesScreen( -2 );
    }
    
    m_dlg->kcfg_SlidesAdvanceTime->setSuffix(ki18ncp("Advance every %1 seconds", " second", " seconds"));

    connect( m_dlg->screenCombo, SIGNAL(activated(int)), this, SLOT(screenComboChanged(int)) );
}

DlgPresentation::~DlgPresentation()
{
    delete m_dlg;
}

void DlgPresentation::screenComboChanged( int which )
{
    Okular::Settings::setSlidesScreen( which - 2 );
}

#include "dlgpresentation.moc"
