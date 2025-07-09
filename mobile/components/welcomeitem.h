/*
    SPDX-FileCopyrightText: 2025 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <qqmlregistration.h>

#include <shell/recentitemsmodel.h>

class WelcomeItem : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(RecentItemsModel *recentItemsModel READ recentItemsModel CONSTANT)

public:
    explicit WelcomeItem(QObject *parent = nullptr);
    ~WelcomeItem() override;

    RecentItemsModel *recentItemsModel() const;

    Q_INVOKABLE void urlOpened(const QUrl &url);

private:
    RecentItemsModel *m_recentItemsModel;
};
