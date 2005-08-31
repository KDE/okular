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

#include <math.h>

#include <qpixmap.h>
#include <qpainter.h>
#include <qstring.h>

#include <kdebug.h>


#include "gsapi/iapi.h"
#include "gsapi/ierrors.h"
#include "gsapi/gdevdsp.h"

#include "interpreter_lib.h"
#include "conf/gssettings.h"

// C API wrappers
int handleStdin(void *caller_handle, char *buf, int len)
{
    return static_cast <GSInterpreterLib*>(caller_handle) -> gs_input(buf,len);
}

int handleStdout(void *caller_handle, const char *str, int len)
{
    return static_cast <GSInterpreterLib*>(caller_handle) -> gs_output(str,len);
}

int handleStderr(void *caller_handle, const char *str, int len)
{
    return static_cast <GSInterpreterLib*>(caller_handle) -> gs_error(str,len);
}

int open(void  */*handle */, void  */*device */ )
{
    kdDebug(4655) << "open called" << endl;
    return 0;
}

int preclose(void * /* handle */, void * /* device */ )
{
    kdDebug(4655) << "preclose called" << endl;
    return 0;
}

int dclose(void * /* handle */, void * /* device */ )
{
    kdDebug(4655) << "close called" << endl;
    return 0;
}


int presize(void * /* handle*/, void * /* device*/, int /* width*/, int /* height*/,
        int /* raster*/, unsigned int /* format */)
{
    kdDebug(4655) << "presize called" << endl;
    return 0;
}


int size(void *handle, void * /* device*/, int width, int height,
        int raster, unsigned int format, unsigned char *pimage)
{
    return static_cast <GSInterpreterLib*>(handle) -> size (width, height,
        raster, format, pimage);
}

int sync(void * /* handle */, void * /* device */ )
{
    kdDebug(4655) << "sync called" << endl;
    return 0;
}


int page(void *handle, void * /* device*/, int /* copies*/, int /* flush*/)
{
    return static_cast <GSInterpreterLib*>(handle) -> page ();
}

int update(void * /* handle*/, void * /* device*/,
    int /* x*/, int /* y*/, int /* w*/, int /* h*/)
{
//        kdDebug(4655) << "update called" << endl;
    return 0;
}

int separation(void * /* handle*/, void * /* device*/,
    int /* comp_num*/, const char * /* name*/,
    unsigned short /* c*/, unsigned short /* m*/,
    unsigned short /* y*/, unsigned short /* k*/)
{
    kdDebug(4655) << "separation called" << endl;
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

    display_callback device =
    {
        sizeof ( display_callback ),
        DISPLAY_VERSION_MAJOR,
        DISPLAY_VERSION_MINOR,
        &open,
        &preclose,
        &dclose,
        &presize,
        &size,
        &sync,
        &page,
        &update,
        NULL,       /* memalloc */
        NULL
#if DISPLAY_VERSION_MAJOR  >= 2
        ,&separation
#endif
    };

GSInterpreterLib::GSInterpreterLib() : m_running(false), m_argsCCount(0), m_argsChar(0)
{
    int exit = gsapi_new_instance(&ghostScriptInstance,this);
    kdDebug(4655) << "Setting image" << endl;
    m_ready=true;
    m_pix=0;
    Q_ASSERT (exit == 0);
}


// interpreter state functions 

bool GSInterpreterLib::startInterpreter(bool setStdio)
{
    m_sync=false;
    kdDebug(4655) << "setting m_sync to " << m_sync << " in startInterpreter " <<endl;
    if ( setStdio )
        gsapi_set_stdio (ghostScriptInstance,&handleStdin,&handleStdout,&handleStderr);
    kdDebug(4655) << "setting display " << endl;
    int call = gsapi_set_display_callback(ghostScriptInstance, &device);
    argsToChar ();
    kdDebug(4655) << "setting args " << endl;
    int exit_i=gsapi_init_with_args (ghostScriptInstance,m_argsCCount,m_argsChar);


    m_running = handleExit ( exit_i ) ;
/*    if ( m_running && setMore )
    {
        QString set;
        // is this needed??
        set.sprintf("<< /Orientation %d /ImagingBBox [ %d %d %d %d ] >> setpagedevice .locksafe",
        m_orientation,m_boundingBox.llx(),
        m_boundingBox.lly(),m_boundingBox.urx(),m_boundingBox.ury());
        kdDebug(4655) << set << endl;
        gsapi_run_string_with_length (ghostScriptInstance,set.latin1(),set.length(),0,&exit_i);
        set="currentpagedevice {exch ==only ( ) print ==} forall";
        gsapi_run_string_with_length (ghostScriptInstance,set.latin1(),set.length(),0,&exit_i);
        m_running = handleExit ( exit_i );
        kdDebug(4655) <<  "After setting : "  << m_running << endl;
    }*/
    return m_running;
}

bool GSInterpreterLib::stopInterpreter()
{
    if (m_running)
    {
        gsapi_exit(ghostScriptInstance);
        m_running=false;
        m_sync=false;
        kdDebug(4655) << "setting m_sync to " << m_sync << " in stopInterpreter " <<endl;
    }
    return m_running;
}


// set options
void GSInterpreterLib::setGhostscriptArguments( const QStringList &list )
{
    interpreterLock.lock();
    if ( m_args != list )
    {
        m_args=list;
        stopInterpreter();
    }
    interpreterLock.unlock();
}

void GSInterpreterLib::setOrientation( int orientation )
{
    interpreterLock.lock();
    if( m_orientation != orientation )
    {
        m_orientation = orientation;
        stopInterpreter();
    }
    interpreterLock.unlock();
}

void GSInterpreterLib::setMagnify( double magnify )
{
    interpreterLock.lock();
    if( m_magnify != magnify )
    {
        m_magnify = magnify;
        stopInterpreter();
    }
    interpreterLock.unlock();
}

void GSInterpreterLib::setMedia( QString media )
{
    interpreterLock.lock();
    if( m_media != media )
    {
        m_media = media;
        stopInterpreter();
    }
    interpreterLock.unlock();
}

void GSInterpreterLib::setSize( int w, int h )
{

    if ( m_width != w ) 
    {
        m_width=w;
        stopInterpreter();

    }
    if ( m_height != h )
    {
        m_height=h;
        stopInterpreter();
    }

}

/*void GSInterpreterLib::setBoundingBox( const KDSCBBOX& boundingBox )
{
    interpreterLock.lock();
    if( m_boundingBox != boundingBox )
    {
        m_boundingBox = boundingBox;
        stopInterpreter();
    }
    interpreterLock.unlock();
}*/


bool GSInterpreterLib::run(FILE * tmp, PagePosition *pos, PixmapRequest *req, bool sync)
{
    if (fseek(tmp,pos->first, SEEK_SET))
        return false;

    m_syncDone=false;
    m_sync=sync;
    kdDebug(4655) << "setting m_sync to " << m_sync << " in run" <<endl;
    m_req=req;
    char buf [4096];
    int read, code, exit_code, wrote=0, left=pos->second - pos->first;
    bool errorExit=false;

    interpreterLock.lock();
    code=gsapi_run_string_begin (ghostScriptInstance, 0, &exit_code);

    if (exit_code)
        errorExit=handleExit(exit_code);

    if (errorExit)
        return false;
//    kdDebug(4655) << "Left " << left  <<endl;
    while (left > 0)
    {
        read=fread (buf,sizeof(char),QMIN(sizeof(buf),left),tmp);
        wrote=gsapi_run_string_continue (ghostScriptInstance, buf, read, 0, &exit_code);
        if (exit_code)
        {
            errorExit=handleExit(exit_code);

            if (errorExit)
                return false;
        }
        left-=read;
//         kdDebug(4655) << "Left " << left  << " read " << read  << " wrote " << wrote  <<endl;
    }
//     kdDebug(4655) << "Left " << left  << " wrote " << wrote  <<endl;
    //wrote=gsapi_run_string_continue (ghostScriptInstance, "showpage", 8, 0, &exit_code);
    //kdDebug(4655) << "Left " << left  << " wrote " << wrote  <<endl;

    gsapi_run_string_end (ghostScriptInstance, 0, &exit_code);
//     kdDebug(4655) << "Ending string " << endl;

    if (exit_code)
    {
        errorExit=handleExit(exit_code);

        if (errorExit)
            return false;
    }

    kdDebug(4655) << "unlocking interpreter " << left  <<endl;
    interpreterLock.unlock();
    if (m_sync && m_syncDone)
        emit newPageImage (m_req);
    m_syncDone=false;
    return true;
}

QPixmap* GSInterpreterLib::takePixmap()
{
        QPixmap* x=m_pix;
        m_pix = 0;
        m_req = 0;
        m_ready=true;
        return x;
}

GSInterpreterLib::~GSInterpreterLib()
{
    if (running())
        gsapi_exit(ghostScriptInstance);

    gsapi_delete_instance(ghostScriptInstance);
}

// gs api wrapping 
int GSInterpreterLib::gs_input  ( char* buffer, int len )
{
    emit io (Input,buffer,len);
    return len;
}
int GSInterpreterLib::gs_output ( const char* buffer, int len )
{
    emit io (Output,buffer,len);
    return len;
}

int GSInterpreterLib::gs_error  ( const char* buffer, int len )
{
    emit io (Error,buffer,len);
    return len;
}

int GSInterpreterLib::size(int width, int height,
        int raster, unsigned int format, unsigned char *pimage)
{
    kdDebug(4655) << "size called, width/height/raster: "<< width << " " << height << " " << raster << " sizeof pimage " << sizeof(pimage) << endl;
    m_raster=raster;
    m_format=format;
    m_imageChar=pimage;
    m_Gwidth=width;
    m_Gheight=height;
    return 0;
}

#include <qdialog.h>
#include <qlabel.h>
#include <qpixmap.h>

int GSInterpreterLib::page()
{
    kdDebug(4655) << "page called with m_sync == " << m_sync << endl;
    if (m_sync && !m_syncDone)
    {
    m_ready=false;
        // assume the image given by the gs is of the requested widthxheight
            kdDebug(4655) << "Size of raster" << m_Gwidth << " " << m_Gheight << endl;
            kdDebug(4655) << "Needed size " << m_width << " " << m_height << endl;
    QImage img(m_imageChar, m_Gwidth, m_Gheight, 
            32, (QRgb*) 0, 0,  QImage::BigEndian );
    //img.setAlphaBuffer(true);
    m_pix = new QPixmap (m_width, m_height);
    m_pix -> fill();

    QPainter* p=new QPainter(m_pix);

    if (m_width != m_Gwidth || m_height != m_Gheight )
        p->drawImage(0,0,img,0,0,m_Gwidth,m_Gheight);

    delete p;
    kdDebug(4655) << "Final size " << m_pix->width() << " " << m_pix->height() << endl;
#ifdef SHOWMEHOWKPDFWASTESRESOURCES
    QDialog t(0);
    QLabel u(&t);
    u.setAutoResize(true);
    u.setPixmap(*m_pix);
    u.show();
    t.exec();
#endif

        m_syncDone=true;
    }
    return 0;
}

bool GSInterpreterLib::handleExit(int code)
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

void GSInterpreterLib::argsToChar()
{
    if (m_argsChar)
    {
        for (int i=0;i<m_argsCCount;i++)
            delete [] *(m_argsChar+i);
        delete [] m_argsChar;
    }
    m_args.clear();
    m_args  << " "
//        << "-q"
        <<"-dMaxBitmap=10000000 "
        << "-dDELAYSAFER"
        << "-dNOPAUSE"
        << "-dNOPAGEPROMPT";
        if ( GSSettings::antialiasing() )
            m_args  << "-dTextAlphaBits=4"
                    <<"-dGraphicsAlphaBits=2";
        if ( !GSSettings::platformFonts() )
            m_args << "-dNOPLATFONTS";
        m_args //<< GSSettings::arguments()
        << QString("-sPAPERSIZE=%1").arg(m_media.lower())
        << QString().sprintf("-r%dx%d",(int)floor(m_magnify*QPaintDevice:: x11AppDpiX()),(int)floor(m_magnify*QPaintDevice:: x11AppDpiY()))
        << QString().sprintf("-dDisplayFormat=%d", DISPLAY_COLORS_RGB | DISPLAY_UNUSED_LAST | DISPLAY_DEPTH_8 |
        DISPLAY_BIGENDIAN | DISPLAY_TOPFIRST)
        << QString().sprintf("-dDisplayHandle=16#%llx", (unsigned long long int) this )
        // FIXME orientation
        /*<< QString("-sOrientation=%1").arg(m_orientation)*/;
        

    int t=m_args.count();
    char ** args=static_cast <char**> (new char* [t]);
    for (int i=0;i<t;i++)
    {
       kdDebug(4655) << "Arg nr " << i << " : " << m_args[i].local8Bit() << endl ;
        *(args+i)=new char [m_args[i].length()+1];
        qstrcpy (*(args+i),m_args[i].local8Bit());
    }

    m_argsChar=args;
    m_argsCCount=t;
}

#include "interpreter_lib.moc"
