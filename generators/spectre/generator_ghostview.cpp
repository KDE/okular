/***************************************************************************
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
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
#include <QtGui/QPrinter>

#include <kaboutdata.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <ktemporaryfile.h>

#include <okular/core/document.h>
#include <okular/core/page.h>
#include <okular/core/fileprinter.h>

#include "ui_gssettingswidget.h"
#include "gssettings.h"

#include "generator_ghostview.h"
#include "rendererthread.h"

static KAboutData createAboutData()
{
    // ### TODO fill after the KDE 4.0 unfreeze
    KAboutData aboutData(
         "okular_ghostview",
         "okular_ghostview",
         KLocalizedString(),
         "0.1",
         KLocalizedString(),
         KAboutData::License_GPL,
         KLocalizedString()
    );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN(GSGenerator, createAboutData())

GSGenerator::GSGenerator( QObject *parent, const QVariantList &args ) :
    Okular::Generator( parent, args ),
    m_internalDocument(0),
    m_docInfo(0),
    m_request(0)
{
    setFeature( PrintPostscript );
    setFeature( PrintToFile );

    GSRendererThread *renderer = GSRendererThread::getCreateRenderer();
    if (!renderer->isRunning()) renderer->start();
    connect(renderer, SIGNAL(imageDone(QImage *, Okular::PixmapRequest *)),
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
    QWidget* w = new QWidget(dlg);
    gsw.setupUi(w);
    dlg->addPage(w, GSSettings::self(), i18n("Ghostscript"), "kghostview", i18n("Ghostscript backend configuration") );
}

bool GSGenerator::print( QPrinter& printer )
{
    bool result = false;

    // Create tempfile to write to
    KTemporaryFile tf;
    tf.setSuffix( ".ps" );
    if ( !tf.open() )
        return false;

    // Get list of pages to print
    QList<int> pageList = Okular::FilePrinter::pageList( printer,
                                               spectre_document_get_n_pages( m_internalDocument ),
                                               document()->bookmarkedPageList() );

    // Default to Postscript export, but if printing to PDF use that instead
    SpectreExporterFormat exportFormat = SPECTRE_EXPORTER_FORMAT_PS;
    if ( printer.outputFileName().right(3) == "pdf" )
    {
        exportFormat = SPECTRE_EXPORTER_FORMAT_PDF;
        tf.setSuffix(".pdf");
    }

    SpectreExporter *exporter = spectre_exporter_new( m_internalDocument, exportFormat );
    SpectreStatus exportStatus = spectre_exporter_begin( exporter, tf.fileName().toAscii() );

    int i = 0;
    while ( i < pageList.count() && exportStatus == SPECTRE_STATUS_SUCCESS )
    {
        exportStatus = spectre_exporter_do_page( exporter, i );
        i++;
    }

    SpectreStatus endStatus = spectre_exporter_end( exporter );

    spectre_exporter_free( exporter );

    if ( exportStatus == SPECTRE_STATUS_SUCCESS && endStatus == SPECTRE_STATUS_SUCCESS )
    {
        tf.setAutoRemove( false );
        int ret = Okular::FilePrinter::printFile( printer, tf.fileName(),
                                                  Okular::FilePrinter::SystemDeletesFiles,
                                                  Okular::FilePrinter::ApplicationSelectsPages,
                                                  document()->bookmarkedPageRange() );
        if ( ret >= 0 ) result = true;
    }

    tf.close();

    return result;
}

bool GSGenerator::loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector )
{
    m_internalDocument = spectre_document_new();
    spectre_document_load(m_internalDocument, QFile::encodeName(fileName));
    pagesVector.resize( spectre_document_get_n_pages(m_internalDocument) );
    kDebug() << "Page count: " << pagesVector.count();
    return loadPages(pagesVector);
}

bool GSGenerator::doCloseDocument()
{
    spectre_document_free(m_internalDocument);
    m_internalDocument = 0;

    delete m_docInfo;
    m_docInfo = 0;

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

bool GSGenerator::loadPages( QVector< Okular::Page * > & pagesVector )
{
    for (uint i = 0; i < spectre_document_get_n_pages(m_internalDocument); i++)
    {
        SpectrePage     *page;
        int              width = 0, height = 0;
        SpectreOrientation pageOrientation = SPECTRE_ORIENTATION_PORTRAIT;
        page = spectre_document_get_page (m_internalDocument, i);
        if (spectre_document_status (m_internalDocument)) {
            kDebug() << "Error getting page" << i << spectre_status_to_string(spectre_document_status(m_internalDocument));
        } else {
            spectre_page_get_size(page, &width, &height);
            pageOrientation = spectre_page_get_orientation(page);
        }
        spectre_page_free(page);
        if (pageOrientation % 2 == 1) qSwap(width, height);
        pagesVector[i] = new Okular::Page(i, width, height, orientation(pageOrientation));
    }
    return pagesVector.count() > 0;
}

void GSGenerator::generatePixmap( Okular::PixmapRequest * req )
{
    kWarning() << "receiving" << *req;

    SpectrePage *page = spectre_document_get_page(m_internalDocument, req->pageNumber());

    GSRendererThread *renderer = GSRendererThread::getCreateRenderer();
    renderer->setPlatformFonts(GSSettings::platformFonts());
    int graphicsAA = 1;
    int textAA = 1;
    if (GSSettings::graphicsAntialiasing()) graphicsAA = 4;
    if (GSSettings::textAntialiasing()) textAA = 2;
    renderer->setAABits(graphicsAA, textAA);

    renderer->setRotation( req->page()->orientation() * 90 );
    renderer->setMagnify( qMax( (double)req->width() / req->page()->width(),
                                (double)req->height() / req->page()->height() ) );
    m_request = req;
    renderer->startRequest(req, page);
}

bool GSGenerator::canGeneratePixmap() const
{
    return !m_request;
}

const Okular::DocumentInfo * GSGenerator::generateDocumentInfo()
{
    if (!m_docInfo)
    {
        m_docInfo = new Okular::DocumentInfo();

        m_docInfo->set( Okular::DocumentInfo::Title, spectre_document_get_title(m_internalDocument) );
        m_docInfo->set( Okular::DocumentInfo::Author, spectre_document_get_for(m_internalDocument) );
        m_docInfo->set( Okular::DocumentInfo::Creator, spectre_document_get_creator(m_internalDocument) );
        m_docInfo->set( Okular::DocumentInfo::CreationDate, spectre_document_get_creation_date(m_internalDocument) );
        m_docInfo->set( "dscversion", spectre_document_get_format(m_internalDocument), i18n("Document version") );

        int languageLevel = spectre_document_get_language_level(m_internalDocument);
        if (languageLevel > 0) m_docInfo->set( "langlevel", QString::number(languageLevel), i18n("Language Level") );
        if (spectre_document_is_eps(m_internalDocument))
            m_docInfo->set( Okular::DocumentInfo::MimeType, "image/x-eps" );
        else
            m_docInfo->set( Okular::DocumentInfo::MimeType, "application/postscript" );

        m_docInfo->set( Okular::DocumentInfo::Pages, QString::number(spectre_document_get_n_pages(m_internalDocument)) );
    }
    return m_docInfo;
}

Okular::Rotation GSGenerator::orientation(SpectreOrientation pageOrientation) const
{
    switch (pageOrientation)
    {
        case SPECTRE_ORIENTATION_PORTRAIT:
            return Okular::Rotation0;
        case SPECTRE_ORIENTATION_LANDSCAPE:
            return Okular::Rotation270;
        case SPECTRE_ORIENTATION_REVERSE_PORTRAIT:
            return Okular::Rotation180;
        case SPECTRE_ORIENTATION_REVERSE_LANDSCAPE:
            return Okular::Rotation90;
    }
// get rid of warnings, should never happen
    return Okular::Rotation0;
}


#include "generator_ghostview.moc"
