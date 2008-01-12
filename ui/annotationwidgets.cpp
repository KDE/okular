/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationwidgets.h"

// qt/kde includes
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qvariant.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kfontrequester.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

#include "guiutils.h"

PixmapPreviewSelector::PixmapPreviewSelector( QWidget * parent )
  : QWidget( parent )
{
    QHBoxLayout * mainlay = new QHBoxLayout( this );
    mainlay->setMargin( 0 );
    m_comboItems = new KComboBox( this );
    mainlay->addWidget( m_comboItems );
    m_iconLabel = new QLabel( this );
    mainlay->addWidget( m_iconLabel );
    m_iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_iconLabel->setAlignment( Qt::AlignCenter );
    m_iconLabel->setFrameStyle( QFrame::StyledPanel );
    setPreviewSize( 32 );

    connect( m_comboItems, SIGNAL( currentIndexChanged( const QString& ) ), this, SLOT( iconComboChanged( const QString& ) ) );
    connect( m_comboItems, SIGNAL( editTextChanged( const QString& ) ), this, SLOT( iconComboChanged( const QString& ) ) );
}

PixmapPreviewSelector::~PixmapPreviewSelector()
{
}

void PixmapPreviewSelector::setIcon( const QString& icon )
{
    int id = m_comboItems->findData( QVariant( icon ), Qt::UserRole, Qt::MatchFixedString );
    if ( id == -1 )
        id = m_comboItems->findText( icon, Qt::MatchFixedString );
    if ( id > -1 )
    {
        m_comboItems->setCurrentIndex( id );
    }
    else if ( m_comboItems->isEditable() )
    {
        m_comboItems->addItem( icon, QVariant( icon ) );
        m_comboItems->setCurrentIndex( m_comboItems->findText( icon, Qt::MatchFixedString ) );
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

void PixmapPreviewSelector::setEditable( bool editable )
{
    m_comboItems->setEditable( editable );
}

void PixmapPreviewSelector::iconComboChanged( const QString& icon )
{
    int id = m_comboItems->findText( icon, Qt::MatchFixedString );
    if ( id >= 0 )
    {
        m_icon = m_comboItems->itemData( id ).toString();
    }
    else
    {
        m_icon = icon;
    }

    QString path;
    QPixmap pixmap = GuiUtils::iconLoader()->loadIcon( m_icon.toLower(), KIconLoader::User, m_previewSize, KIconLoader::DefaultState, QStringList(), &path, true );
    if ( path.isEmpty() )
        pixmap = GuiUtils::iconLoader()->loadIcon( m_icon.toLower(), KIconLoader::NoGroup, m_previewSize );
    m_iconLabel->setPixmap( pixmap );

    emit iconChanged( m_icon );
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
        case Okular::Annotation::AHighlight:
            return new HighlightAnnotationWidget( ann );
            break;
        case Okular::Annotation::AGeom:
            return new GeomAnnotationWidget( ann );
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
    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( m_widget );
    lay->setMargin( 0 );

    if ( m_textAnn->textType() == Okular::TextAnnotation::Linked )
    {
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
    m_pixmapSelector->setIcon( m_textAnn->textIcon() );

    connect( m_pixmapSelector, SIGNAL( iconChanged( const QString& ) ), this, SIGNAL( dataChanged() ) );
    }

    QHBoxLayout * fontlay = new QHBoxLayout();
    QLabel * tmplabel = new QLabel( i18n( "Font:" ), m_widget );
    fontlay->addWidget( tmplabel );
    m_fontReq = new KFontRequester( m_widget );
    fontlay->addWidget( m_fontReq );
    lay->addLayout( fontlay );

    m_fontReq->setFont( m_textAnn->textFont() );

    connect( m_fontReq, SIGNAL( fontSelected( const QFont& ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void TextAnnotationWidget::applyChanges()
{
    if ( m_textAnn->textType() == Okular::TextAnnotation::Linked )
    {
        m_textAnn->setTextIcon( m_pixmapSelector->icon() );
    }
    m_textAnn->setTextFont( m_fontReq->font() );
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
    m_pixmapSelector->setEditable( true );

    // FIXME!!! use the standard names instead (when we'll have the artwork)
    m_pixmapSelector->addItem( i18n( "okular" ), "graphics-viewer-document" );
    m_pixmapSelector->addItem( i18n( "Bookmark" ), "bookmarks" );
    m_pixmapSelector->addItem( i18n( "KDE" ), "kde" );
    m_pixmapSelector->addItem( i18n( "Information" ), "help-about" );
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
    m_pixmapSelector->setIcon( m_stampAnn->stampIconName() );
    m_pixmapSelector->setPreviewSize( 64 );

    connect( m_pixmapSelector, SIGNAL( iconChanged( const QString& ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void StampAnnotationWidget::applyChanges()
{
    m_stampAnn->setStampIconName( m_pixmapSelector->icon() );
}



LineAnnotationWidget::LineAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 )
{
    m_lineAnn = static_cast< Okular::LineAnnotation * >( ann );
    if ( m_lineAnn->linePoints().count() == 2 )
        m_lineType = 0; // line
    else if ( m_lineAnn->lineClosed() )
        m_lineType = 1; // polygon
    else
        m_lineType = 2; // polyline
}

QWidget * LineAnnotationWidget::widget()
{
    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( m_widget );
    lay->setMargin( 0 );
    if ( m_lineType == 0 )
    {
    QGroupBox * gb = new QGroupBox( m_widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( "Line Extensions" ) );
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
    }

    QGroupBox * gb2 = new QGroupBox( m_widget );
    lay->addWidget( gb2 );
    gb2->setTitle( i18n( "Style" ) );
    QGridLayout * gridlay2 = new QGridLayout( gb2 );
    QLabel * tmplabel2 = new QLabel( i18n( "&Size:" ), gb2 );
    gridlay2->addWidget( tmplabel2, 0, 0 );
    m_spinSize = new QDoubleSpinBox( gb2 );
    gridlay2->addWidget( m_spinSize, 0, 1 );
    tmplabel2->setBuddy( m_spinSize );

    if ( m_lineType == 0 )
    {
    m_spinLL->setRange( -500, 500 );
    m_spinLL->setValue( m_lineAnn->lineLeadingForwardPoint() );
    m_spinLLE->setRange( 0, 500 );
    m_spinLLE->setValue( m_lineAnn->lineLeadingBackwardPoint() );
    }
    m_spinSize->setRange( 1, 100 );
    m_spinSize->setValue( m_lineAnn->style().width() );

    if ( m_lineType == 0 )
    {
    connect( m_spinLL, SIGNAL( valueChanged( double ) ), this, SIGNAL( dataChanged() ) );
    connect( m_spinLLE, SIGNAL( valueChanged( double ) ), this, SIGNAL( dataChanged() ) );
    }
    connect( m_spinSize, SIGNAL( valueChanged( double ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void LineAnnotationWidget::applyChanges()
{
    if ( m_lineType == 0 )
    {
        m_lineAnn->setLineLeadingForwardPoint( m_spinLL->value() );
        m_lineAnn->setLineLeadingBackwardPoint( m_spinLLE->value() );
    }
    m_lineAnn->style().setWidth( m_spinSize->value() );
}



HighlightAnnotationWidget::HighlightAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 )
{
    m_hlAnn = static_cast< Okular::HighlightAnnotation * >( ann );
}

QWidget * HighlightAnnotationWidget::widget()
{
    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( m_widget );
    lay->setMargin( 0 );
    QHBoxLayout * typelay = new QHBoxLayout();
    lay->addLayout( typelay );
    QLabel * tmplabel = new QLabel( i18n( "Type:" ), m_widget );
    typelay->addWidget( tmplabel );
    m_typeCombo = new KComboBox( m_widget );
    tmplabel->setBuddy( m_typeCombo );
    typelay->addWidget( m_typeCombo );

    m_typeCombo->addItem( i18n( "Highlight" ) );
    m_typeCombo->addItem( i18n( "Squiggly" ) );
    m_typeCombo->addItem( i18n( "Underline" ) );
    m_typeCombo->addItem( i18n( "Strike out" ) );
    m_typeCombo->setCurrentIndex( m_hlAnn->highlightType() );

    connect( m_typeCombo, SIGNAL( currentIndexChanged ( int ) ), this, SIGNAL( dataChanged() ) );

    return m_widget;
}

void HighlightAnnotationWidget::applyChanges()
{
    m_hlAnn->setHighlightType( (Okular::HighlightAnnotation::HighlightType)m_typeCombo->currentIndex() );
}



GeomAnnotationWidget::GeomAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_widget( 0 )
{
    m_geomAnn = static_cast< Okular::GeomAnnotation * >( ann );
}

QWidget * GeomAnnotationWidget::widget()
{
    if ( m_widget )
        return m_widget;

    m_widget = new QWidget();
    QGridLayout * lay = new QGridLayout( m_widget );
    lay->setMargin( 0 );
    QLabel * tmplabel = new QLabel( i18n( "Type:" ), m_widget );
    lay->addWidget( tmplabel, 0, 0 );
    m_typeCombo = new KComboBox( m_widget );
    tmplabel->setBuddy( m_typeCombo );
    lay->addWidget( m_typeCombo, 0, 1 );
    m_useColor = new QCheckBox( i18n( "Inner color:" ), m_widget );
    lay->addWidget( m_useColor, 1, 0 );
    m_innerColor = new KColorButton( m_widget );
    lay->addWidget( m_innerColor, 1, 1 );

    m_typeCombo->addItem( i18n( "Rectangle" ) );
    m_typeCombo->addItem( i18n( "Ellipse" ) );
    m_typeCombo->setCurrentIndex( m_geomAnn->geometricalType() );
    m_innerColor->setColor( m_geomAnn->geometricalInnerColor() );
    if ( m_geomAnn->geometricalInnerColor().isValid() )
    {
        m_useColor->setChecked( true );
    }
    else
    {
        m_innerColor->setEnabled( false );
    }

    connect( m_typeCombo, SIGNAL( currentIndexChanged ( int ) ), this, SIGNAL( dataChanged() ) );
    connect( m_innerColor, SIGNAL( changed( const QColor & ) ), this, SIGNAL( dataChanged() ) );
    connect( m_useColor, SIGNAL( toggled( bool ) ), this, SIGNAL( dataChanged() ) );
    connect( m_useColor, SIGNAL( toggled( bool ) ), m_innerColor, SLOT( setEnabled( bool ) ) );

    return m_widget;
}

void GeomAnnotationWidget::applyChanges()
{
    m_geomAnn->setGeometricalType( (Okular::GeomAnnotation::GeomType)m_typeCombo->currentIndex() );
    if ( !m_useColor->isChecked() )
    {
        m_geomAnn->setGeometricalInnerColor( QColor() );
    }
    else
    {
        m_geomAnn->setGeometricalInnerColor( m_innerColor->color() );
    }
}




#include "annotationwidgets.moc"
