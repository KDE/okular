/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _DLGPERFORMANCE_H
#define _DLGPERFORMANCE_H

#include <QWidget>

class QLabel;

class DlgPerformance : public QWidget
{
    Q_OBJECT

public:
    explicit DlgPerformance(QWidget *parent = nullptr);

protected Q_SLOTS:
    void slotMemoryLevelSelected(int which);

protected:
    QLabel *m_memoryExplanationLabel;
};

#endif
