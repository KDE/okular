/***************************************************************************
 *   Copyright (C) 2019 by Bubli <Katarina.Behrens@cib.de>                 *
 *   Copyright (C) 2020 by Albert Astals Cid <albert.astals.cid@kdab.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PDFSETTINGSWIDGET_H
#define PDFSETTINGSWIDGET_H

#include <QWidget>

#include "ui_pdfsettingswidget.h"

class QTreeWidget;

class PDFSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PDFSettingsWidget(QWidget *parent = nullptr);
    bool event(QEvent *e) override;

private:
    void warnRestartNeeded();

    QTreeWidget *m_tree = nullptr;
    bool m_certificatesAsked = false;
    bool m_warnedAboutRestart = false;
    Ui_PDFSettingsWidgetBase m_pdfsw;
};

#endif
