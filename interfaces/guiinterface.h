/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GUIINTERFACE_H_
#define _OKULAR_GUIINTERFACE_H_

#include <okular/core/okular_export.h>

#include <QtCore/QObject>

#include <kxmlguiclient.h>

namespace Okular {

/**
 * @short Abstract interface for user interface control
 *
 * This interface defines an way to interact with the Okular user interface,
 * e.g. adding actions in the menus.
 *
 * How to use it in a custom Generator:
 * @code
    class MyGenerator : public Okular::Generator, public Okular::GuiInterface
    {
        Q_OBJECT
        Q_INTERFACES( Okular::GuiInterface )

        ...
    };
 * @endcode
 * and - of course - implementing its methods.
 */
class OKULAR_EXPORT GuiInterface : protected KXMLGUIClient
{
    public:
        /**
         * Destroys the gui interface.
         */
        virtual ~GuiInterface() {}

        /**
         * This method requests the XML GUI Client provided by the interface.
         */
        KXMLGUIClient* guiClient() { return this; }
};

}

Q_DECLARE_INTERFACE( Okular::GuiInterface, "org.kde.okular.GuiInterface/0.1" )

#endif
