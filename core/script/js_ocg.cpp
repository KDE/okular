/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_ocg_p.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QString>

using namespace Okular;

// OCG.state (getter)
bool JSOCG::state() const
{
    const QModelIndex index = m_model->index(m_i, m_j);

    return m_model->data(index, Qt::CheckStateRole).toBool();
}

// OCG.state (setter)
void JSOCG::setState(bool state)
{
    const QModelIndex index = m_model->index(m_i, m_j);

    m_model->setData(index, QVariant(state ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);
}

JSOCG::JSOCG(QAbstractItemModel *model, int i, int j, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_i(i)
    , m_j(j)
{
}

JSOCG::~JSOCG() = default;
