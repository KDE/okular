/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _DLGGENERAL_H
#define _DLGGENERAL_H

#include <QWidget>

#include "part.h"

class DlgGeneral : public QWidget
{
    Q_OBJECT

public:
    explicit DlgGeneral(QWidget *parent, Okular::EmbedMode embedMode);
};

#endif
