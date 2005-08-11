/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "interpreter_lib.h"
#include "kpdflibgsasyncgenerator.h"

#include "gsapi/iapi.h"
#include "gsapi/gdevdsp.h"

#include <qpixmap.h>
#include <qimage.h>
#include <kapplication.h>
#include <kdebug.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <fixx11h.h>



GSInterpreterLib *interpreter;
int mem;
int anwser;
PageInfo pData;
FILE * f;

void PixHandler::slotPixmap(PixmapRequest *req)
{
    QPixmap *pix=interpreter->takePixmap();

    kdDebug() << "Handles: " << pix->handle() << " " << pData.handle <<endl;
    XCopyArea
        (qt_xdisplay(),
        pix->handle(),
        pData.handle,
        qt_xget_temp_gc( pix->x11Screen(), false ),
        0,
        0,
        pix->width(),
        pix->height(),
        0,
        0);

    XSync(qt_xdisplay(), false);
    int x=3;
    write (anwser,&x,sizeof(int));
}

void process()
{

    read ( mem, &pData, sizeof(pData) );
    if ( !interpreter->running() )
        interpreter->startInterpreter();

    kdDebug() << "Processing: " <<  pData.sync << endl;
    if (pData.sync)
    {
        kdDebug()  << "Request: " << pData.req.width << " " << pData.req.height << endl;
    }

    interpreter-> run ( f, &pData.pos, (pData.sync ? &pData.req : static_cast<PixmapRequest*>(0L) ), pData.sync );
    if (! pData.sync )
    {
        int x=3;
        write (anwser,&x,sizeof(int));
    }
}

int main (int argc, char* argv[])
{
    // Order of argv: fileName, msgQueueId, media type, magnify, orientation 

    for (int i=0;i<argc;i++)
        kdDebug() << "arg nr " << i << " : " <<  QCString(argv[i]) << endl;

    f = fopen ( argv[1] , "r");
    interpreter=new GSInterpreterLib();
    interpreter->setMedia ( QString(argv[4]) );
    interpreter->setMagnify ( QString(argv[5]).toDouble() );
    interpreter->setOrientation ( QString(argv[6]).toInt() );
    interpreter->setSize ( QString(argv[7]).toInt(), QString(argv[8]).toInt() );
    KApplication app(argc,argv,QCString("kpdflibgsasyncgenerator"));
    PixHandler pxHandler;
    QObject::connect(interpreter,SIGNAL(newPageImage(PixmapRequest*)),&pxHandler,SLOT(slotPixmap(PixmapRequest *)));

    int request;
    anwser = open( argv[3] , O_RDWR );
    mem = open( argv[2] , O_RDONLY );
    while( read ( mem, &request, sizeof(int) ) > 0 )
    {
        switch ( request )
        {
            // We are giubg to get a page
            case 0:
                process();
                break;
            case 1:
                delete interpreter;
                exit (0);
                break;
        }
    }
    return 0;
}

#include "kpdflibgsasyncgenerator.moc"

