/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _DLGACCESSIBILITY_H
#define _DLGACCESSIBILITY_H

#include <QWidget>

class QComboBox;
class QStackedWidget;

class DlgAccessibility : public QWidget
{
    Q_OBJECT

public:
    explicit DlgAccessibility(QWidget *parent = nullptr);

protected Q_SLOTS:
    void slotColorModeSelected(int mode);
#ifdef HAVE_SPEECH
    void slotTTSEngineChanged();
#endif

protected:
    QStackedWidget *m_colorModeConfigStack;
#ifdef HAVE_SPEECH
    QComboBox *m_ttsEngineBox;
    QComboBox *m_ttsVoiceBox;
#endif
};

#endif
