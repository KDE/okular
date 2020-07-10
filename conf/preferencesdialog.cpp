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

#include <KLocalizedString>

// single config pages
#include "dlgaccessibility.h"
#include "dlgannotations.h"
#include "dlgdebug.h"
#include "dlgeditor.h"
#include "dlggeneral.h"
#include "dlgperformance.h"
#include "dlgpresentation.h"

PreferencesDialog::PreferencesDialog(QWidget *parent, KConfigSkeleton *skeleton, Okular::EmbedMode embedMode)
    : KConfigDialog(parent, QStringLiteral("preferences"), skeleton)
{
    setWindowModality(Qt::ApplicationModal);

    m_general = new DlgGeneral(this, embedMode);
    m_performance = new DlgPerformance(this);
    m_accessibility = new DlgAccessibility(this);
    m_presentation = nullptr;
    m_annotations = nullptr;
    m_annotationsPage = nullptr;
    m_editor = nullptr;
#ifdef OKULAR_DEBUG_CONFIGPAGE
    m_debug = new DlgDebug(this);
#endif

    addPage(m_general, i18n("General"), QStringLiteral("okular"), i18n("General Options"));
    addPage(m_accessibility, i18n("Accessibility"), QStringLiteral("preferences-desktop-accessibility"), i18n("Accessibility Reading Aids"));
    addPage(m_performance, i18n("Performance"), QStringLiteral("preferences-system-performance"), i18n("Performance Tuning"));
    if (embedMode == Okular::ViewerWidgetMode) {
        setWindowTitle(i18n("Configure Viewer"));
    } else {
        m_presentation = new DlgPresentation(this);
        m_annotations = new DlgAnnotations(this);
        m_editor = new DlgEditor(this);
        addPage(m_presentation, i18n("Presentation"), QStringLiteral("view-presentation"), i18n("Options for Presentation Mode"));
        m_annotationsPage = addPage(m_annotations, i18n("Annotations"), QStringLiteral("draw-freehand"), i18n("Annotation Options"));
        addPage(m_editor, i18n("Editor"), QStringLiteral("accessories-text-editor"), i18n("Editor Options"));
    }
#ifdef OKULAR_DEBUG_CONFIGPAGE
    addPage(m_debug, "Debug", "system-run", "Debug options");
#endif
    setHelp(QStringLiteral("configure"), QStringLiteral("okular"));
}

void PreferencesDialog::switchToAnnotationsPage()
{
    if (m_annotationsPage)
        setCurrentPage(m_annotationsPage);
}
