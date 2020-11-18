/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
