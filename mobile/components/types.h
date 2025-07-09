// SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <qqml.h>

#include "gui/certificatemodel.h"
#include "shell/recentitemsmodel.h"

class CertificateModelForeign : public QObject
{
    Q_OBJECT
    QML_FOREIGN(CertificateModel)
    QML_NAMED_ELEMENT(CertificateModel)
    QML_UNCREATABLE("")
};

class RecentItemsModelForeign : public QObject
{
    Q_OBJECT
    QML_FOREIGN(RecentItemsModel)
    QML_NAMED_ELEMENT(RecentItemsModel)
};
