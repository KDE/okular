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

#include <qwidget.h>

class Ui_DlgPerformanceBase;

class DlgPerformance : public QWidget
{
    Q_OBJECT

    public:
        DlgPerformance( QWidget * parent = 0 );

    protected slots:
        void lowRadio_toggled( bool on );
        void normalRadio_toggled( bool on );
        void aggressiveRadio_toggled( bool on );

    protected:
        Ui_DlgPerformanceBase * m_dlg;
};

#endif
