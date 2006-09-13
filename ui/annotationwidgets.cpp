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
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>

// local includes
#include "annotationwidgets.h"

PixmapPreviewSelector::PixmapPreviewSelector( QWidget * parent )
  : QWidget( parent )
{
    QHBoxLayout * mainlay = new QHBoxLayout( this );
    mainlay->setMargin( 0 );
    m_comboItems = new QComboBox( this );
    mainlay->addWidget( m_comboItems );
    m_iconLabel = new QLabel( this );
    mainlay->addWidget( m_iconLabel );
    m_iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_iconLabel->setAlignment( Qt::AlignCenter );
    m_iconLabel->setFrameStyle( QFrame::StyledPanel );
    setPreviewSize( 32 );

    connect( m_comboItems, SIGNAL( currentIndexChanged( const QString& ) ), this, SLOT( iconComboChanged( const QString& ) ) );
    connect( m_comboItems, SIGNAL( currentIndexChanged( const QString& ) ), this, SIGNAL( iconChanged( const QString& ) ) );
}

PixmapPreviewSelector::~PixmapPreviewSelector()
{
}

void PixmapPreviewSelector::setIcon( const QString& icon )
{
    int id = m_comboItems->findText( icon );
    if ( id > -1 )
    {
        m_comboItems->setCurrentIndex( id );
    }
}

QString PixmapPreviewSelector::icon() const
{
    return m_icon;
}

void PixmapPreviewSelector::setItems( const QStringList& items )
{
    m_comboItems->addItems( items );
    setIcon( m_icon );
}

void PixmapPreviewSelector::setPreviewSize( int size )
{
    m_previewSize = size;
    m_iconLabel->setFixedSize( m_previewSize + 8, m_previewSize + 8 );
    iconComboChanged( m_icon );
}

int PixmapPreviewSelector::previewSize() const
{
    return m_previewSize;
}

void PixmapPreviewSelector::iconComboChanged( const QString& icon )
{
    m_icon = icon;
    QPixmap pixmap = KGlobal::iconLoader()->loadIcon( m_icon, K3Icon::NoGroup, m_previewSize );
    m_iconLabel->setPixmap( pixmap );
}


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
    : AnnotationWidget( ann ), m_widget( 0 ), m_pixmapSelector( 0 )
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
    m_pixmapSelector = new PixmapPreviewSelector( gb );
    gblay->addWidget( m_pixmapSelector );

    QStringList items;
    // FIXME!!! use the standard names instead (when we'll have the artwork)
    items
      << "okular"
      << "kmenu"
      << "kttsd"
      << "password";
#if 0
      << "Approved"
      << "AsIs"
      << "Confidential"
      << "Departmental"
      << "Draft"
      << "Experimental"
      << "Expired"
      << "Final"
      << "ForComment"
      << "ForPublicRelease"
      << "NotApproved"
      << "NotForPublicRelease"
      << "Sold"
      << "TopSecret";
#endif
    m_pixmapSelector->setItems( items );
    m_pixmapSelector->setIcon( m_stampAnn->stampIconName );
    m_pixmapSelector->setPreviewSize( 64 );

    return m_widget;
}

void StampAnnotationWidget::applyChanges()
{
    m_stampAnn->stampIconName = m_pixmapSelector->icon();
}



#include "annotationwidgets.moc"
