/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   Many portions of this file are based on kghostview's kpswidget code   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef _KPDF_GSINTERPETERLIB_H_
#define _KPDF_GSINTERPETERLIB_H_

#include <stdio.h>
#include <qimage.h>
#include <qstringlist.h>
#include <qmutex.h>

#include "interpreter.h"
#include "core/generator.h"

class GSInterpreterLib : public QObject
{
    Q_OBJECT
    public:
        GSInterpreterLib();
        ~GSInterpreterLib();
        // state functions
        bool startInterpreter(bool setStdio=true);
        bool stopInterpreter();
        bool busy () { return interpreterLock.locked() ;} ;
        bool ready () { return !interpreterLock.locked() && m_ready; } ;
        bool running () { return m_running; } ;
        // stdio handling
        int gs_input  ( char* buffer, int len );
        int gs_output ( const char* buffer, int len );
        int gs_error  ( const char* buffer, int len );

        bool run(FILE * tmp, PagePosition * pos, PixmapRequest *req,bool sync=true);
        void unlock() { interpreterLock.unlock(); };
        // setting properties 
        void setGhostscriptArguments( const QStringList &list );
        void setOrientation( int orientation );
        void setSize( int w, int h );
        void setMagnify( double magnify );
        void setMedia (QString media) ;
//         void setBoundingBox( const KDSCBBOX& boundingBox );

        // take pixmap
        QPixmap* takePixmap();

        // handling communication with libgs
        int size(int width, int height, int raster, unsigned int format, unsigned char *pimage);
        int page();

    signals:
    /**
     * This signal gets emited whenever a page is finished, but contains a reference to the pixmap
     * used to hold the image.
     *
     * Don't change the pixmap or bad things will happen. This is the backing pixmap of the display.
     */
        void newPageImage( PixmapRequest *req );

    /**
     * This signal is emitted whenever the ghostscript process has
     * written data to stdout or stderr.
     */
        void io ( MessageType t, const char* data, int len );

    private:

        bool handleExit(int code);
        // state bools
        bool m_running; // is any instance running
        bool m_sync; // expect sync
        bool m_syncDone; // sync finished
        bool m_ready;

        // additional info
        int m_orientation;
//         KDSCBBOX m_boundingBox;
        double m_magnify;
        int m_width;
        int m_height;
        QString m_media;
        int m_Gwidth;
        int m_Gheight;

        // data
        unsigned char * m_imageChar;
        unsigned int m_format;
        int m_raster;

        // returns
        PixmapRequest* m_req;
        QPixmap* m_pix;

        // arguments and argument functions
        int m_argsCCount;
        char** m_argsChar;
        QStringList m_args;
        QStringList m_internalArgs;
        void  argsToChar();

        // instance
        void * ghostScriptInstance;
        QMutex interpreterLock;
};
#endif
