/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
