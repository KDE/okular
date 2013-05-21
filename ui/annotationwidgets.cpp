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
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kdebug.h>

#include "core/document.h"
#include "guiutils.h"

#define FILEATTACH_ICONSIZE 48

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

    connect( m_comboItems, SIGNAL(currentIndexChanged(QString)), this, SLOT(iconComboChanged(QString)) );
    connect( m_comboItems, SIGNAL(editTextChanged(QString)), this, SLOT(iconComboChanged(QString)) );
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

    QPixmap pixmap = GuiUtils::loadStamp( m_icon, QSize(), m_previewSize );
    const QRect cr = m_iconLabel->contentsRect();
    if ( pixmap.width() > cr.width() || pixmap.height() > cr.height() )
        pixmap = pixmap.scaled( cr.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );
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
        case Okular::Annotation::AInk:
            return new InkAnnotationWidget( ann );
            break;
        case Okular::Annotation::AGeom:
            return new GeomAnnotationWidget( ann );
            break;
        case Okular::Annotation::AFileAttachment:
            return new FileAttachmentAnnotationWidget( ann );
            break;
        case Okular::Annotation::ACaret:
            return new CaretAnnotationWidget( ann );
            break;
        // shut up gcc
        default:
            ;
    }
    // cases not covered yet: return a generic widget
    return new AnnotationWidget( ann );
}


AnnotationWidget::AnnotationWidget( Okular::Annotation * ann )
    : QObject(), m_ann( ann ), m_appearanceWidget( 0 ), m_extraWidget( 0 )
{
}

AnnotationWidget::~AnnotationWidget()
{
}

Okular::Annotation::SubType AnnotationWidget::annotationType() const
{
    return m_ann->subType();
}

QWidget * AnnotationWidget::appearanceWidget()
{
    if ( m_appearanceWidget )
        return m_appearanceWidget;

    m_appearanceWidget = createAppearanceWidget();
    return m_appearanceWidget;
}

QWidget * AnnotationWidget::extraWidget()
{
    if ( m_extraWidget )
        return m_extraWidget;

    m_extraWidget = createExtraWidget();
    return m_extraWidget;
}

void AnnotationWidget::applyChanges()
{
    m_ann->style().setColor( m_colorBn->color() );
    m_ann->style().setOpacity( (double)m_opacity->value() / 100.0 );
}

QWidget * AnnotationWidget::createAppearanceWidget()
{
    QWidget * widget = new QWidget();
    QGridLayout * gridlayout = new QGridLayout( widget );

    QLabel * tmplabel = new QLabel( i18n( "&Color:" ), widget );
    gridlayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    m_colorBn = new KColorButton( widget );
    m_colorBn->setColor( m_ann->style().color() );
    tmplabel->setBuddy( m_colorBn );
    gridlayout->addWidget( m_colorBn, 0, 1 );

    tmplabel = new QLabel( i18n( "&Opacity:" ), widget );
    gridlayout->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    m_opacity = new KIntNumInput( widget );
    m_opacity->setRange( 0, 100 );
    m_opacity->setValue( (int)( m_ann->style().opacity() * 100 ) );
    m_opacity->setSuffix( i18nc( "Suffix for the opacity level, eg '80 %'", " %" ) );
    tmplabel->setBuddy( m_opacity );
    gridlayout->addWidget( m_opacity, 1, 1 );

    QWidget * styleWidget = createStyleWidget();
    if ( styleWidget )
        gridlayout->addWidget( styleWidget, 2, 0, 1, 2 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding ), 3, 0 );

    connect( m_colorBn, SIGNAL(changed(QColor)), this, SIGNAL(dataChanged()) );
    connect( m_opacity, SIGNAL(valueChanged(int)), this, SIGNAL(dataChanged()) );

    return widget;
}

QWidget * AnnotationWidget::createStyleWidget()
{
    return 0;
}

QWidget * AnnotationWidget::createExtraWidget()
{
    return 0;
}


TextAnnotationWidget::TextAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_pixmapSelector( 0 )
{
    m_textAnn = static_cast< Okular::TextAnnotation * >( ann );
}

QWidget * TextAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );

    if ( m_textAnn->textType() == Okular::TextAnnotation::Linked )
    {
        QGroupBox * gb = new QGroupBox( widget );
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

        connect( m_pixmapSelector, SIGNAL(iconChanged(QString)), this, SIGNAL(dataChanged()) );
    }
    else if ( m_textAnn->textType() == Okular::TextAnnotation::InPlace )
    {
        QGridLayout * innerlay = new QGridLayout();
        lay->addLayout( innerlay );

        QLabel * tmplabel = new QLabel( i18n( "Font:" ), widget );
        innerlay->addWidget( tmplabel, 0, 0 );
        m_fontReq = new KFontRequester( widget );
        innerlay->addWidget( m_fontReq, 0, 1 );
        m_fontReq->setFont( m_textAnn->textFont() );

        tmplabel = new QLabel( i18n( "Align:" ), widget );
        innerlay->addWidget( tmplabel, 1, 0 );
        m_textAlign = new KComboBox( widget );
        innerlay->addWidget( m_textAlign, 1, 1 );
        m_textAlign->addItem( i18n("Left") );
        m_textAlign->addItem( i18n("Center") );
        m_textAlign->addItem( i18n("Right") );
        m_textAlign->setCurrentIndex( m_textAnn->inplaceAlignment() );

        connect( m_fontReq, SIGNAL(fontSelected(QFont)), this, SIGNAL(dataChanged()) );
        connect( m_textAlign, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dataChanged()) );
    }

    return widget;
}

void TextAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    if ( m_textAnn->textType() == Okular::TextAnnotation::Linked )
    {
        m_textAnn->setTextIcon( m_pixmapSelector->icon() );
    }
    else if ( m_textAnn->textType() == Okular::TextAnnotation::InPlace )
    {
        m_textAnn->setTextFont( m_fontReq->font() );
        m_textAnn->setInplaceAlignment( m_textAlign->currentIndex() );
    }
}


StampAnnotationWidget::StampAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_pixmapSelector( 0 )
{
    m_stampAnn = static_cast< Okular::StampAnnotation * >( ann );
}

QWidget * StampAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );
    QGroupBox * gb = new QGroupBox( widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( "Stamp Symbol" ) );
    QHBoxLayout * gblay = new QHBoxLayout( gb );
    m_pixmapSelector = new PixmapPreviewSelector( gb );
    gblay->addWidget( m_pixmapSelector );
    m_pixmapSelector->setEditable( true );

    m_pixmapSelector->addItem( i18n( "Okular" ), "okular" );
    m_pixmapSelector->addItem( i18n( "Bookmark" ), "bookmarks" );
    m_pixmapSelector->addItem( i18n( "KDE" ), "kde" );
    m_pixmapSelector->addItem( i18n( "Information" ), "help-about" );
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
    m_pixmapSelector->setIcon( m_stampAnn->stampIconName() );
    m_pixmapSelector->setPreviewSize( 64 );

    connect( m_pixmapSelector, SIGNAL(iconChanged(QString)), this, SIGNAL(dataChanged()) );

    return widget;
}

void StampAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_stampAnn->setStampIconName( m_pixmapSelector->icon() );
}



LineAnnotationWidget::LineAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann )
{
    m_lineAnn = static_cast< Okular::LineAnnotation * >( ann );
    if ( m_lineAnn->linePoints().count() == 2 )
        m_lineType = 0; // line
    else if ( m_lineAnn->lineClosed() )
        m_lineType = 1; // polygon
    else
        m_lineType = 2; // polyline
}

QWidget * LineAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );
    if ( m_lineType == 0 )
    {
        QGroupBox * gb = new QGroupBox( widget );
        lay->addWidget( gb );
        gb->setTitle( i18n( "Line Extensions" ) );
        QGridLayout * gridlay = new QGridLayout( gb );
        QLabel * tmplabel = new QLabel( i18n( "Leader Line Length:" ), gb );
        gridlay->addWidget( tmplabel, 0, 0, Qt::AlignRight );
        m_spinLL = new QDoubleSpinBox( gb );
        gridlay->addWidget( m_spinLL, 0, 1 );
        tmplabel->setBuddy( m_spinLL );
        tmplabel = new QLabel( i18n( "Leader Line Extensions Length:" ), gb );
        gridlay->addWidget( tmplabel, 1, 0, Qt::AlignRight );
        m_spinLLE = new QDoubleSpinBox( gb );
        gridlay->addWidget( m_spinLLE, 1, 1 );
        tmplabel->setBuddy( m_spinLLE );
    }

    QGroupBox * gb2 = new QGroupBox( widget );
    lay->addWidget( gb2 );
    gb2->setTitle( i18n( "Style" ) );
    QGridLayout * gridlay2 = new QGridLayout( gb2 );
    QLabel * tmplabel2 = new QLabel( i18n( "&Size:" ), gb2 );
    gridlay2->addWidget( tmplabel2, 0, 0, Qt::AlignRight );
    m_spinSize = new QDoubleSpinBox( gb2 );
    gridlay2->addWidget( m_spinSize, 0, 1 );
    tmplabel2->setBuddy( m_spinSize );

    if ( m_lineType == 1 )
    {
        m_useColor = new QCheckBox( i18n( "Inner color:" ), gb2 );
        gridlay2->addWidget( m_useColor, 1, 0 );
        m_innerColor = new KColorButton( gb2 );
        gridlay2->addWidget( m_innerColor, 1, 1 );
    }

    if ( m_lineType == 0 )
    {
        m_spinLL->setRange( -500, 500 );
        m_spinLL->setValue( m_lineAnn->lineLeadingForwardPoint() );
        m_spinLLE->setRange( 0, 500 );
        m_spinLLE->setValue( m_lineAnn->lineLeadingBackwardPoint() );
    }
    else if ( m_lineType == 1 )
    {
        m_innerColor->setColor( m_lineAnn->lineInnerColor() );
        if ( m_lineAnn->lineInnerColor().isValid() )
        {
            m_useColor->setChecked( true );
        }
        else
        {
            m_innerColor->setEnabled( false );
        }
    }
    m_spinSize->setRange( 1, 100 );
    m_spinSize->setValue( m_lineAnn->style().width() );

    if ( m_lineType == 0 )
    {
        connect( m_spinLL, SIGNAL(valueChanged(double)), this, SIGNAL(dataChanged()) );
        connect( m_spinLLE, SIGNAL(valueChanged(double)), this, SIGNAL(dataChanged()) );
    }
    else if ( m_lineType == 1 )
    {
        connect( m_innerColor, SIGNAL(changed(QColor)), this, SIGNAL(dataChanged()) );
        connect( m_useColor, SIGNAL(toggled(bool)), this, SIGNAL(dataChanged()) );
        connect( m_useColor, SIGNAL(toggled(bool)), m_innerColor, SLOT(setEnabled(bool)) );
    }
    connect( m_spinSize, SIGNAL(valueChanged(double)), this, SIGNAL(dataChanged()) );

    return widget;
}

void LineAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    if ( m_lineType == 0 )
    {
        m_lineAnn->setLineLeadingForwardPoint( m_spinLL->value() );
        m_lineAnn->setLineLeadingBackwardPoint( m_spinLLE->value() );
    }
    else if ( m_lineType == 1 )
    {
        if ( !m_useColor->isChecked() )
        {
            m_lineAnn->setLineInnerColor( QColor() );
        }
        else
        {
            m_lineAnn->setLineInnerColor( m_innerColor->color() );
        }
    }
    m_lineAnn->style().setWidth( m_spinSize->value() );
}



InkAnnotationWidget::InkAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann )
{
    m_inkAnn = static_cast< Okular::InkAnnotation * >( ann );
}

QWidget * InkAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );

    QGroupBox * gb2 = new QGroupBox( widget );
    lay->addWidget( gb2 );
    gb2->setTitle( i18n( "Style" ) );
    QGridLayout * gridlay2 = new QGridLayout( gb2 );
    QLabel * tmplabel2 = new QLabel( i18n( "&Size:" ), gb2 );
    gridlay2->addWidget( tmplabel2, 0, 0, Qt::AlignRight );
    m_spinSize = new QDoubleSpinBox( gb2 );
    gridlay2->addWidget( m_spinSize, 0, 1 );
    tmplabel2->setBuddy( m_spinSize );

    m_spinSize->setRange( 1, 100 );
    m_spinSize->setValue( m_inkAnn->style().width() );

    connect( m_spinSize, SIGNAL(valueChanged(double)), this, SIGNAL(dataChanged()) );

    return widget;
}

void InkAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_inkAnn->style().setWidth( m_spinSize->value() );
}



HighlightAnnotationWidget::HighlightAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann )
{
    m_hlAnn = static_cast< Okular::HighlightAnnotation * >( ann );
}

QWidget * HighlightAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );
    QHBoxLayout * typelay = new QHBoxLayout();
    lay->addLayout( typelay );
    QLabel * tmplabel = new QLabel( i18n( "Type:" ), widget );
    typelay->addWidget( tmplabel, 0, Qt::AlignRight );
    m_typeCombo = new KComboBox( widget );
    tmplabel->setBuddy( m_typeCombo );
    typelay->addWidget( m_typeCombo );

    m_typeCombo->addItem( i18n( "Highlight" ) );
    m_typeCombo->addItem( i18n( "Squiggle" ) );
    m_typeCombo->addItem( i18n( "Underline" ) );
    m_typeCombo->addItem( i18n( "Strike out" ) );
    m_typeCombo->setCurrentIndex( m_hlAnn->highlightType() );

    connect( m_typeCombo, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dataChanged()) );

    return widget;
}

void HighlightAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_hlAnn->setHighlightType( (Okular::HighlightAnnotation::HighlightType)m_typeCombo->currentIndex() );
}



GeomAnnotationWidget::GeomAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann )
{
    m_geomAnn = static_cast< Okular::GeomAnnotation * >( ann );
}

QWidget * GeomAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QGridLayout * lay = new QGridLayout( widget );
    lay->setMargin( 0 );
    QLabel * tmplabel = new QLabel( i18n( "Type:" ), widget );
    lay->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    m_typeCombo = new KComboBox( widget );
    tmplabel->setBuddy( m_typeCombo );
    lay->addWidget( m_typeCombo, 0, 1 );
    m_useColor = new QCheckBox( i18n( "Inner color:" ), widget );
    lay->addWidget( m_useColor, 1, 0 );
    m_innerColor = new KColorButton( widget );
    lay->addWidget( m_innerColor, 1, 1 );
    tmplabel = new QLabel( i18n( "&Size:" ), widget );
    lay->addWidget( tmplabel, 2, 0, Qt::AlignRight );
    m_spinSize = new QDoubleSpinBox( widget );
    lay->addWidget( m_spinSize, 2, 1 );
    tmplabel->setBuddy( m_spinSize );

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
    m_spinSize->setRange( 0, 100 );
    m_spinSize->setValue( m_geomAnn->style().width() );

    connect( m_typeCombo, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dataChanged()) );
    connect( m_innerColor, SIGNAL(changed(QColor)), this, SIGNAL(dataChanged()) );
    connect( m_useColor, SIGNAL(toggled(bool)), this, SIGNAL(dataChanged()) );
    connect( m_useColor, SIGNAL(toggled(bool)), m_innerColor, SLOT(setEnabled(bool)) );
    connect( m_spinSize, SIGNAL(valueChanged(double)), this, SIGNAL(dataChanged()) );

    return widget;
}

void GeomAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_geomAnn->setGeometricalType( (Okular::GeomAnnotation::GeomType)m_typeCombo->currentIndex() );
    if ( !m_useColor->isChecked() )
    {
        m_geomAnn->setGeometricalInnerColor( QColor() );
    }
    else
    {
        m_geomAnn->setGeometricalInnerColor( m_innerColor->color() );
    }
    m_geomAnn->style().setWidth( m_spinSize->value() );
}



FileAttachmentAnnotationWidget::FileAttachmentAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_pixmapSelector( 0 )
{
    m_attachAnn = static_cast< Okular::FileAttachmentAnnotation * >( ann );
}

QWidget * FileAttachmentAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );
    QGroupBox * gb = new QGroupBox( widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( "File Attachment Symbol" ) );
    QHBoxLayout * gblay = new QHBoxLayout( gb );
    m_pixmapSelector = new PixmapPreviewSelector( gb );
    gblay->addWidget( m_pixmapSelector );
    m_pixmapSelector->setEditable( true );

    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Graph" ), "graph" );
    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Push Pin" ), "pushpin" );
    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Paperclip" ), "paperclip" );
    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Tag" ), "tag" );
    m_pixmapSelector->setIcon( m_attachAnn->fileIconName() );

    connect( m_pixmapSelector, SIGNAL(iconChanged(QString)), this, SIGNAL(dataChanged()) );

    return widget;
}

QWidget * FileAttachmentAnnotationWidget::createExtraWidget()
{
    QWidget * widget = new QWidget();
    widget->setWindowTitle( i18nc( "'File' as normal file, that can be opened, saved, etc..", "File" ) );

    Okular::EmbeddedFile *ef = m_attachAnn->embeddedFile();
    const int size = ef->size();
    const QString sizeString = size <= 0 ? i18nc( "Not available size", "N/A" ) : KGlobal::locale()->formatByteSize( size );
    const QString descString = ef->description().isEmpty() ? i18n( "No description available." ) : ef->description();

    QGridLayout * lay = new QGridLayout( widget );
    lay->setMargin( 0 );
    QLabel * tmplabel = new QLabel( i18n( "Name: %1", ef->name() ), widget );
    tmplabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
    lay->addWidget( tmplabel, 0, 0 );

    tmplabel = new QLabel( i18n( "Size: %1", sizeString ), widget );
    tmplabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
    lay->addWidget( tmplabel, 1, 0 );

    tmplabel = new QLabel( i18n( "Description:" ), widget );
    lay->addWidget( tmplabel, 2, 0 );
    tmplabel = new QLabel( widget );
    tmplabel->setTextFormat( Qt::PlainText );
    tmplabel->setWordWrap( true );
    tmplabel->setText( descString );
    tmplabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
    lay->addWidget( tmplabel, 3, 0, 1, 2 );

    KMimeType::Ptr mime = KMimeType::findByPath( ef->name(), 0, true );
    if ( mime )
    {
        tmplabel = new QLabel( widget );
        tmplabel->setPixmap( KIcon( mime->iconName() ).pixmap( FILEATTACH_ICONSIZE, FILEATTACH_ICONSIZE ) );
        tmplabel->setFixedSize( FILEATTACH_ICONSIZE, FILEATTACH_ICONSIZE );
        lay->addWidget( tmplabel, 0, 1, 3, 1, Qt::AlignTop );
    }

    lay->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding ), 4, 0 );

    return widget;
}

void FileAttachmentAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_attachAnn->setFileIconName( m_pixmapSelector->icon() );
}



static QString caretSymbolToIcon( Okular::CaretAnnotation::CaretSymbol symbol )
{
    switch ( symbol )
    {
        case Okular::CaretAnnotation::None:
            return QString::fromLatin1( "caret-none" );
        case Okular::CaretAnnotation::P:
            return QString::fromLatin1( "caret-p" );
    }
    return QString();
}

static Okular::CaretAnnotation::CaretSymbol caretSymbolFromIcon( const QString &icon )
{
    if ( icon == QLatin1String( "caret-none" ) )
        return Okular::CaretAnnotation::None;
    else if ( icon == QLatin1String( "caret-p" ) )
        return Okular::CaretAnnotation::P;
    return Okular::CaretAnnotation::None;
}

CaretAnnotationWidget::CaretAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_pixmapSelector( 0 )
{
    m_caretAnn = static_cast< Okular::CaretAnnotation * >( ann );
}

QWidget * CaretAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout( widget );
    lay->setMargin( 0 );
    QGroupBox * gb = new QGroupBox( widget );
    lay->addWidget( gb );
    gb->setTitle( i18n( "Caret Symbol" ) );
    QHBoxLayout * gblay = new QHBoxLayout( gb );
    m_pixmapSelector = new PixmapPreviewSelector( gb );
    gblay->addWidget( m_pixmapSelector );

    m_pixmapSelector->addItem( i18nc( "Symbol for caret annotations", "None" ), "caret-none" );
    m_pixmapSelector->addItem( i18nc( "Symbol for caret annotations", "P" ), "caret-p" );
    m_pixmapSelector->setIcon( caretSymbolToIcon( m_caretAnn->caretSymbol() ) );

    connect( m_pixmapSelector, SIGNAL(iconChanged(QString)), this, SIGNAL(dataChanged()) );

    return widget;
}

void CaretAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_caretAnn->setCaretSymbol( caretSymbolFromIcon( m_pixmapSelector->icon() ) );
}



#include "annotationwidgets.moc"
