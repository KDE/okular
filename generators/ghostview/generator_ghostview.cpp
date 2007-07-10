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
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qtoolbox.h>

#include <kactioncollection.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kprinter.h>
#include <ktemporaryfile.h>

#include <okular/core/page.h>

#include "ui_gssettingswidget.h"
#include "gssettings.h"

#include "gvlogwindow.h"
#include "interpreter_cmd.h"
#include "internaldocument.h"
#include "interpreter.h"
#include "generator_ghostview.h"
#include "gshandler.h"

OKULAR_EXPORT_PLUGIN(GSGenerator)

static Okular::PageSize buildPageSizeFromCDSCMEDIA( const CDSCMEDIA & media )
{
    return Okular::PageSize( media.width, media.height, QString::fromAscii( media.name ) );
}

GSGenerator::GSGenerator() :
    Okular::Generator(),
    m_converted(false),
    m_request(0)
{
    setFeature( PageSizes );
    internalDoc = 0;
    dscForPDF = 0;
// TODO
//     if ( GSSettings::messages() )
//     {
//         m_logWindow = new GSLogWindow();
//     }
//     else
        m_logWindow = 0;

    for ( int i = 0; i < CDSC_KNOWN_MEDIA; ++i )
    {
        m_pageSizes.append( buildPageSizeFromCDSCMEDIA( dsc_known_media[i] ) );
    }

    GSInterpreterCMD *asyncGenerator = GSInterpreterCMD::getCreateInterpreter();
    if (!asyncGenerator->isRunning()) asyncGenerator->start();
    connect(asyncGenerator, SIGNAL(imageDone(QImage *, Okular::PixmapRequest *)),
                            SLOT(slotImageGenerated(QImage *, Okular::PixmapRequest *)),
                            Qt::QueuedConnection);
}

GSGenerator::~GSGenerator()
{
}

bool GSGenerator::reparseConfig()
{
    return false;
}

void GSGenerator::addPages( KConfigDialog *dlg )
{
    Ui_GSSettingsWidget gsw;
    QWidget* w = new QWidget(0);
    gsw.setupUi(w);
    dlg->addPage(w, GSSettings::self(), i18n("Ghostscript"), "kghostview", i18n("Ghostscript backend configuration") );
}

CDSC_ORIENTATION_ENUM GSGenerator::orientation( int rot ) const
{
    Q_ASSERT( rot >= 0 && rot < 4 );
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

int GSGenerator::rotation ( CDSC_ORIENTATION_ENUM orientation ) const
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
        default: ;
    }
// get rid of warnings, should never happen
    return 0;
}

// From kghostview
int GSGenerator::angle( CDSC_ORIENTATION_ENUM orientation ) const
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

bool GSGenerator::print( KPrinter& printer ) 
{
    KTemporaryFile tf;
    tf.setSuffix( ".ps" );
    if ( tf.open() )
    {
        bool result = false;
        if ( internalDoc->savePages( tf.fileName(), printer.pageList() ) )
        {
            result = printer.printFiles( QStringList( tf.fileName() ), true );
        }
        tf.close();
        return result;
    }
    return false; 
}

bool GSGenerator::loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector )
{
    QString name=fileName;

    bool ps=true;
/*    KMimeType::Ptr mime = KMimeType::findByPath (name);
    if (mime->name().contains("pdf"))
    {
        ps=false;
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
        << QString("-sPDFname=%1").arg(fileName)
        << QString("-sDSCname=%1").arg(dscForPDF->name())
        << "-c '<< /PermitFileReading [ InputFile ] /PermitFileWriting [] /PermitFileControl [] >> setuserparams .locksafe'"
        << "-f pdf2dsc.ps"
        << "-c quit";
        GSInterpreterLib * m_convert=new GSInterpreterLib();
        m_convert->setGhostscriptArguments(arg);
        m_convert->start();
        delete m_convert;
        m_convert=0;
        arg.clear();
        name=dscForPDF->name();
    }*/
    return loadDocumentWithDSC(name,pagesVector,ps);
}

bool GSGenerator::closeDocument()
{
    delete internalDoc;
    internalDoc = 0;

    return true;
}

void GSGenerator::slotImageGenerated(QImage *img, Okular::PixmapRequest *request)
{
    // This can happen as GSInterpreterCMD is a singleton and on creation signals all the slots
    // of all the generators attached to it
    if (request != m_request) return;

    m_request = 0;
    QPixmap *pix = new QPixmap(QPixmap::fromImage(*img));
    delete img;
    request->page()->setPixmap( request->id(), pix );
    signalPixmapRequestDone( request );
}

Okular::PageSize::List GSGenerator::pageSizes() const
{
    return m_pageSizes;
}

void GSGenerator::pageSizeChanged( const Okular::PageSize &size, const Okular::PageSize &/*oldSize*/ )
{
    for ( int i = 0; i < m_pageSizes.count(); ++i )
    {
        if ( size == m_pageSizes.at(i) )
        {
            internalDoc->setMedia( size.name() );
            kDebug() << "New Page size:" << size.name() << ":" << internalDoc->computePageSize( internalDoc->pageMedia() ) << endl;
            break;
        }
    }
}

QString GSGenerator::xmlFile() const
{
    return QString();
}

void GSGenerator::setupGui( KActionCollection *ac, QToolBox *tBox )
{
// TODO
/*    if ( GSSettings::messages() )
    {
        m_box=tBox;
        m_box->addItem( m_logWindow, KIcon("queue"), i18n("GhostScript Messages") );
    }*/
    m_actionCollection = ac;
}

void GSGenerator::freeGui()
{
// TODO
/*    if ( m_logWindow )
    {
        m_box->removeItem(m_box->indexOf(m_logWindow));
    }*/
}

bool GSGenerator::loadPages( QVector< Okular::Page * > & pagesVector )
{
    QSize pSize;
    bool atLeastOne=false;
    if( internalDoc->dsc()->isStructured() )
    {
        unsigned int i, end=internalDoc->dsc() -> page_count();
        internalDoc->setProlog(qMakePair (internalDoc->dsc()->beginprolog(),
                internalDoc->dsc()->endprolog()));
        internalDoc->setSetup(qMakePair (internalDoc->dsc()->beginsetup(),
             internalDoc->dsc()->endsetup()));
        CDSCPAGE * tmpPage;
        for ( i=0 ;i < end ; i++ )
        {
            tmpPage=(internalDoc->dsc() -> page() + i);
            if (!tmpPage)
            {
                kDebug() << "no tmpPage for page nr " << i << endl;
                continue;
            }
            pSize = internalDoc -> computePageSize( internalDoc -> pageMedia( i ) );
            pSize.setHeight((int)ceil(pSize.height()*DPIMod::Y));
            pSize.setWidth((int)ceil(pSize.width()*DPIMod::X));
            pagesVector[i]=new Okular::Page( i, pSize.width(),
                pSize.height() , (Okular::Rotation)rotation (internalDoc ->  orientation(i) ) );
            internalDoc -> insertPageData (i,qMakePair(tmpPage->begin, tmpPage->end));
            atLeastOne=true;
        }
    }
    else
    {
        pSize = internalDoc -> computePageSize( internalDoc -> pageMedia() );
        pSize.setHeight((int)ceil(pSize.height()*DPIMod::Y));
        pSize.setWidth((int)ceil(pSize.width()*DPIMod::X));
        QFile f(internalDoc->fileName());
        unsigned long end = f.size();
        internalDoc -> insertPageData (0,qMakePair((unsigned long) 0, end));
        pagesVector.resize(1);
        pagesVector[0]=new Okular::Page( 0, pSize.width(),
            pSize.height() , (Okular::Rotation)rotation (internalDoc ->  orientation() ) );
        atLeastOne=true;
    }
    return atLeastOne;
}

/*bool GSGenerator::initInterpreter()
{
    if (! pixGenerator->running())
    {
        if( pixGenerator->start(true) && internalDoc->dsc()->isStructured() )
        {
            kWarning() << "setStructure\n";
            // this 0 is ok here, we will not be getting a PAGE answer from those
            pixGenerator->run ( internalDoc->file() , internalDoc->prolog(), false);
            pixGenerator->run ( internalDoc->file() , internalDoc->setup(), false );
        }
    }
    return pixGenerator->running();
}*/

bool GSGenerator::loadDocumentWithDSC( const QString & name, QVector< Okular::Page * > & pagesVector, bool ps )
{
    internalDoc = new GSInternalDocument (name, ps ? GSInternalDocument::PS : GSInternalDocument::PDF);
    pagesVector.resize( internalDoc->dsc()->page_count() );
    kDebug() << "Page count: " << internalDoc->dsc()->page_count() << endl;
    kDebug() << "Page size: " << internalDoc->computePageSize( internalDoc->pageMedia() ) << endl;
    return loadPages (pagesVector);
}

void GSGenerator::generatePixmap( Okular::PixmapRequest * req )
{
    kWarning() << "receiving req id=" << req->id() << " " <<req->width() << "x" << req->height() << "@" << req->pageNumber() << " async == " << req->asynchronous() << endl;
    int pgNo=req->pageNumber();
    double width = req->page()->width();
    double height = req->page()->height();
    int reqwidth = req->width();
    int reqheight = req->height();
    if ( req->page()->rotation() % 2 == 1 )
    {
        qSwap( width, height );
        qSwap( reqwidth, reqheight );
    }

    GSInterpreterCMD *asyncGenerator = GSInterpreterCMD::getCreateInterpreter();
    asyncGenerator->setPlatformFonts(GSSettings::platformFonts());
    if ( GSSettings::antialiasing())
    {
        asyncGenerator->setAABits(4,2);
    }
    else
    {
        asyncGenerator->setAABits(1,1);
    }

// TODO check if needed
//    asyncGenerator->setOrientation(rotation (internalDoc->orientation(pgNo)));
    asyncGenerator->setMedia( internalDoc -> getPaperSize ( internalDoc -> pageMedia( pgNo )) );
    asyncGenerator->setMagnify( qMax( (double)reqwidth / width, (double)reqheight / height ) );
    PsPosition u=internalDoc->pagePos(pgNo);
    if ( internalDoc->dsc()->isStructured() )
    {
        asyncGenerator->setStructure( internalDoc->prolog() , internalDoc->setup() );
    }
    asyncGenerator->setPosition(internalDoc->pagePos(pgNo));
    asyncGenerator->setFileName(internalDoc->fileName());
    m_request = req;
    asyncGenerator->startRequest(req);
}

bool GSGenerator::canGeneratePixmap() const
{
    return !m_request;
}

const Okular::DocumentInfo * GSGenerator::generateDocumentInfo()
{
    return internalDoc->generateDocumentInfo();
}

#include "generator_ghostview.moc"
