/***************************************************************************
 *   Copyright (C) 1997-2005 the KGhostView authors. See file GV_AUTHORS.  *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   Many portions of this file are based on kghostview's kpswidget code   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include <kdebug.h>
#include <qpixmap.h>

#include "interpreter.h"
#include "gs/iapi.h"
#include "gs/ierrors.h"

int intPow(int x, int n)
{
    int z;
    if (n == 0)
        return 1;
    else if (n == 1)
        return x;
    else if ((n%2)==0)
    {
        // x^2n=x^n * x^n;
        z=intPow(x,n/2);
        return z*z;
    }
    else
    {
        // x^(2n+1)=x^2n * x;
        z=intPow(x,(n-1)/2);
        return z*z*x;
    }
}


QImage* generatedImage=0;

int size(void *handle, void *device, int width, int height,
        int raster, unsigned int format, unsigned char *pimage)
{
    int depth = format & DISPLAY_DEPTH_MASK;
    int color = format & DISPLAY_COLORS_MASK;
    bool lEndian = (format & DISPLAY_ENDIAN_MASK) == DISPLAY_LITTLEENDIAN ;
    int dep= QPixmap::defaultDepth () ;
    int numCol = (color == DISPLAY_COLORS_GRAY ) ? 1 : intPow( 2, dep ) ;
    switch (depth)
    {
        case DISPLAY_DEPTH_1:
            numCol=2;
            dep=1;
        case DISPLAY_DEPTH_2:
            dep=2;
            numCol=4;
            break;
        case DISPLAY_DEPTH_4:
            dep=4;
            numCol=16;
            break;
        case DISPLAY_DEPTH_8:
            dep=8;
            numCol=256;
            break;
        case DISPLAY_DEPTH_12:
            dep=12;
            numCol=4096;
            break;
        case DISPLAY_DEPTH_16:
            dep=16;
            numCol=65536;
            break;
    }
    kdDebug() << "size called" << endl;
    generatedImage = new QImage (pimage, width, height, 
        dep, (QRgb*) 0, numCol,  (lEndian) ? QImage::LittleEndian : QImage::BigEndian );
    return 0;
}

/*
    device.size = sizeof ( display_callback );
    device.version_major = DISPLAY_VERSION_MAJOR;
    device.version_minor = DISPLAY_VERSION_MINOR;
    device.display_open = NULL;
    device.display_preclose = NULL;
    device.display_close = NULL;
    device.display_presize = NULL;
    device.display_size = & size;
    device.display_sync = NULL;
    device.display_page = NULL;
    device.display_update = NULL;
    device.display_memalloc = NULL;
    device.display_memfree = NULL; */

    display_callback device=
    {
        sizeof ( display_callback ),
        DISPLAY_VERSION_MAJOR,
        DISPLAY_VERSION_MINOR,
        NULL,
        NULL,
        NULL,
        NULL,
        & size,
        NULL,
        NULL,
        NULL,
        NULL,       /* memalloc */
        NULL
#if DISPLAY_VERSION_MAJOR  >= 2
        ,NULL
#endif
    };

GVInterpreter::GVInterpreter(QStringList *list)
{
    gsArgs=list;
    int exit=gsapi_new_instance(&ghostScriptInstance,NULL);
    if (exit != 0)
    {
        kdDebug() << "sth is very wrong! " << endl;
    }
    int exit_i=gsapi_init_with_args (&ghostScriptInstance,gsArgs->count(),argsToChar());
    if (exit_i != 0)
    {
        kdDebug() << "screwed sth with args?! " << endl;
    }
}

GVInterpreter::~GVInterpreter()
{
    gsapi_exit(&ghostScriptInstance);
    gsapi_delete_instance(&ghostScriptInstance);
    delete gsArgs;
}

bool GVInterpreter::run(FILE * tmp, long begin , unsigned int lenght)
{
    if (fseek(tmp,begin, SEEK_SET))
        return false;

    char * buf = new char [4096];
    int read=4096;
    int exit_code, left=lenght;
    bool errorExit=false;

    interpreterLock.lock();
    gsapi_set_display_callback(&ghostScriptInstance, &device);
    gsapi_run_string_begin (&ghostScriptInstance, 0, &exit_code);

    if (exit_code)
        errorExit=handleExit(exit_code);

    if (errorExit)
        return false;

    while (lenght)
    {
        read=fread (buf,sizeof(char),read,tmp);
        gsapi_run_string_continue (&ghostScriptInstance, buf, read, 0, &exit_code);;

        if (exit_code)
            errorExit=handleExit(exit_code);

        if (errorExit)
            return false;

        left-=read;
    }


    gsapi_run_string_end (&ghostScriptInstance, 0, &exit_code);

    if (exit_code)
        errorExit=handleExit(exit_code);

    if (errorExit)
        return false;

    gsapi_set_display_callback(&ghostScriptInstance, NULL);
    interpreterLock.unlock();
    m_img=generatedImage;
    return true;
}

bool GVInterpreter::handleExit(int code)
{
    switch (code)
    {
        // quit was issued
        case e_Quit:
        // gs -h was run
        case e_Info:
            //gsapi_exit
            return false;

        // no error or we need further input
        case 0:
        case e_NeedInput:
            return true;

        // <0 - error
        default:
            return false;
    }
}

char** GVInterpreter::argsToChar()
{
    char ** ret;
    int i,size=gsArgs->count()+1;
    ret = static_cast <char **> (new char* [size]);
    for (i=0;i<size;i++)
    {
        ret[i+1]=const_cast <char*> ((*gsArgs)[i].ascii());
        kdDebug() << "Arg nr " << i << " : " << (*gsArgs)[i].ascii() << endl ;
    }
    //ret[size+1]=0;
    return ret;
}


/*
bool GVInterpreter::x11Event( XEvent* e )
{
    if( e->type == ClientMessage )
    {
        _gsWindow = e->xclient.data.l[0];

        if( e->xclient.message_type == _atoms[PAGE] )
        {
            kdDebug(4500) << "KPSWidget: received PAGE" << endl;
            interpreterLock.unLock();
            unsetCursor();
            emit newPageImage( _backgroundPixmap );
            if ( _doubleBuffer ) 
                setErasePixmap( _backgroundPixmap );
            return true;
        }
        else if( e->xclient.message_type == _atoms[DONE] )
        {
            kdDebug(4500) << "KPSWidget: received DONE" << endl;
            stopInterpreter();
            return true;
        }
	}
    
    return QWidget::x11Event( e );
}*/

