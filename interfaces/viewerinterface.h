/***************************************************************************
 *   Copyright (C) 2011 by Michel Ludwig <michel.ludwig@kdemail.net>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_VIEWERINTERFACE_H_
#define _OKULAR_VIEWERINTERFACE_H_

#include "../core/okular_export.h"

#include <QtCore/QObject>
#include <QString>

namespace Okular {

/**
 * @short Abstract interface for controlling advanced features of a document viewer
 *
 * This interface can be used to control some more or less advanced features of a document
 * viewer.
 */
class OKULAR_EXPORT ViewerInterface
{
    public:
        virtual ~ViewerInterface() {}

        /**
         * Show the specified source location centrally in the viewer.
         */
        virtual void showSourceLocation(const QString& fileName, int line, int column) = 0;


        /**
         * Allows to enable or disable the watch file mode
         */
        virtual void setWatchFileModeEnabled(bool b) = 0;


        // SIGNALS
        /**
         * The signal 'openSourceReference' is emitted whenever the user has triggered a source
         * reference in the currently displayed document.
         */
        void openSourceReference(const QString& absFileName, int line, int column);
};

}

Q_DECLARE_INTERFACE( Okular::ViewerInterface, "org.kde.okular.ViewerInterface/0.1" )

#endif
