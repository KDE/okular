/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "editdrawingtooldialogtest.h"
#include "../part/editdrawingtooldialog.h"

#include <KColorButton>
#include <KLineEdit>

#include <QDialogButtonBox>
#include <QSpinBox>
#include <QTest>

EditDrawingToolDialogTest::EditDrawingToolDialogTest(QObject *parent)
    : QObject(parent)
{
}

EditDrawingToolDialogTest::~EditDrawingToolDialogTest()
{
}

void EditDrawingToolDialogTest::shouldHaveDefaultValues()
{
    EditDrawingToolDialog dlg;

    const QDialogButtonBox *buttonBox = dlg.findChild<QDialogButtonBox *>(QStringLiteral("buttonbox"));
    QVERIFY(buttonBox);

    const KLineEdit *name = dlg.findChild<KLineEdit *>(QStringLiteral("name"));
    QVERIFY(name);

    const KColorButton *colorButton = dlg.findChild<KColorButton *>(QStringLiteral("colorbutton"));
    QVERIFY(colorButton);

    const QSpinBox *opacity = dlg.findChild<QSpinBox *>(QStringLiteral("opacity"));
    QVERIFY(opacity);

    QVERIFY(name->text().isEmpty());
}

QTEST_MAIN(EditDrawingToolDialogTest)
