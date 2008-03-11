/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_SAVEINTERFACE_H_
#define _OKULAR_SAVEINTERFACE_H_

#include <okular/core/okular_export.h>

#include <QtCore/QObject>

namespace Okular {

class AnnotationProxy;

/**
 * @short Abstract interface for saving
 *
 * This interface defines a way to save (or help saving) the document opened
 * by the Generator.
 *
 * How to use it in a custom Generator:
 * @code
    class MyGenerator : public Okular::Generator, public Okular::SaveInterface
    {
        Q_OBJECT
        Q_INTERFACES( Okular::SaveInterface )

        ...
    };
 * @endcode
 * and - of course - implementing its methods.
 */
class OKULAR_EXPORT SaveInterface
{
    public:
        /**
         * The possible options for the saving.
         */
        enum SaveOption
        {
            NoOption = 0,
            SaveChanges = 1        ///< The possibility to save with the current changes to the document.
        };
        Q_DECLARE_FLAGS( SaveOptions, SaveOption )

        /**
         * Destroys the save interface.
         */
        virtual ~SaveInterface() {}

        /**
         * Query for the supported saving options.
         *
         * @note NoOption is never queried
         */
        virtual bool supportsOption( SaveOption option ) const = 0;

        /**
         * Save to the specified @p fileName with the specified @p options.
         */
        virtual bool save( const QString &fileName, SaveOptions options ) = 0;
};

}

Q_DECLARE_INTERFACE( Okular::SaveInterface, "org.kde.okular.SaveInterface/0.1" )
Q_DECLARE_OPERATORS_FOR_FLAGS( Okular::SaveInterface::SaveOptions )

#endif
