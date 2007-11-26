/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2006-2007 by Pino Toscano <pino@kde.org>                *
 *   Copyright (C) 2006-2007 by Tobias Koenig <tokoe@kde.org>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_kimgio.h"

#include <QtCore/QBuffer>
#include <QtGui/QImageReader>
#include <QtGui/QPainter>
#include <QtGui/QPrinter>

#include <kaboutdata.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kicon.h>
#include <klocale.h>

#include <okular/core/page.h>

OKULAR_EXPORT_PLUGIN(KIMGIOGenerator)

KIMGIOGenerator::KIMGIOGenerator()
    : Generator()
{
    setFeature( ReadRawData );
    setFeature( Threaded );
    setFeature( PrintNative );
    setFeature( PrintToFile );

    KAboutData *about = new KAboutData(
         "okular_kimgio",
         "okular_kimgio",
         ki18n( "Image Backend" ),
         "0.1",
         ki18n( "A simple image backend" ),
         KAboutData::License_GPL,
         ki18n( "© 2005 Albert Astals Cid\n"
                "© 2006-2007 Pino Toscano\n"
                "© 2006-2007 Tobias Koenig" )
    );
    about->addAuthor( ki18n( "Albert Astals Cid" ), KLocalizedString(), "aacid@kde.org" );
    about->addAuthor( ki18n( "Pino Toscano" ), KLocalizedString(), "pino@kde.org" );
    about->addAuthor( ki18n( "Tobias Koenig" ), KLocalizedString(), "tokoe@kde.org" );
    setAboutData( about );

/*
    setComponentData( *ownComponentData() );

    setXMLFile( "gui.rc" );
    KAction * kimgio_test = new KAction( this );
    kimgio_test->setText( "Image test" );
    kimgio_test->setIcon( KIcon( "smiley" ) );
    connect( kimgio_test, SIGNAL( triggered( bool ) ), this, SLOT( slotTest() ) );
    actionCollection()->addAction( "kimgio_test", kimgio_test );
*/
}

KIMGIOGenerator::~KIMGIOGenerator()
{
}

bool KIMGIOGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    QImageReader reader( fileName );
    if ( !reader.read( &m_img ) ) {
        emit error( i18n( "Unable to load document: %1", reader.errorString() ), -1 );
        return false;
    }

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector )
{
    QBuffer buffer;
    buffer.setData( fileData );
    buffer.open( QIODevice::ReadOnly );

    QImageReader reader( &buffer );
    if ( !reader.read( &m_img ) ) {
        emit error( i18n( "Unable to load document: %1", reader.errorString() ), -1 );
        return false;
    }

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::doCloseDocument()
{
    m_img = QImage();

    return true;
}

QImage KIMGIOGenerator::image( Okular::PixmapRequest * request )
{
    // perform a smooth scaled generation
    int width = request->width();
    int height = request->height();
    if ( request->page()->rotation() % 2 == 1 )
        qSwap( width, height );

    return m_img.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
}

bool KIMGIOGenerator::print( QPrinter& printer )
{
    QPainter p( &printer );

    QImage image( m_img );

    if ( ( image.width() > printer.width() ) || ( image.height() > printer.height() ) )

        image = image.scaled( printer.width(), printer.height(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation );

    p.drawImage( 0, 0, image );

    return true;
}

void KIMGIOGenerator::slotTest()
{
    kDebug() << "Test";
}

#include "generator_kimgio.moc"

