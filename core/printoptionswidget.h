/*
    SPDX-FileCopyrightText: 2019 Michael Weghorn <m.weghorn@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PRINTOPTIONSWIDGET_H
#define PRINTOPTIONSWIDGET_H

#include <QWidget>

#include "interfaces/printinterface.h"
#include "okularcore_export.h"

class QComboBox;

namespace Okular
{
/**
 * @short The default okular extra print options widget.
 *
 * It implements the required methods from 'PrintOptionsWidgetInterface'.
 */
class OKULARCORE_EXPORT DefaultPrintOptionsWidget : public QWidget, public PrintOptionsWidgetInterface
{
    Q_OBJECT

public:
    explicit DefaultPrintOptionsWidget(QWidget *parent = nullptr);

    QWidget *widget() override;
    bool ignorePrintMargins() const override;

private:
    QComboBox *m_ignorePrintMargins;
};

}

#endif
