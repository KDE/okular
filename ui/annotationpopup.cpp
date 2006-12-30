/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <klocale.h>
#include <kmenu.h>

#include "annotationpropertiesdialog.h"
#include "annotationpopup.h"

#include "core/annotations.h"
#include "core/document.h"

AnnotationPopup::AnnotationPopup( Okular::Annotation *annotation,
                                  Okular::Document *document,
                                  QWidget *parent )
    : mParent( parent ), mAnnotation( annotation ),
      mDocument( document ), mPageNumber( -1 )
{
}

void AnnotationPopup::setPageNumber( int pageNumber )
{
    mPageNumber = pageNumber;
}

void AnnotationPopup::exec( const QPoint &point )
{
    KMenu menu( mParent );

    QAction *popoutWindow = 0;
    QAction *deleteNote = 0;
    QAction *showProperties = 0;

    menu.addTitle( i18n( "Annotation" ) );
    popoutWindow = menu.addAction( KIcon( "comment" ), i18n( "&Open Pop-up Note" ) );
    deleteNote = menu.addAction( KIcon( "remove" ), i18n( "&Delete" ) );

    if ( mAnnotation->flags() & Okular::Annotation::DenyDelete )
        deleteNote->setEnabled( false );

    showProperties = menu.addAction( KIcon( "configure" ), i18n( "&Properties..." ) );

    QAction *choice = menu.exec( point.isNull() ? QCursor::pos() : point );

    // check if the user really selected an action
    if ( choice ) {
        if ( choice == popoutWindow ) {
            emit setAnnotationWindow( mAnnotation );
        } else if( choice == deleteNote ) {
            emit removeAnnotationWindow( mAnnotation );

            if ( mPageNumber != -1 )
                mDocument->removePageAnnotation( mPageNumber, mAnnotation );

        } else if( choice == showProperties ) {
            if ( mPageNumber != -1 ) {
                AnnotsPropertiesDialog propdialog( mParent, mDocument, mPageNumber, mAnnotation );
                propdialog.exec();
            }
        }
    }
}

#include "annotationpopup.moc"
