/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
