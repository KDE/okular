/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef EDITDRAWINGTOOLDIALOGTEST_H
#define EDITDRAWINGTOOLDIALOGTEST_H

#include <QObject>

class EditDrawingToolDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit EditDrawingToolDialogTest(QObject *parent = nullptr);
    ~EditDrawingToolDialogTest();

private Q_SLOTS:
    void shouldHaveDefaultValues();
};

#endif // EDITDRAWINGTOOLDIALOGTEST_H
