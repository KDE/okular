/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef DLGSIGNATURES_H
#define DLGSIGNATURES_H

#include <qwidget.h>

class Ui_DlgSignaturesBase;

class DlgSignatures : public QWidget
{
    Q_OBJECT

public:
    explicit DlgSignatures(QWidget *parent = nullptr);
    virtual ~DlgSignatures();

private:
    Ui_DlgSignaturesBase *m_dlg;
};

#endif
