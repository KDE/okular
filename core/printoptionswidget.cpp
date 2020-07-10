/***************************************************************************
 *   Copyright (C) 2019 Michael Weghorn <m.weghorn@posteo.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "printoptionswidget.h"

#include <QComboBox>
#include <QFormLayout>

#include <KLocalizedString>

namespace Okular
{
DefaultPrintOptionsWidget::DefaultPrintOptionsWidget(QWidget *parent)
    : PrintOptionsWidget(parent)
{
    setWindowTitle(i18n("Print Options"));
    QFormLayout *layout = new QFormLayout(this);
    m_ignorePrintMargins = new QComboBox;
    // value indicates whether full page is enabled (i.e. print margins ignored)
    m_ignorePrintMargins->insertItem(0, i18n("Fit to printable area"), false);
    m_ignorePrintMargins->insertItem(1, i18n("Fit to full page"), true);
    layout->addRow(i18n("Scale mode:"), m_ignorePrintMargins);
}

bool DefaultPrintOptionsWidget::ignorePrintMargins() const
{
    return m_ignorePrintMargins->currentData().value<bool>();
}

}
