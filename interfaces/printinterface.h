/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_PRINTINTERFACE_H_
#define _OKULAR_PRINTINTERFACE_H_

#include "../core/okularcore_export.h"

#include <QObject>

class QWidget;

namespace Okular
{
/**
 * @short Interface for an extra print options widget in the print dialog.
 */
class OKULARCORE_EXPORT PrintOptionsWidgetInterface
{
public:
    PrintOptionsWidgetInterface() = default;

    virtual ~PrintOptionsWidgetInterface()
    {
    }

    PrintOptionsWidgetInterface(const PrintOptionsWidgetInterface &) = delete;
    PrintOptionsWidgetInterface &operator=(const PrintOptionsWidgetInterface &) = delete;

    virtual QWidget *widget() = 0;
    virtual bool ignorePrintMargins() const = 0;
};

/**
 * @short Abstract interface for advanced printing control
 *
 * This interface defines an advanced way of interfacing with the print
 * process.
 *
 * How to use it in a custom Generator:
 * @code
    class MyGenerator : public Okular::Generator, public Okular::PrintInterface
    {
        Q_OBJECT
        Q_INTERFACES( Okular::PrintInterface )

        ...
    };
 * @endcode
 * and - of course - implementing its methods.
 */
class OKULARCORE_EXPORT PrintInterface
{
public:
    PrintInterface()
    {
    }

    /**
     * Destroys the printer interface.
     */
    virtual ~PrintInterface()
    {
    }

    PrintInterface(const PrintInterface &) = delete;
    PrintInterface &operator=(const PrintInterface &) = delete;

    /**
     * Builds and returns a new printing configuration widget interface.
     *
     * @note don't keep a pointer to the returned interface or its widget,
     * as the widget will be handled elsewhere (in the Okular KPart)
     *
     */
    virtual PrintOptionsWidgetInterface *printConfigurationWidget() const = 0;
};

}

Q_DECLARE_INTERFACE(Okular::PrintInterface, "org.kde.okular.PrintInterface/0.1")

#endif
