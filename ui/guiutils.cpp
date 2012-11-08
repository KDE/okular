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
#include <qpainter.h>
#include <qsvgrenderer.h>
#include <qtextdocument.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

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
    std::auto_ptr< QSvgRenderer > svgStampFile;
};

QSvgRenderer* GuiUtilsHelper::svgStamps()
{
    if ( !svgStampFile.get() )
    {
        const QString stampFile = KStandardDirs::locate( "data", "okular/pics/stamps.svg" );
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

K_GLOBAL_STATIC( GuiUtilsHelper, s_data )

namespace GuiUtils {

QString captionForAnnotation( const Okular::Annotation * ann )
{
    Q_ASSERT( ann );

    QString ret;
    switch( ann->subType() )
    {
        case Okular::Annotation::AText:
            if( ( (Okular::TextAnnotation*)ann )->textType() == Okular::TextAnnotation::Linked )
                ret = i18n( "Note" );
            else
                ret = i18n( "Inline Note" );
            break;
        case Okular::Annotation::ALine:
            ret = i18n( "Line" );
            break;
        case Okular::Annotation::AGeom:
            ret = i18n( "Geometry" );
            break;
        case Okular::Annotation::AHighlight:
            ret = i18n( "Highlight" );
            break;
        case Okular::Annotation::AStamp:
            ret = i18n( "Stamp" );
            break;
        case Okular::Annotation::AInk:
            ret = i18n( "Ink" );
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

QString contents( const Okular::Annotation * ann )
{
    Q_ASSERT( ann );

    // 1. window text
    QString ret = ann->window().text();
    if ( !ret.isEmpty() )
        return ret;
    // 2. if Text and InPlace, the inplace text
    if ( ann->subType() == Okular::Annotation::AText )
    {
        const Okular::TextAnnotation * txtann = static_cast< const Okular::TextAnnotation * >( ann );
        if ( txtann->textType() == Okular::TextAnnotation::InPlace )
        {
            ret = txtann->inplaceText();
            if ( !ret.isEmpty() )
                return ret;
        }
    }

    // 3. contents
    ret = ann->contents();

    return ret;
}

QString contentsHtml( const Okular::Annotation * ann )
{
    QString text = Qt::escape( contents( ann ) );
    text.replace( '\n', "<br>" );
    return text;
}

QString prettyToolTip( const Okular::Annotation * ann )
{
    Q_ASSERT( ann );

    QString author = authorForAnnotation( ann );
    QString contents = contentsHtml( ann );

    QString tooltip = QString( "<qt><b>" ) + i18n( "Author: %1", author ) + QString( "</b>" );
    if ( !contents.isEmpty() )
        tooltip += QString( "<div style=\"font-size: 4px;\"><hr /></div>" ) + contents;

    tooltip += "</qt>";

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
    const QString path = KFileDialog::getSaveFileName( ef->name(), QString(), parent, caption,
                                                       KFileDialog::ConfirmOverwrite );
    if ( path.isEmpty() )
        return;

    QFile f( path );
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        KMessageBox::error( parent, i18n( "Could not open \"%1\" for writing. File was not saved.", path ) );
        return;
    }
    f.write( ef->data() );
    f.close();
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

}
