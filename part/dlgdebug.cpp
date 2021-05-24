/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dlgdebug.h"

#include <QCheckBox>
#include <QLayout>

#define DEBUG_SIMPLE_BOOL(cfgname, layout)                                                                                                                                                                                                     \
    {                                                                                                                                                                                                                                          \
        QCheckBox *foo = new QCheckBox(QStringLiteral(cfgname), this);                                                                                                                                                                         \
        foo->setObjectName(QStringLiteral("kcfg_" cfgname));                                                                                                                                                                                   \
        layout->addWidget(foo);                                                                                                                                                                                                                \
    }

DlgDebug::DlgDebug(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    DEBUG_SIMPLE_BOOL("DebugDrawBoundaries", lay);
    DEBUG_SIMPLE_BOOL("DebugDrawAnnotationRect", lay);
    DEBUG_SIMPLE_BOOL("TocPageColumn", lay);

    lay->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));
}
