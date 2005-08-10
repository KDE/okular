/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <math.h>

#include <qfile.h>
#include <qsize.h>
#include <qtoolbox.h>

#include <kconfigdialog.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kprinter.h>
#include <ktempfile.h>

#include "core/page.h"
#include "core/generator.h"
#include "conf/gssettingswidget.h"
#include "conf/gssettings.h"

#include "gvlogwindow.h"
#include "interpreter_cmd.h"
#include "interpreter_lib.h"
#include "internaldocument.h"

#include "generator_ghostview.h"

KPDF_EXPORT_PLUGIN(GSGenerator)

GSGenerator::GSGenerator( KPDFDocument * doc ) :
    Generator ( doc ),
    m_converted(false)
{
    pixGenerator = 0;
    asyncGenerator = 0;
    internalDoc = 0;
    dscForPDF = 0;
    m_asyncBusy = false;
    if ( GSSettings::messages() )
        m_logWindow = new GSLogWindow(QString ("Logwindow"));
}

GSGenerator::~GSGenerator()
{
    delete asyncGenerator;
    delete pixGenerator;
    delete m_logWindow;
}

void GSGenerator::addPages( KConfigDialog *dlg )
{
    dlg->addPage(new GSSettingsWidget(0), GSSettings::self() , i18n ("Ghostscript backend"), "kghostview" );
}

CDSC_ORIENTATION_ENUM GSGenerator::orientation( int  rot )
{
    Q_ASSERT( rot > 0 && rot < 4 );
    switch (rot)
    {
        case 0:
            return CDSC_PORTRAIT;
        case 1:
            return CDSC_LANDSCAPE;
        case 2:
            return CDSC_UPSIDEDOWN;
        case 3:
            return CDSC_SEASCAPE;
    }
// get rid of warnings, should never happen
    return CDSC_PORTRAIT;
}

int GSGenerator::rotation ( CDSC_ORIENTATION_ENUM orientation )
{
    Q_ASSERT( orientation != CDSC_ORIENT_UNKNOWN );
    switch (orientation)
    {
        case CDSC_PORTRAIT:
            return 0;
        case CDSC_LANDSCAPE:
            return 1;
        case CDSC_UPSIDEDOWN:
            return 2;
        case CDSC_SEASCAPE:
            return 3;
    }
// get rid of warnings, should never happen
            return 0;
}

// From kghostview
int GSGenerator::angle( CDSC_ORIENTATION_ENUM orientation )
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

void GSGenerator::setOrientation(QValueVector<KPDFPage*>& pages, int rot) 
{
    internalDoc->setOrientation(orientation(rot));
    loadPages (pages);
}

inline QString GSGenerator::fileName() { return internalDoc->fileName(); };

bool GSGenerator::print( KPrinter& printer ) 
{
    KTempFile tf( QString::null, ".ps" );
    if( tf.status() == 0 ) 
    {
        if ( internalDoc->savePages( tf.name(), printer.pageList() ) )
        {
            return printer.printFiles( QStringList( tf.name() ), true );
        }
    }
    return false; 
}

bool GSGenerator::loadDocument( const QString & fileName, QValueVector< KPDFPage * > & pagesVector )
{
    QString name=fileName;

    bool ps=true;
    KMimeType::Ptr mime = KMimeType::findByPath (name);
    if (mime->name().contains("pdf"))
    {
        ps=false;
        //m_convertSuccess=false;
        dscForPDF=new KTempFile( QString::null, ".ps" );
        Q_CHECK_PTR( dscForPDF );
        dscForPDF->setAutoDelete(true);
        if( dscForPDF->status() != 0 )
        {
            // error handling here
        }
        dscForPDF->close();
        QStringList arg;
        arg << " "
        << "-q"
        << "-dPARANOIDSAFER"
        << "-dSAFER"
        << "-dDELAYSAFER"
        << "-dNODISPLAY"
        << "-dQUIET"
        << QString("-sPDFname=%1").arg(name)
        << QString("-sDSCname=%1").arg(dscForPDF->name())
        << "-c '/PermitFileReading [ PDFname ] /PermitFileWriting [ DSCname ] /PermitFileControl []'"
        << "-f pdf2dsc.ps"
        << "-c quit";
        m_convert=new GSInterpreterLib();
        m_convert->setGhostscriptArguments(arg);
        m_convert->startInterpreter();
        delete m_convert;
        m_convert=0;
        arg.clear();
        name=dscForPDF->name();
    }

    if( !pixGenerator )
    {
        pixGenerator = new GSInterpreterLib ();
        connect (pixGenerator, SIGNAL (newPageImage(PixmapRequest *)),
         this, SLOT(slotPixmapGenerated (PixmapRequest *)));

        if ( GSSettings::messages() )
        {
            connect (pixGenerator, SIGNAL (io( MessageType , const  char*, int )),
                m_logWindow, SLOT (append(MessageType , const char*,int)));
        }
    }
    if (! asyncGenerator )
    {
        asyncGenerator= new GSInterpreterCMD ( fileName );
        connect (asyncGenerator, SIGNAL (newPageImage(PixmapRequest *)),
         this, SLOT(slotAsyncPixmapGenerated (PixmapRequest *)));
    }
    return loadDocumentWithDSC(name,pagesVector,ps);
}

void GSGenerator::slotPixmapGenerated(PixmapRequest * request )
{
    request->page->setPixmap( request->id, pixGenerator->takePixmap() );
    signalRequestDone( request );
}

void GSGenerator::slotAsyncPixmapGenerated(PixmapRequest * request )
{
    QPixmap *pix=asyncGenerator->takePixmap();
    docLock.unlock();
    request->page->setPixmap( request->id, pix );
    signalRequestDone( request );
}

void GSGenerator::setupGUI(KActionCollection  * /*ac*/ , QToolBox * tBox )
{
    m_box=tBox;
    m_box->addItem( m_logWindow, QIconSet(SmallIcon("queue")), i18n("GhostScript Messages") );
}

bool GSGenerator::loadPages( QValueVector< KPDFPage * > & pagesVector )
{
    unsigned int i, end=internalDoc->dsc() -> page_count();
    if( internalDoc->dsc()->isStructured() )
    {
        internalDoc->setProlog(qMakePair (internalDoc->dsc()->beginprolog(),
                internalDoc->dsc()->endprolog()));
        internalDoc->setSetup(qMakePair (internalDoc->dsc()->beginsetup(),
             internalDoc->dsc()->endsetup()));
    }

    QSize pSize;
    CDSCPAGE * tmpPage;
    bool atLeastOne=false;
    for ( i=0 ;i < end ; i++ )
    {
        tmpPage=(internalDoc->dsc() -> page() + i);
        if (!tmpPage)
            continue;
        pSize = internalDoc -> computePageSize( internalDoc -> pageMedia( i ) );
        pSize.setHeight((int)ceil(pSize.height()*DPIMod::Y));
        pSize.setWidth((int)ceil(pSize.width()*DPIMod::X));
        pagesVector[i]=new KPDFPage( i, pSize.width(),
            pSize.height() , rotation (internalDoc ->  orientation(i) ) );
        internalDoc -> insertPageData (i,qMakePair(tmpPage->begin, tmpPage->end));
        atLeastOne=true;
    }

    return atLeastOne;
}

bool GSGenerator::initInterpreter()
{
    if (! pixGenerator->running())
    {
        if( pixGenerator->startInterpreter() && internalDoc->dsc()->isStructured() )
        {
            // this 0 is ok here, we will not be getting a PAGE anwser from those
            pixGenerator->run ( internalDoc->file() , internalDoc->prolog(), static_cast<PixmapRequest*>(0x0),false);
            pixGenerator->unlock();
            pixGenerator->run ( internalDoc->file() , internalDoc->setup(), static_cast<PixmapRequest*>(0x0),false );
            pixGenerator->unlock();
        }
    }
    return pixGenerator->running();
}

bool GSGenerator::loadDocumentWithDSC( QString & name, QValueVector< KPDFPage * > & pagesVector, bool ps )
{
    internalDoc = new GSInternalDocument (name, ps ? GSInternalDocument::PS : GSInternalDocument::PDF);
    pagesVector.resize( internalDoc->dsc()->page_count() );
    return loadPages (pagesVector);
}

void GSGenerator::generatePixmap( PixmapRequest * req )
{
    QString a="S";
    if (req->async) a="As";

    kdDebug() << a << "ync PixmapRequest of " << req->width << "x" 
    << req->height << " size, pageNo " << req->pageNumber 
    << ", priority: " << req->priority << " pageaddress " << (unsigned long long int) req->page
    <<  endl;

    int i=req->pageNumber;

    if ( req->async )
    {
        docLock.lock();
        asyncGenerator->setOrientation(rotation (internalDoc->orientation(i)));
//         asyncGenerator->setBoundingBox( internalDoc->boundingBox(i));
        asyncGenerator->setSize(req->width ,req->height);
        asyncGenerator->setMedia( internalDoc -> getPaperSize ( internalDoc -> pageMedia( i )) );
        asyncGenerator->setMagnify(QMAX(static_cast<double>(req->width)/req->page->width() ,
                static_cast<double>(req->height)/req->page->height()));

        if (!asyncGenerator->running())
        {
            if ( internalDoc->dsc()->isStructured() )
                asyncGenerator->setStructure( internalDoc->prolog() , internalDoc->setup() );
            asyncGenerator->startInterpreter();
        }
        asyncGenerator->run ( internalDoc->pagePos(i) , req);
        return;
    }

    pixGenerator->setOrientation(rotation (internalDoc->orientation(i)));
//     pixGenerator->setBoundingBox( internalDoc->boundingBox(i));
    pixGenerator->setSize(req->width ,req->height);
    pixGenerator->setMedia( internalDoc -> getPaperSize ( internalDoc -> pageMedia( i )) );
    pixGenerator->setMagnify(QMAX(static_cast<double>(req->width)/req->page->width() ,
            static_cast<double>(req->height)/req->page->height()));
    if (!pixGenerator->running())
        initInterpreter();
    pixGenerator->run ( internalDoc->file() , internalDoc->pagePos(i) , req, true);
}


bool GSGenerator::canGeneratePixmap( bool async )
{
    if (async) return ! docLock.locked();
    return ( pixGenerator ) ? pixGenerator->ready() : true;
}

const DocumentInfo * GSGenerator::generateDocumentInfo()
{
    return internalDoc->generateDocumentInfo();
}

#include "generator_ghostview.moc"
