/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <klocale.h>

// single config pages
/*
#include "dlggeneral.h"
#include "dlgaccessibility.h"
*/

// reimplementing this
#include "preferencesdialog.h"

PreferencesDialog::PreferencesDialog( QWidget * parent, KConfigSkeleton * skeleton )
    : KConfigDialog( parent, "preferences", skeleton )
{
/*
    m_general = new DlgGeneral(0);
    m_accessibility = new DlgAccessibility(0);

    addPage( m_general, i18n("General"), "looknfeel", i18n("General setup") );
    addPage( m_accessibility, i18n("Accessibility"), "folder_open", i18n("Read aids") );
*/
}
