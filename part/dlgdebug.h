/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _DLGDEBUG_H
#define _DLGDEBUG_H

#include <qwidget.h>

class DlgDebug : public QWidget
{
    Q_OBJECT

public:
    explicit DlgDebug(QWidget *parent = nullptr);
};

#endif
