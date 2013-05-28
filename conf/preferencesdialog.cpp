/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// reimplementing this
#include "preferencesdialog.h"

#include <klocale.h>

// single config pages
#include "dlggeneral.h"
#include "dlgperformance.h"
#include "dlgaccessibility.h"
#include "dlgpresentation.h"
#include "dlgannotations.h"
#include "dlgeditor.h"
#include "dlgdebug.h"

PreferencesDialog::PreferencesDialog( QWidget * parent, KConfigSkeleton * skeleton, Okular::EmbedMode embedMode )
    : KConfigDialog( parent, "preferences", skeleton )
{
    setWindowModality( Qt::ApplicationModal );

    m_general = new DlgGeneral( this, embedMode );
    m_performance = new DlgPerformance( this );
    m_accessibility = new DlgAccessibility( this );
    m_presentation = 0;
    m_annotations = 0;
    m_annotationsPage = 0;
    m_editor = 0;
#ifdef OKULAR_DEBUG_CONFIGPAGE
    m_debug = new DlgDebug( this );
#endif

    addPage( m_general, i18n("General"), "okular", i18n("General Options") );
    addPage( m_accessibility, i18n("Accessibility"), "preferences-desktop-accessibility", i18n("Accessibility Reading Aids") );
    addPage( m_performance, i18n("Performance"), "preferences-system-performance", i18n("Performance Tuning") );
    if( embedMode == Okular::ViewerWidgetMode )
    {
        setCaption( i18n("Configure Viewer") );
    }
    else
    {
        m_presentation = new DlgPresentation( this );
        m_annotations = new DlgAnnotations( this );
        m_editor = new DlgEditor( this );
        addPage( m_presentation, i18n("Presentation"), "view-presentation",
                 i18n("Options for Presentation Mode") );
        m_annotationsPage = addPage( m_annotations, i18n("Annotations"),
                                     "draw-freehand", i18n("Annotation Options") );
        addPage( m_editor, i18n("Editor"), "accessories-text-editor", i18n("Editor Options") );
    }
#ifdef OKULAR_DEBUG_CONFIGPAGE
    addPage( m_debug, "Debug", "system-run", "Debug options" );
#endif
    setHelp("configure", "okular");
}

void PreferencesDialog::switchToAnnotationsPage()
{
    if ( m_annotationsPage )
        setCurrentPage( m_annotationsPage );
}
