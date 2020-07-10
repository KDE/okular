/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DLGGENERAL_H
#define _DLGGENERAL_H

#include <qwidget.h>

#include "part.h"

class Ui_DlgGeneralBase;

class DlgGeneral : public QWidget
{
    Q_OBJECT

public:
    explicit DlgGeneral(QWidget *parent, Okular::EmbedMode embedMode);
    ~DlgGeneral() override;

protected:
    void showEvent(QShowEvent *) override;

    Ui_DlgGeneralBase *m_dlg;
};

#endif
