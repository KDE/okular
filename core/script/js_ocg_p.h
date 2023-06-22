/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_OCG_P_H
#define OKULAR_SCRIPT_JS_OCG_P_H

#include <QObject>

class QAbstractItemModel;

namespace Okular
{
class JSOCG : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool state READ state WRITE setState) // clazy:exclude=qproperty-without-notify

public:
    explicit JSOCG(QAbstractItemModel *model, int i, int j, QObject *parent = nullptr);
    ~JSOCG() override;

    bool state() const;
    void setState(bool state);

private:
    QAbstractItemModel *m_model = nullptr;
    int m_i;
    int m_j;
};

}

#endif
