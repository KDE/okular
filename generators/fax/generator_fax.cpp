/***************************************************************************
 *   Copyright (C) 2008 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_fax.h"

#include "faxdocument.h"

#include <QtGui/QPainter>
#include <QtGui/QPrinter>

#include <kaboutdata.h>
#include <klocale.h>

#include <core/document.h>
#include <core/page.h>

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_fax",
         "okular_fax",
         ki18n( "Fax Backend" ),
         "0.1.1",
         ki18n( "A G3/G4 fax document backend" ),
         KAboutData::License_GPL,
         ki18n( "© 2008 Tobias Koenig" )
    );
    aboutData.addAuthor( ki18n( "Tobias Koenig" ), KLocalizedString(), "tokoe@kde.org" );

    return aboutData;
}

OKULAR_EXPORT_PLUGIN( FaxGenerator, createAboutData() )

FaxGenerator::FaxGenerator( QObject *parent, const QVariantList &args )
    : Generator( parent, args )
{
    setFeature( Threaded );
    setFeature( PrintNative );
    setFeature( PrintToFile );
}

FaxGenerator::~FaxGenerator()
{
}

bool FaxGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    FaxDocument::DocumentType type;
    if ( fileName.toLower().endsWith( ".g3" ) )
        type = FaxDocument::G3;
    else
        type = FaxDocument::G4;

    FaxDocument faxDocument( fileName, type );

    if ( !faxDocument.load() )
    {
        emit error( i18n( "Unable to load document" ), -1 );
        return false;
    }

    m_img = faxDocument.image();

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    m_docInfo = new Okular::DocumentInfo();
    if ( type == FaxDocument::G3 )
        m_docInfo->set( Okular::DocumentInfo::MimeType, "image/fax-g3" );
    else
        m_docInfo->set( Okular::DocumentInfo::MimeType, "image/fax-g4" );

    return true;
}

bool FaxGenerator::doCloseDocument()
{
    m_img = QImage();
    delete m_docInfo;
    m_docInfo = 0;

    return true;
}

QImage FaxGenerator::image( Okular::PixmapRequest * request )
{
    // perform a smooth scaled generation
    int width = request->width();
    int height = request->height();
    if ( request->page()->rotation() % 2 == 1 )
        qSwap( width, height );

    return m_img.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
}

const Okular::DocumentInfo * FaxGenerator::generateDocumentInfo()
{
    return m_docInfo;
}

bool FaxGenerator::print( QPrinter& printer )
{
    QPainter p( &printer );

    QImage image( m_img );

    if ( ( image.width() > printer.width() ) || ( image.height() > printer.height() ) )

        image = image.scaled( printer.width(), printer.height(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation );

    p.drawImage( 0, 0, image );

    return true;
}

#include "generator_fax.moc"

