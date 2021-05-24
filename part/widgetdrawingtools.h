/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WIDGETDRAWINGTOOLS_H
#define WIDGETDRAWINGTOOLS_H

#include "widgetconfigurationtoolsbase.h"

class WidgetDrawingTools : public WidgetConfigurationToolsBase
{
    Q_OBJECT
public:
    explicit WidgetDrawingTools(QWidget *parent = nullptr);
    ~WidgetDrawingTools() override;

    QStringList tools() const override;
    void setTools(const QStringList &items) override;

    QString defaultName() const;

protected Q_SLOTS:
    void slotAdd() override;
    void slotEdit() override;
};

#endif // WIDGETDRAWINGTOOLS_H
