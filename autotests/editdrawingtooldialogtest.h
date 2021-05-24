/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef EDITDRAWINGTOOLDIALOGTEST_H
#define EDITDRAWINGTOOLDIALOGTEST_H

#include <QObject>

class EditDrawingToolDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit EditDrawingToolDialogTest(QObject *parent = nullptr);
    ~EditDrawingToolDialogTest() override;

private Q_SLOTS:
    void shouldHaveDefaultValues();
};

#endif // EDITDRAWINGTOOLDIALOGTEST_H
