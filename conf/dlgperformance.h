/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DLGPERFORMANCE_H
#define _DLGPERFORMANCE_H

#include <QWidget>

class Ui_DlgPerformanceBase;

class DlgPerformance : public QWidget
{
    Q_OBJECT

public:
    explicit DlgPerformance(QWidget *parent = nullptr);
    ~DlgPerformance() override;

protected Q_SLOTS:
    void radioGroup_changed(int which);

protected:
    Ui_DlgPerformanceBase *m_dlg;
};

#endif
