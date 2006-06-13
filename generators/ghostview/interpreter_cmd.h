/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_GSINTERPETERCMD_H_
#define _KPDF_GSINTERPETERCMD_H_

#include <sys/types.h>

#include <qmutex.h>
#include <qthread.h>
#include <qmap.h>

#include <kdebug.h>

#include "core/generator.h"

#include "okulargsasyncgenerator.h"
#include "internaldocument.h"

#define GS_DATAREADY_ID 6989

class QCustomEvent;
class QString;
class QStringList;
class KProcess;

struct ProcessData
{
    ProcessData ();
    ~ProcessData();
    QString names[2];
    int fds[2];
};

class GSInterpreterCMD : public QThread
{
    Q_OBJECT
    public:
        GSInterpreterCMD( const QString & fileName);
        ~GSInterpreterCMD();
        QPixmap* takePixmap();
        bool startInterpreter();
        bool stop(bool async=true);
        bool ready() { return !interpreterLock.locked() ; } ;
        bool interpreterRunning();
        void lock() { kDebug() << "locking async\n"; interpreterLock.lock() ; } ;
        void unlock() { kDebug() << "unlocking async\n"; interpreterLock.unlock() ; } ;

//         void setGhostscriptArguments( const QStringList& arguments );
        void setOrientation( int orientation );
        void setSize( int w, int h );
        void setPlatformFonts(bool pfonts=true);
        void setAABits(int text=1, int graphics=1);
        void setMagnify( double magnify );
        void setMedia (QString media) ;
//         void setBoundingBox( const KDSCBBOX& boundingBox );
        void setStructure(GSInterpreterLib::Position prolog, GSInterpreterLib::Position setup);
        bool run( GSInterpreterLib::Position pos );
//        void customEvent( QEvent * e );

    private slots:
        void threadFinished();

    signals:
        /**
         * This signal gets emited whenever a page is finished, but contains a reference to the pixmap
         * used to hold the image.
         *
         * Don't change the pixmap or bad things will happen. This is the backing pixmap of the display.
        */
        void Finished( QPixmap *);
        void error (const QString&, int duration);

    private:
        void run();
        void destroyInternalProcess(KProcess * stop);
        // communication stuff

        PageInfo m_info;
        PixmapRequest *m_req;
        ProcessData * m_processData;

        // stopping list
        QMap<pid_t,ProcessData*> m_stoppingPids;

        // result
        QPixmap* m_pixmap;
        QMutex interpreterLock;
        // process stuff
        KProcess *m_process;
        QString m_error;
        // FILE INFORMATION:
        // hold pointer to a file never delete it, it should 
        // change everytime new request is done
        bool m_structurePending;
        double m_magnify;
        int m_aaText,m_aaGfx;
        bool m_pfonts;
        // prolog/setup positions
        GSInterpreterLib::Position m_data[2];
        bool m_haveStructure;
        // we have to send structure info
        int m_orientation;
        int m_width,m_height;
        QString m_name;
        QString m_media;
};

#endif
