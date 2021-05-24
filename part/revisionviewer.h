/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_REVISIONVIEWER_H
#define OKULAR_REVISIONVIEWER_H

#include <QByteArray>
#include <QObject>

class QWidget;

class RevisionViewer : public QObject
{
    Q_OBJECT

public:
    explicit RevisionViewer(const QByteArray &revisionData, QWidget *parent = nullptr);

public Q_SLOTS:
    void viewRevision();

private:
    QWidget *m_parent;
    QByteArray m_revisionData;
};

#endif
