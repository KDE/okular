/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
class DlgDebug;

class PreferencesDialog : public KConfigDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget *parent, KConfigSkeleton *skeleton, Okular::EmbedMode embedMode);

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
#ifdef OKULAR_DEBUG_CONFIGPAGE
    DlgDebug *m_debug;
#endif

    KPageWidgetItem *m_annotationsPage;
};

#endif
