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
#include <kcmdlineargs.h>
#include <kdebug.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <fixx11h.h>

// #include <qdialog.h>
#include <QX11Info>

extern GC kde_xget_temp_gc( int scrn, bool monochrome );                // get temporary GC

GSInterpreterLib *interpreter;
int mem;
int anwser;
PageInfo pData;
FILE * f;

void PixHandler::slotPixmap(const QImage* img)
{
    QPixmap *pix=new QPixmap();
    kWarning() << "img size/depth " << img->size() << "/" <<img->depth() << endl;
    bool done=pix->convertFromImage(*img);
    kWarning () << "Conversion from qimage " << done << endl;
//         QDialog t;
//         t.resize(pix->width(),pix->height());
//         t.setBackgroundPixmap(*pix);
//         t.update();
//         t.exec();
    
              
    XCopyArea
        (QX11Info::display(),
        pix->handle(),
        pData.handle,
        kde_xget_temp_gc( pix->x11Info().screen(), false ),
        0,
        0,
        pix->width(),
        pix->height(),
        0,
        0);

    XSync(QX11Info::display(), false);
    int x=3;
    write (anwser,&x,sizeof(int));
}

void process()
{

    read ( mem, &pData, sizeof(pData) );
    if ( ! ( interpreter->running() ) )
        interpreter->start(false);

//     kDebug() << "Processing: " <<  pData.sync << endl;
    interpreter-> run ( f, pData.pos, pData.sync );
    if (! pData.sync )
    {
        int x=3;
        write (anwser,&x,sizeof(int));
    }
}

int main (int argc, char* argv[])
{
    KCmdLineArgs::init(argc, argv, "kpdflibgsasyncgenerator", "kpdflibgsasyncgenerator", 0, "0.1", KCmdLineArgs::CmdLineArgNone);
    KApplication app();
    // Order of argv: fileName, msgQueueId, media type, magnify, orientation 

    for (int i=0;i<argc;i++)
        kDebug() << "arg nr " << i << " : " <<  QString(argv[i]) << endl;

    f = fopen ( argv[1] , "r");
    interpreter=new GSInterpreterLib();
    interpreter->setMedia ( QString(argv[4]) );
    interpreter->setMagnify ( QString(argv[5]).toDouble() );
    interpreter->setOrientation ( QString(argv[6]).toInt() );
    interpreter->setSize ( QString(argv[7]).toInt(), QString(argv[8]).toInt() );
    interpreter->setPlatformFonts ( QString(argv[9]).toInt()  !=0 );
    interpreter->setAABits(QString(argv[10]).toInt(), QString(argv[11]).toInt() );
    interpreter->setProgressive(false);
    interpreter->start(false);
    
    PixHandler pxHandler;

    QObject::connect(interpreter,SIGNAL(Finished(const QImage* )),&pxHandler,SLOT(slotPixmap(const QImage* )));

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

