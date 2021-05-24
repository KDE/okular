/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "widgetconfigurationtoolsbase.h"

#include <KLocalizedString>
#include <QIcon>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

WidgetConfigurationToolsBase::WidgetConfigurationToolsBase(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(32, 32));
    hBoxLayout->addWidget(m_list);

    QVBoxLayout *vBoxLayout = new QVBoxLayout();
    m_btnAdd = new QPushButton(i18n("&Add..."), this);
    m_btnAdd->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    vBoxLayout->addWidget(m_btnAdd);
    m_btnEdit = new QPushButton(i18n("&Edit..."), this);
    m_btnEdit->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    m_btnEdit->setEnabled(false);
    vBoxLayout->addWidget(m_btnEdit);
    m_btnRemove = new QPushButton(i18n("&Remove"), this);
    m_btnRemove->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    m_btnRemove->setEnabled(false);
    vBoxLayout->addWidget(m_btnRemove);
    m_btnMoveUp = new QPushButton(i18n("Move &Up"), this);
    m_btnMoveUp->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    m_btnMoveUp->setEnabled(false);
    vBoxLayout->addWidget(m_btnMoveUp);
    m_btnMoveDown = new QPushButton(i18n("Move &Down"), this);
    m_btnMoveDown->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    m_btnMoveDown->setEnabled(false);
    vBoxLayout->addWidget(m_btnMoveDown);
    vBoxLayout->addStretch();
    hBoxLayout->addLayout(vBoxLayout);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &WidgetConfigurationToolsBase::slotEdit);
    connect(m_list, &QListWidget::currentRowChanged, this, &WidgetConfigurationToolsBase::updateButtons);
    connect(m_btnAdd, &QPushButton::clicked, this, &WidgetConfigurationToolsBase::slotAdd);
    connect(m_btnEdit, &QPushButton::clicked, this, &WidgetConfigurationToolsBase::slotEdit);
    connect(m_btnRemove, &QPushButton::clicked, this, &WidgetConfigurationToolsBase::slotRemove);
    connect(m_btnMoveUp, &QPushButton::clicked, this, &WidgetConfigurationToolsBase::slotMoveUp);
    connect(m_btnMoveDown, &QPushButton::clicked, this, &WidgetConfigurationToolsBase::slotMoveDown);
}

WidgetConfigurationToolsBase::~WidgetConfigurationToolsBase()
{
}

void WidgetConfigurationToolsBase::updateButtons()
{
    const int row = m_list->currentRow();
    const int last = m_list->count() - 1;

    m_btnEdit->setEnabled(row != -1);
    m_btnRemove->setEnabled(row != -1);
    m_btnMoveUp->setEnabled(row > 0);
    m_btnMoveDown->setEnabled(row != -1 && row != last);
}

void WidgetConfigurationToolsBase::slotRemove()
{
    const int row = m_list->currentRow();
    delete m_list->takeItem(row);
    updateButtons();
    emit changed();
}

void WidgetConfigurationToolsBase::slotMoveUp()
{
    const int row = m_list->currentRow();
    m_list->insertItem(row, m_list->takeItem(row - 1));
    m_list->scrollToItem(m_list->currentItem());
    updateButtons();
    emit changed();
}

void WidgetConfigurationToolsBase::slotMoveDown()
{
    const int row = m_list->currentRow();
    m_list->insertItem(row, m_list->takeItem(row + 1));
    m_list->scrollToItem(m_list->currentItem());
    updateButtons();
    emit changed();
}
