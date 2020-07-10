/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "editdrawingtooldialogtest.h"
#include "../editdrawingtooldialog.h"

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
