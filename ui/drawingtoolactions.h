/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2015 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
