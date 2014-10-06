/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _CHOOSEENGINEDIALOG_H
#define _CHOOSEENGINEDIALOG_H

#include <QtCore/QStringList>

#include <QDialog>
#include <QMimeType>
#include <QMimeDatabase>

class Ui_ChooseEngineWidget;

class ChooseEngineDialog : public QDialog
{
    public:
        ChooseEngineDialog( const QStringList &generators, const QMimeType &mime, QWidget * parent = 0 );
        ~ChooseEngineDialog();

        int selectedGenerator() const;

    protected:
        Ui_ChooseEngineWidget * m_widget;
};

#endif
