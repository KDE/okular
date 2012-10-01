/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationpopup.h"

#include <klocale.h>
#include <kmenu.h>

#include "annotationpropertiesdialog.h"

#include "core/annotations.h"
#include "core/document.h"
#include "guiutils.h"

Q_DECLARE_METATYPE( AnnotationPopup::AnnotPagePair )

AnnotationPopup::AnnotationPopup( Okular::Document *document, MenuMode mode,
                                  QWidget *parent )
    : mParent( parent ), mDocument( document ), mMenuMode( mode )
{
}

void AnnotationPopup::addAnnotation( Okular::Annotation* annotation, int pageNumber )
{
    AnnotPagePair pair( annotation, pageNumber );
    if ( !mAnnotations.contains( pair ) )
      mAnnotations.append( pair );
}

void AnnotationPopup::exec( const QPoint &point )
{
    if ( mAnnotations.isEmpty() )
        return;

    KMenu menu( mParent );

    QAction *action = 0;
    Okular::FileAttachmentAnnotation *fileAttachAnnot = 0;

    const char *actionTypeId = "actionType";

    const QString openId = QString::fromLatin1( "open" );
    const QString deleteId = QString::fromLatin1( "delete" );
    const QString deleteAllId = QString::fromLatin1( "deleteAll" );
    const QString propertiesId = QString::fromLatin1( "properties" );
    const QString saveId = QString::fromLatin1( "save" );

    if ( mMenuMode == SingleAnnotationMode )
    {
        const bool onlyOne = (mAnnotations.count() == 1);

        const AnnotPagePair &pair = mAnnotations.at(0);

        menu.addTitle( i18np( "Annotation", "%1 Annotations", mAnnotations.count() ) );

        action = menu.addAction( KIcon( "comment" ), i18n( "&Open Pop-up Note" ) );
        action->setData( QVariant::fromValue( pair ) );
        action->setEnabled( onlyOne );
        action->setProperty( actionTypeId, openId );

        action = menu.addAction( KIcon( "list-remove" ), i18n( "&Delete" ) );
        action->setEnabled( mDocument->isAllowed( Okular::AllowNotes ) );
        action->setProperty( actionTypeId, deleteAllId );

        foreach ( const AnnotPagePair& pair, mAnnotations )
        {
            if ( !mDocument->canRemovePageAnnotation( pair.annotation ) )
                action->setEnabled( false );
        }

        action = menu.addAction( KIcon( "configure" ), i18n( "&Properties" ) );
        action->setData( QVariant::fromValue( pair ) );
        action->setEnabled( onlyOne );
        action->setProperty( actionTypeId, propertiesId );

        if ( onlyOne && pair.annotation->subType() == Okular::Annotation::AFileAttachment )
        {
            menu.addSeparator();
            fileAttachAnnot = static_cast< Okular::FileAttachmentAnnotation * >( pair.annotation );
            const QString saveText = i18nc( "%1 is the name of the file to save", "&Save '%1'...", fileAttachAnnot->embeddedFile()->name() );

            action = menu.addAction( KIcon( "document-save" ), saveText );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, saveId );
        }
    }
    else
    {
        foreach ( const AnnotPagePair& pair, mAnnotations )
        {
            menu.addTitle( GuiUtils::captionForAnnotation( pair.annotation ) );

            action = menu.addAction( KIcon( "comment" ), i18n( "&Open Pop-up Note" ) );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, openId );

            action = menu.addAction( KIcon( "list-remove" ), i18n( "&Delete" ) );
            action->setEnabled( mDocument->isAllowed( Okular::AllowNotes ) &&
                                mDocument->canRemovePageAnnotation( pair.annotation ) );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, deleteId );

            action = menu.addAction( KIcon( "configure" ), i18n( "&Properties" ) );
            action->setData( QVariant::fromValue( pair ) );
            action->setProperty( actionTypeId, propertiesId );

            if ( pair.annotation->subType() == Okular::Annotation::AFileAttachment )
            {
                menu.addSeparator();
                fileAttachAnnot = static_cast< Okular::FileAttachmentAnnotation * >( pair.annotation );
                const QString saveText = i18nc( "%1 is the name of the file to save", "&Save '%1'...", fileAttachAnnot->embeddedFile()->name() );

                action = menu.addAction( KIcon( "document-save" ), saveText );
                action->setData( QVariant::fromValue( pair ) );
                action->setProperty( actionTypeId, saveId );
            }
        }
    }

    QAction *choice = menu.exec( point.isNull() ? QCursor::pos() : point );

    // check if the user really selected an action
    if ( choice ) {
        const AnnotPagePair pair = choice->data().value<AnnotPagePair>();

        const QString actionType = choice->property( actionTypeId ).toString();
        if ( actionType == openId ) {
            emit openAnnotationWindow( pair.annotation, pair.pageNumber );
        } else if( actionType == deleteId ) {
            if ( pair.pageNumber != -1 )
                mDocument->removePageAnnotation( pair.pageNumber, pair.annotation );
        } else if( actionType == deleteAllId ) {
            Q_FOREACH ( const AnnotPagePair& pair, mAnnotations )
            {
                if ( pair.pageNumber != -1 )
                    mDocument->removePageAnnotation( pair.pageNumber, pair.annotation );
            }
        } else if( actionType == propertiesId ) {
            if ( pair.pageNumber != -1 ) {
                AnnotsPropertiesDialog propdialog( mParent, mDocument, pair.pageNumber, pair.annotation );
                propdialog.exec();
            }
        } else if( actionType == saveId ) {
            const Okular::FileAttachmentAnnotation * fileAttachAnnot = static_cast< Okular::FileAttachmentAnnotation * >( pair.annotation );
            GuiUtils::saveEmbeddedFile( fileAttachAnnot->embeddedFile(), mParent );
        }
    }
}

#include "annotationpopup.moc"
