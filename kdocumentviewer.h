/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KDOCUMENTVIEWER_H_
#define _KDOCUMENTVIEWER_H_

#include <QtCore/QStringList>

class KUrl;

/**
 * @short Abstract interface for a document viewer
 *
 * This interface describe briefly the basic functions of a document viewer.
 */
class KDocumentViewer
{
    public:
        virtual ~KDocumentViewer() {}

        /**
         * Open the document at the specified @p url at page @p page.
         */
        virtual bool openDocument( const KUrl& url, uint page ) = 0;

        /**
         * Change to @p page the currently shown page.
         */
        virtual void goToPage( uint page ) = 0;

        /**
         * Start the presentation mode.
         */
        virtual void startPresentation() = 0;

        /**
         * Return a list with the supported mimetypes.
         */
        virtual QStringList supportedMimeTypes() const = 0;
};

Q_DECLARE_INTERFACE( KDocumentViewer, "org.kde.kdocumentviewer/0.1" )

#endif
