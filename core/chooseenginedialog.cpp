/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "chooseenginedialog_p.h"

#include <QComboBox>
#include <QLabel>

#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui_chooseenginewidget.h"

ChooseEngineDialog::ChooseEngineDialog(const QStringList &generators, const QMimeType &mime, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Backend Selection"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return); // NOLINT(bugprone-suspicious-enum-usage)
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ChooseEngineDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ChooseEngineDialog::reject);
    okButton->setDefault(true);
    QWidget *main = new QWidget(this);
    m_widget = new Ui_ChooseEngineWidget();
    m_widget->setupUi(main);
    mainLayout->addWidget(main);
    mainLayout->addWidget(buttonBox);
    m_widget->engineList->addItems(generators);

    m_widget->description->setText(
        i18n("<qt>More than one backend found for the MIME type:<br />"
             "<b>%1</b> (%2).<br /><br />"
             "Please select which one to use:</qt>",
             mime.comment(),
             mime.name()));
}

ChooseEngineDialog::~ChooseEngineDialog()
{
    delete m_widget;
}

int ChooseEngineDialog::selectedGenerator() const
{
    return m_widget->engineList->currentIndex();
}
