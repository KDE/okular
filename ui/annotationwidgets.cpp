/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qvariant.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

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
    int id = m_comboItems->findData( QVariant( icon ), Qt::UserRole, Qt::MatchFixedString );
    if ( id > -1 )
        id = m_comboItems->findText( icon, Qt::MatchFixedString );
    if ( id > -1 )
    {
        m_comboItems->setCurrentIndex( id );
    }
}

QString PixmapPreviewSelector::icon() const
{
    return m_icon;
}

void PixmapPreviewSelector::addItem( const QString& item, const QString& id )
{
    m_comboItems->addItem( item, QVariant( id ) );
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
    int id = m_comboItems->findText( icon, Qt::MatchFixedString );
    if ( id < 0 )
        return;

    m_icon = m_comboItems->itemData( id ).toString();
    QString path;
    QPixmap pixmap = KGlobal::iconLoader()->loadIcon( m_icon.toLower(), K3Icon::User, m_previewSize, K3Icon::DefaultState, &path, true );
    if ( path.isEmpty() )
        pixmap = KGlobal::iconLoader()->loadIcon( m_icon.toLower(), K3Icon::NoGroup, m_previewSize );
    m_iconLabel->setPixmap( pixmap );
}


AnnotationWidget * AnnotationWidgetFactory::widgetFor( Okular::Annotation * ann )
{
    switch ( ann->subType() )
    {
        case Okular::Annotation::AStamp:
            return new StampAnnotationWidget( ann );
            break;
        case Okular::Annotation::AText:
            return new TextAnnotationWidget( ann );
            break;
        case Okular::Annotation::ALine:
            return new LineAnnotationWidget( ann );
            break;
        // shut up gcc
        default:
            ;
    }
    // cases not covered yet
    return 0;
}


AnnotationWidget::AnnotationWidget( Okular::Annotation * ann )
    : QObject(), m_ann( ann )
{
}

AnnotationWidget::~AnnotationWidget()
{
}

Okular::Annotation::SubType AnnotationWidget::annotationType() const
{
    return m_ann->subType();
}


TextAnnotationWidget::TextAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 ), m_pixmapSelector( 0 )
{
    m_textAnn = static_cast< Okular::TextAnnotation * >( ann );
}

QWidget * TextAnnotationWidget::widget()
{
    // only Linked TextAnnotations are supported for now
    if ( m_textAnn->textType != Okular::TextAnnotation::Linked )
        return 0;

    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( m_widget );
    lay->setMargin( 0 );
    QGroupBox * gb = new QGroupBox( m_widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( "Icon" ) );
    QHBoxLayout * gblay = new QHBoxLayout( gb );
    m_pixmapSelector = new PixmapPreviewSelector( gb );
    gblay->addWidget( m_pixmapSelector );

    m_pixmapSelector->addItem( i18n( "Comment" ), "Comment" );
    m_pixmapSelector->addItem( i18n( "Help" ), "Help" );
    m_pixmapSelector->addItem( i18n( "Insert" ), "Insert" );
    m_pixmapSelector->addItem( i18n( "Key" ), "Key" );
    m_pixmapSelector->addItem( i18n( "New Paragraph" ), "NewParagraph" );
    m_pixmapSelector->addItem( i18n( "Note" ), "Note" );
    m_pixmapSelector->addItem( i18n( "Paragraph" ), "Paragraph" );
    m_pixmapSelector->setIcon( m_textAnn->textIcon );

    connect( m_pixmapSelector, SIGNAL( iconChanged( const QString& ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void TextAnnotationWidget::applyChanges()
{
    if ( m_textAnn->textType == Okular::TextAnnotation::Linked )
    {
        m_textAnn->textIcon = m_pixmapSelector->icon();
    }
}


StampAnnotationWidget::StampAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 ), m_pixmapSelector( 0 )
{
    m_stampAnn = static_cast< Okular::StampAnnotation * >( ann );
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

    // FIXME!!! use the standard names instead (when we'll have the artwork)
    m_pixmapSelector->addItem( i18n( "okular" ), "okular" );
    m_pixmapSelector->addItem( i18n( "KMenu" ), "kmenu" );
    m_pixmapSelector->addItem( i18n( "KTTSD" ), "kttsd" );
    m_pixmapSelector->addItem( i18n( "Password" ), "password" );
#if 0
    m_pixmapSelector->addItem( i18n( "Approved" ), "Approved" );
    m_pixmapSelector->addItem( i18n( "As Is" ), "AsIs" );
    m_pixmapSelector->addItem( i18n( "Confidential" ), "Confidential" );
    m_pixmapSelector->addItem( i18n( "Departmental" ), "Departmental" );
    m_pixmapSelector->addItem( i18n( "Draft" ), "Draft" );
    m_pixmapSelector->addItem( i18n( "Experimental" ), "Experimental" );
    m_pixmapSelector->addItem( i18n( "Expired" ), "Expired" );
    m_pixmapSelector->addItem( i18n( "Final" ), "Final" );
    m_pixmapSelector->addItem( i18n( "For Comment" ), "ForComment" );
    m_pixmapSelector->addItem( i18n( "For Public Release" ), "ForPublicRelease" );
    m_pixmapSelector->addItem( i18n( "Not Approved" ), "NotApproved" );
    m_pixmapSelector->addItem( i18n( "Not For Public Release" ), "NotForPublicRelease" );
    m_pixmapSelector->addItem( i18n( "Sold" ), "Sold" );
    m_pixmapSelector->addItem( i18n( "Top Secret" ), "TopSecret" );
#endif
    m_pixmapSelector->setIcon( m_stampAnn->stampIconName );
    m_pixmapSelector->setPreviewSize( 64 );

    connect( m_pixmapSelector, SIGNAL( iconChanged( const QString& ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void StampAnnotationWidget::applyChanges()
{
    m_stampAnn->stampIconName = m_pixmapSelector->icon();
}



LineAnnotationWidget::LineAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 )
{
    m_lineAnn = static_cast< Okular::LineAnnotation * >( ann );
    if ( m_lineAnn->linePoints.count() == 2 )
        m_lineType = 0; // line
    else if ( m_lineAnn->lineClosed )
        m_lineType = 1; // polygon
    else
        m_lineType = 2; // polyline
}

QWidget * LineAnnotationWidget::widget()
{
    // only lines are supported for now
    if ( m_lineType != 0 )
        return 0;

    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( m_widget );
    lay->setMargin( 0 );
    QGroupBox * gb = new QGroupBox( m_widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( " Line Extensions" ) );
    QGridLayout * gridlay = new QGridLayout( gb );
    QLabel * tmplabel = new QLabel( i18n( "Leader Line Length:" ), gb );
    gridlay->addWidget( tmplabel, 0, 0 );
    m_spinLL = new QDoubleSpinBox( gb );
    gridlay->addWidget( m_spinLL, 0, 1 );
    tmplabel->setBuddy( m_spinLL );
    tmplabel = new QLabel( i18n( "Leader Line Extensions Length:" ), gb );
    gridlay->addWidget( tmplabel, 1, 0 );
    m_spinLLE = new QDoubleSpinBox( gb );
    gridlay->addWidget( m_spinLLE, 1, 1 );
    tmplabel->setBuddy( m_spinLLE );

    m_spinLL->setRange( -500, 500 );
    m_spinLL->setValue( m_lineAnn->lineLeadingFwdPt );
    m_spinLLE->setRange( 0, 500 );
    m_spinLLE->setValue( m_lineAnn->lineLeadingBackPt );

    connect( m_spinLL, SIGNAL( valueChanged( double ) ), this, SIGNAL( dataChanged() ) );
    connect( m_spinLLE, SIGNAL( valueChanged( double ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void LineAnnotationWidget::applyChanges()
{
    if ( m_lineType == 0 )
    {
        m_lineAnn->lineLeadingFwdPt = m_spinLL->value();
        m_lineAnn->lineLeadingBackPt = m_spinLLE->value();
    }
}



#include "annotationwidgets.moc"
