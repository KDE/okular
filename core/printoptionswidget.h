/*
    SPDX-FileCopyrightText: 2019 Michael Weghorn <m.weghorn@posteo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PRINTOPTIONSWIDGET_H
#define PRINTOPTIONSWIDGET_H

#include "interfaces/printinterface.h"
#include "okularcore_export.h"

class QComboBox;

namespace Okular
{
/**
 * @short The default okular extra print options widget.
 *
 * It just implements the required method 'ignorePrintMargins()' from
 * the base class 'PrintOptionsWidget'.
 */
class OKULARCORE_EXPORT DefaultPrintOptionsWidget : public PrintOptionsWidget
{
    Q_OBJECT

public:
    explicit DefaultPrintOptionsWidget(QWidget *parent = nullptr);

    bool ignorePrintMargins() const override;

private:
    QComboBox *m_ignorePrintMargins;
};

}

#endif
