/***************************************************************************
 *   Copyright (C) 2006-2007 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "guiutils.h"

// qt/kde includes
#include <QPainter>
#include <QSvgRenderer>
#include <QTextDocument>
#include <QFileDialog>
#include <QStandardPaths>
#include <KIconLoader>
#include <KMessageBox>
#include <KLocalizedString>

// local includes
#include "core/action.h"
#include "core/annotations.h"
#include "core/document.h"

#include <memory>


struct GuiUtilsHelper
{
    GuiUtilsHelper()
    {
    }

    QSvgRenderer* svgStamps();

    QList<KIconLoader *> il;
    std::unique_ptr< QSvgRenderer > svgStampFile;
};

QSvgRenderer* GuiUtilsHelper::svgStamps()
{
    if ( !svgStampFile.get() )
    {
        const QString stampFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("okular/pics/stamps.svg") );
        if ( !stampFile.isEmpty() )
        {
            svgStampFile.reset( new QSvgRenderer( stampFile ) );
            if ( !svgStampFile->isValid() )
            {
                svgStampFile.reset();
            }
        }
    }
    return svgStampFile.get();
}

Q_GLOBAL_STATIC( GuiUtilsHelper, s_data )

namespace GuiUtils {

QString captionForAnnotation( const Okular::Annotation * ann )
{
    Q_ASSERT( ann );

    QString ret;
    switch( ann->subType() )
    {
        case Okular::Annotation::AText:
            if( ( (Okular::TextAnnotation*)ann )->textType() == Okular::TextAnnotation::Linked )
                ret = i18n( "Pop-up Note" );
            else
                ret = i18n( "Inline Note" );
            break;
        case Okular::Annotation::ALine:
            if( ( (Okular::LineAnnotation*)ann )->linePoints().count() == 2 )
                ret = i18n( "Straight Line" );
            else
                ret = i18n( "Polygon" );
            break;
        case Okular::Annotation::AGeom:
            ret = i18n( "Geometry" );
            break;
        case Okular::Annotation::AHighlight:
            switch ( ( (Okular::HighlightAnnotation*)ann )->highlightType() )
            {
                case Okular::HighlightAnnotation::Highlight:
                    ret = i18n( "Highlight" );
                    break;
                case Okular::HighlightAnnotation::Squiggly:
                    ret = i18n( "Squiggle" );
                    break;
                case Okular::HighlightAnnotation::Underline:
                    ret = i18n( "Underline" );
                    break;
                case Okular::HighlightAnnotation::StrikeOut:
                    ret = i18n( "Strike Out" );
                    break;
            }
            break;
        case Okular::Annotation::AStamp:
            ret = i18n( "Stamp" );
            break;
        case Okular::Annotation::AInk:
            ret = i18n( "Freehand Line" );
            break;
        case Okular::Annotation::ACaret:
            ret = i18n( "Caret" );
            break;
        case Okular::Annotation::AFileAttachment:
            ret = i18n( "File Attachment" );
            break;
        case Okular::Annotation::ASound:
            ret = i18n( "Sound" );
            break;
        case Okular::Annotation::AMovie:
            ret = i18n( "Movie" );
            break;
        case Okular::Annotation::AScreen:
            ret = i18nc( "Caption for a screen annotation", "Screen" );
            break;
        case Okular::Annotation::AWidget:
            ret = i18nc( "Caption for a widget annotation", "Widget" );
            break;
        case Okular::Annotation::ARichMedia:
            ret = i18nc( "Caption for a rich media annotation", "Rich Media" );
            break;
        case Okular::Annotation::A_BASE:
            break;
    }
    return ret;
}

QString authorForAnnotation( const Okular::Annotation * ann )
{
    Q_ASSERT( ann );

    return !ann->author().isEmpty() ? ann->author() : i18nc( "Unknown author", "Unknown" );
}

QString contentsHtml( const Okular::Annotation * ann )
{
    QString text = ann->contents().toHtmlEscaped();
    text.replace( QLatin1Char('\n'), QLatin1String("<br>") );
    return text;
}

QString prettyToolTip( const Okular::Annotation * ann )
{
    Q_ASSERT( ann );

    QString author = authorForAnnotation( ann );
    QString contents = contentsHtml( ann );

    QString tooltip = QStringLiteral( "<qt><b>" ) + i18n( "Author: %1", author ) + QStringLiteral( "</b>" );
    if ( !contents.isEmpty() )
        tooltip += QStringLiteral( "<div style=\"font-size: 4px;\"><hr /></div>" ) + contents;

    tooltip += QLatin1String("</qt>");

    return tooltip;
}

QPixmap loadStamp( const QString& _name, const QSize& size, int iconSize )
{
    const QString name = _name.toLower();
    QSvgRenderer * r = 0;
    if ( ( r = s_data->svgStamps() ) && r->elementExists( name ) )
    {
        const QRectF stampElemRect = r->boundsOnElement( name );
        const QRectF stampRect( size.isValid() ? QRectF( QPointF( 0, 0 ), size ) : stampElemRect );
        QPixmap pixmap( stampRect.size().toSize() );
        pixmap.fill( Qt::transparent );
        QPainter p( &pixmap );
        r->render( &p, name );
        p.end();
        return pixmap;
    }
    QPixmap pixmap;
    const KIconLoader * il = iconLoader();
    QString path;
    const int minSize = iconSize > 0 ? iconSize : qMin( size.width(), size.height() );
    pixmap = il->loadIcon( name, KIconLoader::User, minSize, KIconLoader::DefaultState, QStringList(), &path, true );
    if ( path.isEmpty() )
        pixmap = il->loadIcon( name, KIconLoader::NoGroup, minSize );
    return pixmap;
}

void addIconLoader( KIconLoader * loader )
{
    s_data->il.append( loader );
}

void removeIconLoader( KIconLoader * loader )
{
    s_data->il.removeAll( loader );
}

KIconLoader* iconLoader()
{
    return s_data->il.isEmpty() ? KIconLoader::global() : s_data->il.back();
}

void saveEmbeddedFile( Okular::EmbeddedFile *ef, QWidget *parent )
{
    const QString caption = i18n( "Where do you want to save %1?", ef->name() );
    const QString path = QFileDialog::getSaveFileName( parent, caption, ef->name() );
    if ( path.isEmpty() )
        return;
    QFile targetFile( path );
    writeEmbeddedFile( ef, parent, targetFile );
}

void writeEmbeddedFile( Okular::EmbeddedFile *ef, QWidget *parent, QFile& target ) {
    if ( !target.open( QIODevice::WriteOnly ) )
    {
        KMessageBox::error( parent, i18n( "Could not open \"%1\" for writing. File was not saved.", target.fileName() ) );
        return;
    }
    target.write( ef->data() );
    target.close();
}

Okular::Movie* renditionMovieFromScreenAnnotation( const Okular::ScreenAnnotation *annotation )
{
    if ( !annotation )
        return 0;

    if ( annotation->action() && annotation->action()->actionType() == Okular::Action::Rendition )
    {
        Okular::RenditionAction *renditionAction = static_cast< Okular::RenditionAction * >( annotation->action() );
        return renditionAction->movie();
    }

    return 0;
}

// from Arthur - qt4
static inline int qt_div_255(int x) { return (x + (x>>8) + 0x80) >> 8; }

void colorizeImage( QImage & grayImage, const QColor & color, unsigned int destAlpha )
{
    // Make sure that the image is Format_ARGB32_Premultiplied
    if ( grayImage.format() != QImage::Format_ARGB32_Premultiplied )
        grayImage = grayImage.convertToFormat( QImage::Format_ARGB32_Premultiplied );

    // iterate over all pixels changing the alpha component value
    unsigned int * data = (unsigned int *)grayImage.bits();
    unsigned int pixels = grayImage.width() * grayImage.height();
    int red = color.red(),
        green = color.green(),
        blue = color.blue();

    int source, sourceSat, sourceAlpha;
    for( unsigned int i = 0; i < pixels; ++i )
    {   // optimize this loop keeping byte order into account
        source = data[i];
        sourceSat = qRed( source );
        int newR = qt_div_255( sourceSat * red ),
            newG = qt_div_255( sourceSat * green ),
            newB = qt_div_255( sourceSat * blue );
        if ( (sourceAlpha = qAlpha( source )) == 255 )
        {
            // use destAlpha
            data[i] = qRgba( newR, newG, newB, destAlpha );
        }
        else
        {
            // use destAlpha * sourceAlpha product
            if ( destAlpha < 255 )
                sourceAlpha = qt_div_255( destAlpha * sourceAlpha );
            data[i] = qRgba( newR, newG, newB, sourceAlpha );
        }
    }
}

}
