/*
    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WELCOMESCREEN_H
#define WELCOMESCREEN_H

#include "shell/ui_welcomescreen.h"

#include <QFrame>
#include <QUrl>

class KRecentFilesAction;
class QListWidgetItem;
class RecentItemsModel;
class RecentsListItemDelegate;

class WelcomeScreen : public QWidget, Ui::WelcomeScreen
{
    Q_OBJECT
public:
    explicit WelcomeScreen(QWidget *parent = nullptr);
    ~WelcomeScreen() override;

    void loadRecents();

Q_SIGNALS:
    void openClicked();
    void closeClicked();
    void recentItemClicked(QUrl const &url);
    void forgetAllRecents();
    void forgetRecentItem(QUrl const &url);

protected:
    void showEvent(QShowEvent *e) override;

private Q_SLOTS:
    void recentsItemActivated(QModelIndex const &index);
    void recentListChanged();

private:
    int recentsCount();

    RecentItemsModel *m_recentsModel;
    RecentsListItemDelegate *m_recentsItemDelegate;

    QLabel *m_noRecentsLabel;
};

#endif // WELCOMESCREEN_H
