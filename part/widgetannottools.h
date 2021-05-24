/*
    SPDX-FileCopyrightText: 2012 Fabio D 'Urso <fabiodurso@hotmail.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _WIDGETANNOTTOOLS_H_
#define _WIDGETANNOTTOOLS_H_

#include "widgetconfigurationtoolsbase.h"

#include <qdom.h>
#include <qwidget.h>

class WidgetAnnotTools : public WidgetConfigurationToolsBase
{
    Q_OBJECT
public:
    explicit WidgetAnnotTools(QWidget *parent = nullptr);
    ~WidgetAnnotTools() override;

    QStringList tools() const override;
    void setTools(const QStringList &items) override;

protected Q_SLOTS:
    void slotAdd() override;
    void slotEdit() override;
};

#endif
