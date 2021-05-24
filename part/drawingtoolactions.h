/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2015 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DRAWINGTOOLACTIONS_H
#define DRAWINGTOOLACTIONS_H

#include <QDomDocument>
#include <QObject>

class QAction;
class KActionCollection;

class DrawingToolActions : public QObject
{
    Q_OBJECT
public:
    explicit DrawingToolActions(KActionCollection *parent);
    ~DrawingToolActions() override;

    QList<QAction *> actions() const;

    void reparseConfig();

Q_SIGNALS:
    void changeEngine(const QDomElement &doc);
    void actionsRecreated();

private Q_SLOTS:
    void actionTriggered();

private:
    void loadTools();
    void createToolAction(const QString &text, const QString &toolName, const QString &colorName, const QDomElement &root);

    QList<QAction *> m_actions;
};

Q_DECLARE_METATYPE(QDomElement)

#endif // DRAWINGTOOLACTIONS_H
