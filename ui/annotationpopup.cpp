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

AnnotationPopup::AnnotationPopup( Okular::Document *document,
                                  QWidget *parent )
    : mParent( parent ), mDocument( document )
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

    QAction *popoutWindow = 0;
    QAction *deleteNote = 0;
    QAction *showProperties = 0;
    QAction *saveAttachment = 0;
    Okular::FileAttachmentAnnotation *fileAttachAnnot = 0;

    const bool onlyOne = mAnnotations.count() == 1;

    menu.addTitle( i18np( "Annotation", "%1 Annotations", mAnnotations.count() ) );
    popoutWindow = menu.addAction( KIcon( "comment" ), i18n( "&Open Pop-up Note" ) );
    popoutWindow->setEnabled( onlyOne );
    deleteNote = menu.addAction( KIcon( "list-remove" ), i18n( "&Delete" ) );
    deleteNote->setEnabled( mDocument->isAllowed( Okular::AllowNotes ) );

    if ( onlyOne && mAnnotations.first().annotation->flags() & Okular::Annotation::DenyDelete )
        deleteNote->setEnabled( false );

    showProperties = menu.addAction( KIcon( "configure" ), i18n( "&Properties" ) );
    showProperties->setEnabled( onlyOne );

    if ( onlyOne && mAnnotations.first().annotation->subType() == Okular::Annotation::AFileAttachment )
    {
        menu.addSeparator();
        fileAttachAnnot = static_cast< Okular::FileAttachmentAnnotation * >( mAnnotations.first().annotation );
        const QString saveText = i18nc( "%1 is the name of the file to save", "&Save '%1'...", fileAttachAnnot->embeddedFile()->name() );
        saveAttachment = menu.addAction( KIcon( "document-save" ), saveText );
    }

    QAction *choice = menu.exec( point.isNull() ? QCursor::pos() : point );

    // check if the user really selected an action
    if ( choice ) {
        if ( choice == popoutWindow ) {
            emit setAnnotationWindow( mAnnotations.first().annotation  );
        } else if( choice == deleteNote ) {
            Q_FOREACH ( const AnnotPagePair& pair, mAnnotations )
            {
                if ( pair.pageNumber != -1 )
                    mDocument->removePageAnnotation( pair.pageNumber, pair.annotation );

                emit removeAnnotationWindow( pair.annotation );
            }
        } else if( choice == showProperties ) {
            if ( mAnnotations.first().pageNumber != -1 ) {
                AnnotsPropertiesDialog propdialog( mParent, mDocument, mAnnotations.first().pageNumber, mAnnotations.first().annotation );
                propdialog.exec();
            }
        } else if( choice == saveAttachment ) {
            Q_ASSERT( fileAttachAnnot );
            GuiUtils::saveEmbeddedFile( fileAttachAnnot->embeddedFile(), mParent );
        }
    }
}

#include "annotationpopup.moc"
