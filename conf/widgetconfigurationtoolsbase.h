/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef WIDGETCONFIGURATIONTOOLSBASE_H
#define WIDGETCONFIGURATIONTOOLSBASE_H

#include <QWidget>
class QListWidget;
class QPushButton;

class WidgetConfigurationToolsBase : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QStringList tools READ tools WRITE setTools NOTIFY changed USER true)

public:
    explicit WidgetConfigurationToolsBase(QWidget *parent = nullptr);
    ~WidgetConfigurationToolsBase() override;

    virtual QStringList tools() const = 0;
    virtual void setTools(const QStringList &items) = 0;

Q_SIGNALS:
    void changed();

protected:
    QListWidget *m_list;

private:
    QPushButton *m_btnAdd;
    QPushButton *m_btnEdit;
    QPushButton *m_btnRemove;
    QPushButton *m_btnMoveUp;
    QPushButton *m_btnMoveDown;

protected Q_SLOTS:
    virtual void slotAdd() = 0;
    virtual void slotEdit() = 0;
    void updateButtons();
    void slotRemove();
    void slotMoveUp();
    void slotMoveDown();
};

#endif // WIDGETCONFIGURATIONTOOLSBASE_H
