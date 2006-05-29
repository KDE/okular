/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <qfile.h>

#include <qapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <ktempfile.h>


#include "settings.h"
#include "interpreter_cmd.h"

ProcessData :: ProcessData ()
{
    KTempFile* tmp[2];
    tmp[0] = new KTempFile();
    tmp[1] = new KTempFile();
    tmp[0]->close();
    tmp[1]->close();
    names[0]=QFile::encodeName( tmp[0]->name() );
    names[1]=QFile::encodeName( tmp[1]->name() );
    tmp[0]->unlink();
    tmp[1]->unlink();
    delete tmp[0];
    delete tmp[1];

    mkfifo( names[0].toAscii().constData() , S_IRUSR | S_IWUSR );
    mkfifo( names[1].toAscii().constData() , S_IRUSR | S_IWUSR );
    fds[0]=open ( names[0].toAscii().constData(), O_RDWR );
    fds[1]=open ( names[1].toAscii().constData(), O_RDWR );
}

ProcessData :: ~ProcessData()
{
    close (fds[0]);
    close (fds[1]);
    QFile::remove ( names[0] );
    QFile::remove ( names[1] );
}

QPixmap* GSInterpreterCMD::takePixmap()
{
    kDebug(4655) << "taking pixmap" << endl;
    QPixmap * x=m_pixmap;
    m_pixmap=0;
    return x;
}


GSInterpreterCMD::GSInterpreterCMD( const QString & fileName ) :
    m_process         ( 0 ),
    m_structurePending( false ),
    m_magnify         ( 1 ),
    m_orientation     ( CDSC_PORTRAIT ),
    m_name            ( fileName ),
    m_aaGfx            (1),
    m_aaText           (1),
    m_pfonts            (false)
{
    kDebug(4655) << "Constructing async interpreter!" << endl;
    m_pixmap=0;
}

GSInterpreterCMD::~GSInterpreterCMD()
{
    if (!m_pixmap)
        delete m_pixmap;
    if ( running() )
        stop(false);
    // remove (crashes kpdf somehow, probably because 
    // the destuction thread does the same a line higher
    /* this should not be needed!
    if (m_stoppingPids.count() > 0)
    {
        QMapIterator<pid_t,ProcessData*> it, end=m_stoppingPids.end();
        while ( it != end )
        {
            ProcessData *p=it.data();
            ++it;
            delete p;
        }
    }*/
    m_stoppingPids.clear();
    unlock();
}

void GSInterpreterCMD::destroyInternalProcess(KProcess * stop)
{
    pid_t pId=stop->pid();
    kDebug(4655) << "Destroy thread pid " << getpid() << " of " << pId << endl;
    int x=1;
    ProcessData *mem=m_stoppingPids[pId];
    write(mem->fds[0],&x,sizeof(int));
    stop->wait();
    kDebug(4655) << "Normal exit : " << !stop->isRunning() << endl;
    // give it the time to close the interpreter before we kill it
    if( stop->isRunning() )
    {
        stop->kill();
        stop->wait(5);
        kDebug(4655) << "after stopping, the proces running: "
            << stop->isRunning() << endl;
        if( stop->isRunning() )
            stop->kill( SIGKILL );
    }
    m_stoppingPids.remove( pId );
    delete stop;
    delete mem;
}

bool GSInterpreterCMD::running () 
{
    if (m_process==0)
    {
        kDebug (4655) << "no process\n";
        return false;
    }
    else
    {
        kDebug(4655) << "running " << m_process->isRunning() << endl;
        return m_process->isRunning();
    }
}

void GSInterpreterCMD::setStructure(GSInterpreterLib::Position prolog, GSInterpreterLib::Position setup)
{
    kDebug(4655) << "setStructure()" << endl;
    m_structurePending=true;
    m_data[0]=prolog;
    m_data[1]=setup;
}

bool GSInterpreterCMD::stop(bool async)
{
    kDebug(4655) << "stop()" << endl;
    // if( !_interpreterBusy ) return;

    if ( running() )
    {
        if (m_stoppingPids.contains(m_process->pid()))
            return true;
        KProcess * stop=m_process;
        m_stoppingPids.insert ( stop->pid(), m_processData );
        m_process=0;
        kDebug(4655) << "Launching destroy thread" << endl;
        if (!async)
            destroyInternalProcess(stop);
        else
        {
            switch ( fork() )
            {
                case -1:
                    // we cant kill it in a fork, kill it outside a fork
                    destroyInternalProcess(stop);
                    break;
                case 0:
                    destroyInternalProcess(stop);
                    _exit(0);
                    break;
                default:
                    break;
            }
        }
    }
    return true;
}

bool GSInterpreterCMD::startInterpreter()
{
    kDebug(4655) << "start()" << endl;
    if ( m_process && m_process->isRunning() )
    {
        kDebug(4655) << "ERROR: starting an interpreter while one is running" << endl;
        return false;
    }

    m_processData=new ProcessData();
    m_process = new KProcess;

    (*m_process) << QString("kpdflibgsasyncgenerator");
    // Order of sending: fileName, msgQueueId, media type, magnify, orientation, expected width, height
    QStringList list;
    list << m_name
        << m_processData->names[0]
        << m_processData->names[1]
        << m_media.lower()
        << QString::number ( m_magnify )
        << QString::number ( m_orientation )
        << QString::number ( m_width )
        << QString::number ( m_height )
        << QString::number ( m_pfonts ? 1 : 0 )
        << QString::number (m_aaText)
        << QString::number (m_aaGfx);

    kDebug(4655) << "Argument count: " << list.count() << endl;
    (*m_process) << list;
    /*connect( m_process, SIGNAL( processExited( KProcess* ) ),
             this, SLOT( slotProcessExited( KProcess* ) ) );
    connect( m_process, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
             this, SLOT( output( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
             this, SLOT( output( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( wroteStdin( KProcess*) ),
             this, SLOT( gs_input( KProcess* ) ) );*/

    // Finally fire up the interpreter.
//    kDebug(4500) << "KPSWidget: starting interpreter" << endl;

    if( m_process->start( KProcess::NotifyOnExit,
              /*m_usePipe ?*/ KProcess::All /*: KProcess::AllOutput*/ ) )
    {
        kDebug(4655) << "Starting async! " << m_process->pid() << endl;
        return true;
    }
    else
    {
        emit error(i18n( "Could not start kpdf's libgs helper application. This is most likely "
            "caused by kpdflibgsasyncgenerator not being installed, or installed to a "
            "directory not listed in the environment PATH variable."),0);
        kDebug(4655) << "Could not start helper" << endl;
        return false;
    }
}

void GSInterpreterCMD::setOrientation( int orientation )
{
    lock();
    if( m_orientation != orientation )
    {
        m_orientation = orientation;
        stop();
    }
    unlock();
}

void GSInterpreterCMD::setMagnify( double magnify )
{
    lock();
    if( m_magnify != magnify )
    {
        m_magnify = magnify;
        stop();
    }
    unlock();
}

void GSInterpreterCMD::setMedia( QString media )
{
    lock();
    if( m_media != media )
    {
        m_media = media;
        stop();
    }
    unlock();
}

void GSInterpreterCMD::setPlatformFonts(bool pfonts)
{
    lock();
    if( m_pfonts != pfonts )
    {
        m_pfonts = pfonts;
        stop();
    }
    unlock();
}


void GSInterpreterCMD::setSize( int w, int h )
{
    lock();
    if ( m_width != w ) 
    {
        m_width=w;
        stop();

    }
    if ( m_height != h )
    {
        m_height=h;
        stop();
    }
    unlock();
}

void GSInterpreterCMD::setAABits(int text, int graphics)
{
    lock();
    if ( m_aaText!= text ) 
    {
        m_aaText=text;
        stop();

    }
    if ( m_aaGfx != graphics )
    {
        m_aaGfx=graphics;
        stop();
    }
    unlock();
}

bool GSInterpreterCMD::run( GSInterpreterLib::Position pos)
{
    kDebug(4655) << "Running request with size: " << m_width << "x" << m_height << endl;

    if( !running() )
        return false;

    lock();
    if ( m_pixmap != 0 )
    {
        kDebug(4655) << "ERROR DELETING PIXMAP, THIS SHOULD NOT HAPPEN" << endl;
        delete m_pixmap;
    }
    // the pixmap to which the generated image will be copied
    m_pixmap = new QPixmap (m_width, m_height);
    m_pixmap -> fill();

    m_info.pos=pos;
    m_info.sync=true;
    m_info.handle=m_pixmap->handle();
    start();
    return true;
}

void GSInterpreterCMD::run()
{
    // we are inside a thread
    kDebug(4655)<< "Generation thread started " << getpid() << endl;
    int x;

    // pending structural information -> send them
    if (m_structurePending)
    {
        kDebug(4655) << "sending structural data" << endl;
        PageInfo pageInf;
        for (int i=0;i<2;i++)
        {
            x=0;
            write ( m_processData->fds[0] , &x, sizeof(int) );
            pageInf.sync=false;
            pageInf.pos=m_data[i];
            pageInf.handle=m_info.handle;
            write( m_processData->fds[0], &pageInf, sizeof(pageInf));
            read( m_processData->fds[1],&x,sizeof(int));
        }
        m_structurePending=false;
    }
    kDebug(4655)<< "sending page request" << endl;
    // Communication with the helper looks like this
    // 1. Sendign a 0 to helper telling it to start a function that processes the request
    x=0;
    write ( m_processData->fds[0] , &x, sizeof(int) );
    // 2. sending the structure with relevant information
    write ( m_processData->fds[0], &m_info, sizeof(m_info) );
    // 3. we will receive a '3' from the helper when request is done,
    // the helper will copy the pixmap using XCopyArea
    read ( m_processData->fds[1], &x, sizeof(int) );
    unlock();
    kDebug(4655)<< "pximap ready" << endl;
    if (x==3)
    {
	kDebug(4655)<< "sending the pximap to generator" << endl;
        // inform interpreter about PixmaRequest being done
        QCustomEvent * readyEvent = new QCustomEvent( GS_DATAREADY_ID );
        // set sth just to send nonempty
        readyEvent->setData(&x);
        QApplication::postEvent( this , readyEvent );
    }
}

#include <qdialog.h>
#include <qpainter.h>

void GSInterpreterCMD::customEvent( QCustomEvent * e )
{
    if (e->type() == GS_DATAREADY_ID )
    {
        kWarning() << "emitting signal" << endl;
        QPixmap *pix=takePixmap();
  
        emit Finished(pix);
    }
}

#include "interpreter_cmd.moc"
