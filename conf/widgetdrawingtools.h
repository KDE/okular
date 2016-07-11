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
    explicit WidgetDrawingTools( QWidget * parent = Q_NULLPTR );
    ~WidgetDrawingTools();

    QStringList tools() const Q_DECL_OVERRIDE;
    void setTools( const QStringList& items ) Q_DECL_OVERRIDE;
    
    QString defaultName() const;

protected Q_SLOTS:
    void slotAdd() Q_DECL_OVERRIDE;
    void slotEdit() Q_DECL_OVERRIDE;
};

#endif // WIDGETDRAWINGTOOLS_H
