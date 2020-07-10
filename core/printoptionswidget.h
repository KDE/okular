/***************************************************************************
 *   Copyright (C) 2019 Michael Weghorn <m.weghorn@posteo.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PRINTOPTIONSWIDGET_H
#define PRINTOPTIONSWIDGET_H

#include <QWidget>

#include "okularcore_export.h"

class QComboBox;

namespace Okular
{
/**
 * @short Abstract base class for an extra print options widget in the print dialog.
 */
class OKULARCORE_EXPORT PrintOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PrintOptionsWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }
    virtual bool ignorePrintMargins() const = 0;
};

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
