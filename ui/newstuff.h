/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_NEWSTUFF_H_
#define _KPDF_NEWSTUFF_H_

#include <qdialog.h>
#include <knewstuff/provider.h>
using namespace KNS;
namespace KIO { class JOB; }

class AvailableItem;

class NewStuffDialog : public QDialog
{
    Q_OBJECT
    public:
        NewStuffDialog( QWidget * parent );
        ~NewStuffDialog();

        // show a message in the bottom bar
        enum MessageType { Normal, Info, Error };
        void displayMessage( const QString & msg, MessageType type = Normal,
                             int timeOutMs = 3000 );

    private:
        // private storage class
        class NewStuffDialogPrivate * d;

    private slots:
        void slotResetMessageColors();
        void slotNetworkTimeout();
        void slotSortingSelected( int sortType );
        // providers loading related
        void slotLoadProviders();
        void slotProvidersLoaded( Provider::List * list );
        // items loading related
        void slotLoadProvider( int provider = 0 );
        void slotProviderInfoData( KIO::Job *, const QByteArray & );
        void slotProviderInfoResult( KIO::Job * );
        // files downloading related
        //void slotDownloadItem( AvailableItem * );
        //void slotItemDownloaded( KIO::Job * );
};

#endif
