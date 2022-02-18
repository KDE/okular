/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _PREFERENCESDIALOG_H
#define _PREFERENCESDIALOG_H

#include "part.h"
#include "settings.h"
#include <KConfigDialog>

class QWidget;
class KConfigSkeleton;

class DlgGeneral;
class DlgPerformance;
class DlgAccessibility;
class DlgPresentation;
class DlgAnnotations;
class DlgEditor;
class DlgSignatures;
class DlgDebug;

class PreferencesDialog : public KConfigDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget *parent, KConfigSkeleton *skeleton, Okular::EmbedMode embedMode, const QString &editCmd);

    void switchToAccessibilityPage();
    void switchToAnnotationsPage();

protected:
    //      void updateSettings(); // Called when OK/Apply is pressed.
    //      void updateWidgets(); // Called upon construction or when Reset is pressed
    //      void updateWidgetsDefault(); // Called when Defaults button is pressed
    //      bool hasChanged(); // In order to correctly disable/enable Apply button
    //      bool isDefault(); //  In order to correctly disable/enable Defaults button

private:
    DlgGeneral *m_general;
    DlgPerformance *m_performance;
    DlgAccessibility *m_accessibility;
    DlgPresentation *m_presentation;
    DlgAnnotations *m_annotations;
    DlgEditor *m_editor;
    DlgSignatures *m_signatures;
#ifdef OKULAR_DEBUG_CONFIGPAGE
    DlgDebug *m_debug;
#endif

    KPageWidgetItem *m_accessibilityPage;
    KPageWidgetItem *m_annotationsPage;
};

#endif
