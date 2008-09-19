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
#include "core/annotations.h"
#include "core/document.h"

struct GuiUtilsHelper
{
    GuiUtilsHelper()
        : il( 0 )
    {
    }

    KIconLoader * il;
};

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
    text.replace( "\n", "<br>" );
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
    if ( name.startsWith( QLatin1String( "stamp-" ) ) )
    {
        QSvgRenderer r( KStandardDirs::locate( "data", QString::fromLatin1( "okular/pics/" ) + name + ".svg" ) );
        if ( r.isValid() )
        {
            QPixmap pixmap( size.isValid() ? size : r.defaultSize() );
            pixmap.fill( Qt::transparent );
            QPainter p( &pixmap );
            r.render( &p );
            p.end();
            return pixmap;
        }
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

void setIconLoader( KIconLoader * loader )
{
    s_data->il = loader;
}

KIconLoader* iconLoader()
{
    return s_data->il ? s_data->il : KIconLoader::global();
}

void saveEmbeddedFile( Okular::EmbeddedFile *ef, QWidget *parent )
{
    const QString caption = i18n( "Where do you want to save %1?", ef->name() );
    const QString path = KFileDialog::getSaveFileName( ef->name(), QString(), parent, caption );
    if ( path.isEmpty() )
        return;

    QFile f( path );
    if ( !f.exists() || KMessageBox::warningContinueCancel( parent, i18n( "A file named \"%1\" already exists. Are you sure you want to overwrite it?", path ), QString(), KGuiItem( i18nc( "@action:button", "&Overwrite" ) ) ) == KMessageBox::Continue )
    {
        if ( f.open( QIODevice::WriteOnly ) )
        {
            f.write( ef->data() );
            f.close();
        }
        else
        {
            KMessageBox::error( parent, i18n( "Could not open \"%1\" for writing. File was not saved.", path ) );
        }
    }
}

}
