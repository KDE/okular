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

class QToolBox;
class KActionCollection;

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
class OKULAR_EXPORT GuiInterface
{
    public:
        /**
         * Destroys the gui interface.
         */
        virtual ~GuiInterface() {}

        /**
         * Returns the name of the gui description file that shall
         * be merged with the Okular menu.
         */
        virtual QString xmlFile() const = 0;

        /**
         * This method is called when the Okular gui is set up.
         *
         * You can insert the action which are listed in the file returned by
         * @p xmlFile() into the given @p collection to make them appear in the
         * menu bar.
         *
         * The @p toolbox pointer allows you to add new custom widgets to Okulars left
         * side pane.
         */
        virtual void setupGui( KActionCollection *collection, QToolBox *toolbox ) = 0;

        /**
         * This method is called when the Okular gui is cleaned up.
         * You should free all the gui elements created in @p setupGui() here.
         */
        virtual void freeGui() = 0;
};

}

Q_DECLARE_INTERFACE( Okular::GuiInterface, "org.kde.okular.GuiInterface/0.1" )

#endif
