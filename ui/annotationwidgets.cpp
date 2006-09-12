/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
/*
#include <qframe.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qheaderview.h>
#include <kcolorbutton.h>
#include <kicon.h>
#include <knuminput.h>
*/
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qwidget.h>
#include <kiconloader.h>
#include <klocale.h>

// local includes
#include "annotationwidgets.h"

AnnotationWidget * AnnotationWidgetFactory::widgetFor( Annotation * ann )
{
    switch ( ann->subType() )
    {
        case Annotation::AStamp:
            return new StampAnnotationWidget( ann );
            break;
        // shut up gcc
        default:
            ;
    }
    // cases not covered yet
    return 0;
}


AnnotationWidget::AnnotationWidget( Annotation * ann )
    : QObject(), m_ann( ann )
{
}

AnnotationWidget::~AnnotationWidget()
{
}

Annotation::SubType AnnotationWidget::annotationType()
{
    return m_ann->subType();
}


StampAnnotationWidget::StampAnnotationWidget( Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 ), m_iconLabel( 0 )
{
    m_stampAnn = static_cast< StampAnnotation * >( ann );
}

QWidget * StampAnnotationWidget::widget()
{
    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( m_widget );
    lay->setMargin( 0 );
    QGroupBox * gb = new QGroupBox( m_widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( "Stamp Symbol" ) );
    QHBoxLayout * gblay = new QHBoxLayout( gb );
    QComboBox * cb = new QComboBox( gb );
    gblay->addWidget( cb );
    m_iconLabel = new QLabel( gb );
    gblay->addWidget( m_iconLabel );
    m_iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_iconLabel->setFixedSize( 40, 40 );
    m_iconLabel->setAlignment( Qt::AlignCenter );
    m_iconLabel->setFrameStyle( QFrame::StyledPanel );

    // FIXME!!! use the standard names instead (when we'll have the artwork)
    cb->addItem( "okular" );
    cb->addItem( "kmenu" );
    cb->addItem( "kttsd" );
    cb->addItem( "password" );
    cb->addItem( "Approved" );
#if 0
    cb->addItem( "AsIs" );
    cb->addItem( "Confidential" );
    cb->addItem( "Departmental" );
    cb->addItem( "Draft" );
    cb->addItem( "Experimental" );
    cb->addItem( "Expired" );
    cb->addItem( "Final" );
    cb->addItem( "ForComment" );
    cb->addItem( "ForPublicRelease" );
    cb->addItem( "NotApproved" );
    cb->addItem( "NotForPublicRelease" );
    cb->addItem( "Sold" );
    cb->addItem( "TopSecret" );
#endif

    connect( cb, SIGNAL( currentIndexChanged( const QString& ) ), this, SLOT( iconChanged( const QString& ) ) );

    // fire the event manually
    iconChanged( m_stampAnn->stampIconName );
    int id = cb->findText( m_currentIcon );
    if ( id > -1 )
        cb->setCurrentIndex( id );

    return m_widget;
}

void StampAnnotationWidget::applyChanges()
{
    m_stampAnn->stampIconName = m_currentIcon;
}

void StampAnnotationWidget::iconChanged( const QString& icon )
{
    if ( !m_iconLabel ) return;

    m_currentIcon = icon;
    QPixmap pixmap = DesktopIcon( icon, 32 );
    m_iconLabel->setPixmap( pixmap );
}


#include "annotationwidgets.moc"
