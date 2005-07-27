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
#include <math.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <klocale.h>

#include "interpreter_cmd.h"
#include "conf/settings.h"



int handler( Display* d, XErrorEvent* e )
{
    char msg[80], req[80], number[80];

    XGetErrorText( d, e->error_code, msg, sizeof( msg ) );
    sprintf( number, "%d", e->request_code );
    XGetErrorDatabaseText( d, "XRequest", number, "<unknown>", 
                           req, sizeof( req ) );
    return 0;
}

int orientation2angle( CDSC_ORIENTATION_ENUM orientation )
{
    Q_ASSERT( orientation != CDSC_ORIENT_UNKNOWN );

    int angle = 0;

    switch( orientation ) 
    {
    case CDSC_ORIENT_UNKNOWN:		    break; // Catched by Q_ASSERT
    case CDSC_PORTRAIT:	    angle = 0;	    break;
    case CDSC_LANDSCAPE:    angle = 90;	    break;
    case CDSC_UPSIDEDOWN:   angle = 180;    break;
    case CDSC_SEASCAPE:	    angle = 270;    break;
    }

    return angle;
}

QCString getPaletteString()
{
    QCString str;

    switch( Settings::renderMode() )
    {
        case (int) Settings::EnumRenderMode::BlackWhite:
            if (Settings::bWThreshold() == 2)
                str = "Monochrome";
            else
                str = "Grayscale";
            break;
        default:
            str = "Color";
            break;
    }
    return str;
}

GVInterpreterCMD::GVInterpreterCMD( QWidget* parent, const char* name, const QString & fileName ) :
    QWidget           ( parent, name ),
    comm_window       ( None ),
    m_needUpdate      ( false ),
    m_stdinReady      ( false ),
    m_busy            ( false ),
    m_usePipe         ( false ),
    m_buffer          ( 0 ),
    m_process         ( 0 ),
    m_magnify         ( 1 ),
    m_work            (),
    m_orientation     ( CDSC_PORTRAIT ),
    m_name            (fileName)
{
    XSetErrorHandler( handler );
    kdDebug(4500) << "Constructing interpreter!" << endl;

    // Create the Atoms used to communicate with Ghostscript.
    const char* const atomNames[] = { "GHOSTVIEW", "GHOSTVIEW_COLORS", 
                                      "NEXT", "PAGE", "DONE" };
    XInternAtoms( x11Display(), const_cast<char**>( atomNames ), 
                  5, false, m_atoms );
    m_pixmap=0;
}

GVInterpreterCMD::~GVInterpreterCMD()
{
    if (!m_pixmap)
        delete m_pixmap;
}

void GVInterpreterCMD::stopInterpreter()
{
    kdDebug(4500) << "GVInterpreterCMD::stopInterpreter()" << endl;
    // if( !_interpreterBusy ) return;

    if( running() )
        m_process->kill( SIGHUP );

    m_tmpFile=0;
    m_process = 0;
    m_work.unlock();
}


void GVInterpreterCMD::gs_output( KProcess*, char* buffer, int len )
{
    kdDebug(4500) << "Output sent!" << endl;
    emit output( buffer, len );
}

bool GVInterpreterCMD::startInterpreter()
{

    setupWidget();
    if (!m_pixmap)
        m_pixmap = new QPixmap;

    this->show();
    m_process = new KProcess;
    m_process -> setEnvironment( "GHOSTVIEW", QString(  "%1 %2" ).arg( winId() ).arg( m_pixmap->handle() ) );

    *m_process << m_path.local8Bit();
    *m_process << m_args;

    *m_process << 
        // The following two lines are their to 
        // ensure that we are allowed to read m_fileName
        "-dDELAYSAFER" 
        << "-sInputFile=" + m_name
        << "-c '/PermitFileReading [ InputFile ] /PermitFileWriting [] /PermitFileControl []'"
        << "-";
/*    }
    else
        *m_process << m_name << "-c" << "quit";*/

    connect( m_process, SIGNAL( processExited( KProcess* ) ),
             this, SLOT( slotProcessExited( KProcess* ) ) );
    connect( m_process, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
             this, SLOT( gs_output( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
             this, SLOT( gs_output( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( wroteStdin( KProcess*) ),
             this, SLOT( gs_input( KProcess* ) ) );
    
    // really necessary?
    KApplication::kApplication () -> flushX();

    // Finally fire up the interpreter.
    kdDebug(4500) << "KPSWidget: starting interpreter" << endl;

    if( m_process->start( KProcess::NotifyOnExit,
              /*m_usePipe ?*/ KProcess::All /*: KProcess::AllOutput*/ ) )
    {
        m_busy = true;
        m_work.lock();
        m_stdinReady = true;
        m_needUpdate = false;
        return true;
    }
    else
    {
        KMessageBox::error( this,
            i18n( "Could not start Ghostscript. This is most likely "
            "caused by an incorrectly specified interpreter." ) );
        return false;
    }
}

bool GVInterpreterCMD::busy() 
{
    return (running() && m_busy && m_work.locked() && !m_stdinReady) || (! running()) ;
}

void GVInterpreterCMD::setGhostscriptPath( const QString& path )
{
    kdDebug() << "KPSWidget::setGhostscriptPath( " << path << " )" << endl;
    if( m_path != path )
    {
        m_path = path;
        stopInterpreter();
        m_needUpdate = true;
    }
}

void GVInterpreterCMD::setGhostscriptArguments( const QStringList& arguments )
{
    if( m_args != arguments )
    {
        m_args= arguments;
        stopInterpreter();
        m_needUpdate = true;
    }
}

void GVInterpreterCMD::setOrientation( CDSC_ORIENTATION_ENUM orientation )
{
    if( m_orientation != orientation ) 
    {
        m_orientation = orientation;
            stopInterpreter();
        m_needUpdate = true;
    }
}

void GVInterpreterCMD::setBoundingBox( const KDSCBBOX& boundingBox )
{
    if( m_boundingBox != boundingBox ) 
    {
        m_boundingBox = boundingBox;
        stopInterpreter();
        m_needUpdate = true;
    }
}

void GVInterpreterCMD::setMagnification( double magnification )
{
    if( abs( static_cast <int>(magnification - m_magnify )) > 0.0001 )
    {
        m_magnify = magnification;
        stopInterpreter();
        m_needUpdate = true;
    }
}


bool GVInterpreterCMD::sendPS( FILE* fp, const PagePosition *pos, PixmapRequest * req )
{
    kdDebug(4500) << "KPSWidget::sendPS" << endl;

    if( !running() )
        return false;

    if (!m_pixmap)
        m_pixmap = new QPixmap;

    m_tmpFile = fp;
    m_req=req;
    m_begin=pos->first;
    m_len=static_cast <unsigned int> (pos->first - pos->second);
    // Start processing the queue.
    if( m_stdinReady )
    {
        gs_input( m_process );
    }
    return true;
}


void GVInterpreterCMD::gs_input( KProcess* process )
{
    kdDebug(4500) << "Sending input ::gs_input" << endl;

    if (process != m_process)
    {
        kdDebug(4500) << "BROKEN::gs_input(): process != _process" << endl;
        return;
    }

    m_stdinReady = true;
    if ( fseek( m_tmpFile, m_begin , SEEK_SET ) )
    {
        kdDebug(4500) << "KPSWidget::gs_input(): seek failed!" << endl;
        stopInterpreter();
        return;
    }

    if (m_len == 0)
    {
        m_busy = false;
        m_work.unlock();
    }

    Q_ASSERT( m_len > 0 );

    const unsigned buffer_size = 4096;
    if ( !m_buffer ) 
        m_buffer = static_cast<char*>( operator new( buffer_size ) );

    const int bytesRead = fread( m_buffer, sizeof (char),
            QMIN( buffer_size, m_len ),
            m_tmpFile );

    if( bytesRead > 0 ) 
    {
        m_begin += bytesRead;
        m_len -= bytesRead;
        if( process && process->writeStdin( m_buffer, bytesRead ) )
            m_stdinReady = false;
        else
        {
        
            stopInterpreter();
        }
    }
    else
        stopInterpreter();
}

bool GVInterpreterCMD::x11Event( XEvent* e )
{
    if( e->type == ClientMessage )
    {
        comm_window = e->xclient.data.l[0];

        if( e->xclient.message_type == m_atoms[PAGE] )
        {
            kdDebug() << "received PAGE" << endl;
            m_busy = false;
            if (m_work.locked())
                m_work.unlock();
            m_tmpFile=0;
            emit newPageImage( m_req );
            setErasePixmap( *m_pixmap );
            return true;
        }

        else if( e->xclient.message_type == m_atoms[DONE] )
        {
            kdDebug() << "received DONE" << endl;
            stopInterpreter();
            return true;
        }
    }
    return QWidget::x11Event( e );
}


void GVInterpreterCMD::setupWidget()
{
    if( !m_needUpdate )
        return;

    if (!m_pixmap)
        m_pixmap = new QPixmap;

    Q_ASSERT( m_orientation != CDSC_ORIENT_UNKNOWN );

    const float dpiX = m_magnify * x11AppDpiX();
    const float dpiY = m_magnify * x11AppDpiY();

    int newWidth = 0, newHeight = 0;
    if( m_orientation == CDSC_PORTRAIT || m_orientation == CDSC_UPSIDEDOWN )
    {
        newWidth  = (int) ceil( m_boundingBox.width()  * dpiX / 72.0 );
        newHeight = (int) ceil( m_boundingBox.height() * dpiY / 72.0 );
    }
    else
    {
        newWidth  = (int) ceil( m_boundingBox.height() * dpiX / 72.0 );
        newHeight = (int) ceil( m_boundingBox.width()  * dpiY / 72.0 );
    }

    if( newWidth != width() || newHeight != height() )
    {
        setEraseColor( white );
        setFixedSize( newWidth, newHeight );
        //kapp->processEvents();

        m_pixmap->resize( size() );
        m_pixmap->fill( white );
        // The line below is needed to work around certain "features" of styles such as liquid
        // see bug:61711 for more info (LPC, 20 Aug '03)
        setBackgroundOrigin( QWidget::WidgetOrigin );
        setErasePixmap( *m_pixmap );
    }

    char data[512];

    sprintf( data, "%ld %d %d %d %d %d %g %g",
             m_pixmap->handle() ,
             orientation2angle( m_orientation ),
             m_boundingBox.llx(), m_boundingBox.lly(), 
             m_boundingBox.urx(), m_boundingBox.ury(),
             dpiX, dpiY );

    XChangeProperty( x11Display(), winId(),
                     m_atoms[GHOSTVIEW],
                     XA_STRING, 8, PropModeReplace,
                     (unsigned char*) data, strlen( data ) );

    sprintf( data, "%s %d %d",
             getPaletteString().data(),
             (int)BlackPixel( x11Display(), DefaultScreen( x11Display() ) ),
             (int)WhitePixel( x11Display(), DefaultScreen( x11Display() ) ) );

    XChangeProperty( x11Display(), winId(),
                     m_atoms[GHOSTVIEW_COLORS],
                     XA_STRING, 8, PropModeReplace,
                     (unsigned char*) data, strlen( data ) );

    // Make sure the properties are updated immediately.
    XSync( x11Display(), false );

    repaint();

    m_needUpdate = false;
}

void GVInterpreterCMD::slotProcessExited( KProcess* process )
{
    kdDebug() << "KPSWidget: process exited" << endl;

    if ( process == m_process )
    {
        kdDebug( 4500 ) << "KPSWidget::slotProcessExited(): looks like it was not a clean exit." << endl;
        if ( process->normalExit() ) {
            emit ghostscriptError( QString( i18n( "Exited with error code %1." ).arg( process->exitStatus() ) ) );
        } else {
            emit ghostscriptError( QString( i18n( "Process killed or crashed." ) ) );
        }
        m_process = 0;
        stopInterpreter();
    }
}
#include "interpreter_cmd.moc"
