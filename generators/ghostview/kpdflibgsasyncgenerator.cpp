/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include "kpdflibgsasyncgenerator.h"

#include <qgs.h>

#include <qpainter.h>
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

void PixHandler::slotPixmap(const QImage* img)
{
    QPixmap *pix=new QPixmap(img->size(), img->depth());
    kdDebug() << "Handles: " << pix->handle() << " " << pData.handle <<endl;
    QPainter p;
    p.begin(pix);
    p.drawImage(0,0,*img, 0,0,img->width(),img->height());
    p.end();
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
        interpreter->start(false);

//     kdDebug() << "Processing: " <<  pData.sync << endl;
    interpreter-> run ( f, pData.pos, pData.sync );
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
    interpreter->setPlatformFonts ( QString(argv[9]).toInt()  !=0 );
    interpreter->setAABits(QString(argv[10]).toInt(), QString(argv[11]).toInt() );
    KApplication app(argc,argv,QCString("kpdflibgsasyncgenerator"));
    PixHandler pxHandler;

    QObject::connect(interpreter,SIGNAL(Finished(const QImage* img)),&pxHandler,SLOT(slotPixmap(const QImage* img)));

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

