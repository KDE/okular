/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DLGACCESSIBILITY_H
#define _DLGACCESSIBILITY_H

#include <QWidget>

class QStackedWidget;

class DlgAccessibility : public QWidget
{
    Q_OBJECT

public:
    explicit DlgAccessibility(QWidget *parent = nullptr);

protected Q_SLOTS:
    void slotColorModeSelected(int mode);

protected:
    QStackedWidget *m_colorModeConfigStack;
};

#endif
