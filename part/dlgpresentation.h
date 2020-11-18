/***************************************************************************
 *   Copyright (C) 2006,2008 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DLGPRESENTATION_H
#define _DLGPRESENTATION_H

#include <qwidget.h>

class Ui_DlgPresentationBase;

class DlgPresentation : public QWidget
{
    Q_OBJECT

public:
    explicit DlgPresentation(QWidget *parent = nullptr);
    ~DlgPresentation() override;

protected Q_SLOTS:
    void screenComboChanged(int which);

protected:
    Ui_DlgPresentationBase *m_dlg;
};

#endif
