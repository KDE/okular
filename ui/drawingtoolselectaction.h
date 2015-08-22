/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef DRAWINGTOOLSELECTACTION_H
#define DRAWINGTOOLSELECTACTION_H

#include <QDomDocument>
#include <QWidgetAction>

class QAbstractButton;
class QButtonGroup;
class QHBoxLayout;

class DrawingToolSelectAction : public QWidgetAction
{
    Q_OBJECT
public:
    explicit DrawingToolSelectAction( QObject *parent = Q_NULLPTR );
    ~DrawingToolSelectAction();

signals:
    void changeEngine( const QDomElement &doc );

private slots:
    void toolButtonClicked();

private:
    void loadTools();
    void createToolButton( const QString &description, const QString &colorName, const QDomElement &root, const QString &shortcut = QString() );

    QList<QAbstractButton*> m_buttons;
    QHBoxLayout *m_layout;
};

Q_DECLARE_METATYPE( QDomElement )

#endif // DRAWINGTOOLSELECTACTION_H
