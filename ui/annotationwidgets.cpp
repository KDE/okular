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
#include <QIcon>
#include <kiconloader.h>
#include <KLocalizedString>
#include <QDebug>
#include <QMimeDatabase>
#include <KFormat>

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
    connect( m_comboItems, &QComboBox::editTextChanged, this, &PixmapPreviewSelector::iconComboChanged );
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
    : m_ann( ann )
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
    if (m_colorBn)
        m_ann->style().setColor( m_colorBn->color() );
    if (m_opacity)
        m_ann->style().setOpacity( (double)m_opacity->value() / 100.0 );
}

QWidget * AnnotationWidget::createAppearanceWidget()
{
    QWidget * widget = new QWidget();
    QGridLayout * gridlayout = new QGridLayout( widget );
    if ( hasColorButton() )
    {
        QLabel * tmplabel = new QLabel( i18n( "&Color:" ), widget );
        gridlayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
        m_colorBn = new KColorButton( widget );
        m_colorBn->setColor( m_ann->style().color() );
        tmplabel->setBuddy( m_colorBn );
        gridlayout->addWidget( m_colorBn, 0, 1 );
    }
    if ( hasOpacityBox() )
    {
        QLabel * tmplabel = new QLabel( i18n( "&Opacity:" ), widget );
        gridlayout->addWidget( tmplabel, 1, 0, Qt::AlignRight );
        m_opacity = new QSpinBox( widget );
        m_opacity->setRange( 0, 100 );
        m_opacity->setValue( (int)( m_ann->style().opacity() * 100 ) );
        m_opacity->setSuffix( i18nc( "Suffix for the opacity level, eg '80 %'", " %" ) );
        tmplabel->setBuddy( m_opacity );
        gridlayout->addWidget( m_opacity, 1, 1 );
    }

    QWidget * styleWidget = createStyleWidget();
    if ( styleWidget )
        gridlayout->addWidget( styleWidget, 2, 0, 1, 2 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding ), 3, 0 );

    if ( m_colorBn )
        connect( m_colorBn, &KColorButton::changed, this, &AnnotationWidget::dataChanged );
    if ( m_opacity )
        connect( m_opacity, SIGNAL(valueChanged(int)), this, SIGNAL(dataChanged()) );

    return widget;
}

QWidget * AnnotationWidget::createStyleWidget()
{
    return nullptr;
}

QWidget * AnnotationWidget::createExtraWidget()
{
    return nullptr;
}


TextAnnotationWidget::TextAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann )
{
    m_textAnn = static_cast< Okular::TextAnnotation * >( ann );
}

QWidget * TextAnnotationWidget::createStyleWidget()
{
    QWidget * widget = new QWidget();
    QVBoxLayout * layout = new QVBoxLayout( widget );
    layout->setMargin( 0 );

    if ( m_textAnn->textType() == Okular::TextAnnotation::Linked )
    {
        createPopupNoteStyleUi( widget, layout );
    }
    else if ( m_textAnn->textType() == Okular::TextAnnotation::InPlace )
    {
        if ( isTypewriter() )
            createTypewriterStyleUi( widget, layout );
        else
            createInlineNoteStyleUi( widget, layout );
    }

    return widget;
}

bool TextAnnotationWidget::hasColorButton() const {
    return !isTypewriter();
}

bool TextAnnotationWidget::hasOpacityBox() const {
    return !isTypewriter();
}

void TextAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    if ( m_textAnn->textType() == Okular::TextAnnotation::Linked )
    {
        Q_ASSERT( m_pixmapSelector );
        m_textAnn->setTextIcon( m_pixmapSelector->icon() );
    }
    else if ( m_textAnn->textType() == Okular::TextAnnotation::InPlace )
    {
        Q_ASSERT( m_fontReq );
        m_textAnn->setTextFont( m_fontReq->font() );
        if ( !isTypewriter() )
        {
            Q_ASSERT( m_textAlign && m_spinWidth );
            m_textAnn->setInplaceAlignment( m_textAlign->currentIndex() );
            m_textAnn->style().setWidth( m_spinWidth->value() );
        }
        else
        {
            Q_ASSERT( m_textColorBn );
            m_textAnn->setTextColor( m_textColorBn->color() );
        }
    }
}

void TextAnnotationWidget::createPopupNoteStyleUi( QWidget * widget, QVBoxLayout * layout  ) {
    QGroupBox * gb = new QGroupBox( widget );
    layout->addWidget( gb );
    QHBoxLayout * gblay = new QHBoxLayout( gb );
    gb->setTitle( i18n( "Icon" ) );
    addPixmapSelector( gb, gblay );
}

void TextAnnotationWidget::createInlineNoteStyleUi( QWidget * widget, QVBoxLayout * layout  ) {
    QGridLayout * innerlay = new QGridLayout();
    layout->addLayout( innerlay );
    addFontRequester( widget, innerlay );
    addTextAlignComboBox( widget, innerlay );
    addWidthSpinBox( widget, innerlay );
}

void TextAnnotationWidget::createTypewriterStyleUi( QWidget * widget, QVBoxLayout * layout  ) {
    QGridLayout * innerlay = new QGridLayout();
    layout->addLayout( innerlay );
    addFontRequester( widget, innerlay );
    addTextColorButton( widget, innerlay );
}

void TextAnnotationWidget::addPixmapSelector( QWidget * widget, QLayout * layout )
{
  m_pixmapSelector = new PixmapPreviewSelector( widget );
  layout->addWidget( m_pixmapSelector );
  m_pixmapSelector->addItem( i18n( "Comment" ), QStringLiteral("Comment") );
  m_pixmapSelector->addItem( i18n( "Help" ), QStringLiteral("Help") );
  m_pixmapSelector->addItem( i18n( "Insert" ), QStringLiteral("Insert") );
  m_pixmapSelector->addItem( i18n( "Key" ), QStringLiteral("Key") );
  m_pixmapSelector->addItem( i18n( "New Paragraph" ), QStringLiteral("NewParagraph") );
  m_pixmapSelector->addItem( i18n( "Note" ), QStringLiteral("Note") );
  m_pixmapSelector->addItem( i18n( "Paragraph" ), QStringLiteral("Paragraph") );
  m_pixmapSelector->setIcon( m_textAnn->textIcon() );
  connect( m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged );
}

void TextAnnotationWidget::addFontRequester( QWidget * widget, QGridLayout * layout )
{
    const int row = layout->rowCount();
    QLabel * tmplabel = new QLabel( i18n( "Font:" ), widget );
    layout->addWidget( tmplabel, row, 0 );
    m_fontReq = new KFontRequester( widget );
    layout->addWidget( m_fontReq, row, 1 );
    m_fontReq->setFont( m_textAnn->textFont() );
    connect( m_fontReq, &KFontRequester::fontSelected, this, &AnnotationWidget::dataChanged );
}

void TextAnnotationWidget::addTextColorButton( QWidget * widget, QGridLayout * layout )
{
    const int row = layout->rowCount();
    QLabel * tmplabel = new QLabel( i18n( "&Text Color:" ), widget );
    layout->addWidget( tmplabel, row, 0, Qt::AlignRight );
    m_textColorBn = new KColorButton( widget );
    m_textColorBn->setColor( m_textAnn->textColor() );
    tmplabel->setBuddy( m_textColorBn );
    layout->addWidget( m_textColorBn, row, 1 );
    connect( m_textColorBn, &KColorButton::changed, this, &AnnotationWidget::dataChanged );
}

void TextAnnotationWidget::addTextAlignComboBox( QWidget * widget, QGridLayout * layout )
{
    const int row = layout->rowCount();
    QLabel * tmplabel = new QLabel( i18n( "Align:" ), widget );
    layout->addWidget( tmplabel, row, 0 );
    m_textAlign = new KComboBox( widget );
    layout->addWidget( m_textAlign, row, 1 );
    m_textAlign->addItem( i18n("Left") );
    m_textAlign->addItem( i18n("Center") );
    m_textAlign->addItem( i18n("Right") );
    m_textAlign->setCurrentIndex( m_textAnn->inplaceAlignment() );
    connect( m_textAlign, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dataChanged()) );
}

void TextAnnotationWidget::addWidthSpinBox( QWidget * widget, QGridLayout * layout )
{
    const int row = layout->rowCount();
    QLabel * tmplabel = new QLabel( i18n( "Border Width:" ), widget );
    layout->addWidget( tmplabel, row, 0, Qt::AlignRight );
    m_spinWidth = new QDoubleSpinBox( widget );
    layout->addWidget( m_spinWidth, row, 1 );
    tmplabel->setBuddy( m_spinWidth );
    m_spinWidth->setRange( 0, 100 );
    m_spinWidth->setValue( m_textAnn->style().width() );
    m_spinWidth->setSingleStep( 0.1 );
    connect( m_spinWidth, SIGNAL(valueChanged(double)), this, SIGNAL(dataChanged()) );
}

StampAnnotationWidget::StampAnnotationWidget( Okular::Annotation * ann )
    : AnnotationWidget( ann ), m_pixmapSelector( nullptr )
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

    m_pixmapSelector->addItem( i18n( "Okular" ), QStringLiteral("okular") );
    m_pixmapSelector->addItem( i18n( "Bookmark" ), QStringLiteral("bookmarks") );
    m_pixmapSelector->addItem( i18n( "KDE" ), QStringLiteral("kde") );
    m_pixmapSelector->addItem( i18n( "Information" ), QStringLiteral("help-about") );
    m_pixmapSelector->addItem( i18n( "Approved" ), QStringLiteral("Approved") );
    m_pixmapSelector->addItem( i18n( "As Is" ), QStringLiteral("AsIs") );
    m_pixmapSelector->addItem( i18n( "Confidential" ), QStringLiteral("Confidential") );
    m_pixmapSelector->addItem( i18n( "Departmental" ), QStringLiteral("Departmental") );
    m_pixmapSelector->addItem( i18n( "Draft" ), QStringLiteral("Draft") );
    m_pixmapSelector->addItem( i18n( "Experimental" ), QStringLiteral("Experimental") );
    m_pixmapSelector->addItem( i18n( "Expired" ), QStringLiteral("Expired") );
    m_pixmapSelector->addItem( i18n( "Final" ), QStringLiteral("Final") );
    m_pixmapSelector->addItem( i18n( "For Comment" ), QStringLiteral("ForComment") );
    m_pixmapSelector->addItem( i18n( "For Public Release" ), QStringLiteral("ForPublicRelease") );
    m_pixmapSelector->addItem( i18n( "Not Approved" ), QStringLiteral("NotApproved") );
    m_pixmapSelector->addItem( i18n( "Not For Public Release" ), QStringLiteral("NotForPublicRelease") );
    m_pixmapSelector->addItem( i18n( "Sold" ), QStringLiteral("Sold") );
    m_pixmapSelector->addItem( i18n( "Top Secret" ), QStringLiteral("TopSecret") );
    m_pixmapSelector->setIcon( m_stampAnn->stampIconName() );
    m_pixmapSelector->setPreviewSize( 64 );

    connect( m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged );

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
        connect( m_innerColor, &KColorButton::changed, this, &AnnotationWidget::dataChanged );
        connect( m_useColor, &QAbstractButton::toggled, this, &AnnotationWidget::dataChanged );
        connect(m_useColor, &QCheckBox::toggled, m_innerColor, &KColorButton::setEnabled);
    }
    connect( m_spinSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineAnnotationWidget::dataChanged );

    //Line Term Styles
    QLabel * tmplabel3 = new QLabel( i18n( "Line Start:" ), widget );
    QLabel * tmplabel4 = new QLabel( i18n( "Line End:" ), widget );
    gridlay2->addWidget( tmplabel3, 1, 0, Qt::AlignRight );
    gridlay2->addWidget( tmplabel4, 2, 0, Qt::AlignRight );
    m_startStyleCombo = new QComboBox( widget );
    m_endStyleCombo = new QComboBox( widget );
    tmplabel3->setBuddy( m_startStyleCombo );
    tmplabel4->setBuddy( m_endStyleCombo );
    gridlay2->addWidget( m_startStyleCombo, 1, 1, Qt::AlignLeft );
    gridlay2->addWidget( m_endStyleCombo,  2, 1, Qt::AlignLeft );
    tmplabel3->setToolTip( i18n("Only for PDF documents") );
    tmplabel4->setToolTip( i18n("Only for PDF documents") );
    m_startStyleCombo->setToolTip( i18n("Only for PDF documents"));
    m_endStyleCombo->setToolTip( i18n("Only for PDF documents"));

    for ( const QString &i: { i18n( " Square" ), i18n( " Circle" ), i18n( " Diamond" ), i18n( " Open Arrow" ), i18n( " Closed Arrow" ),
                    i18n( " None" ), i18n( " Butt" ), i18n( " Right Open Arrow" ), i18n( " Right Closed Arrow" ), i18n( "Slash" ) } )
    {
        m_startStyleCombo->addItem(i);
        m_endStyleCombo->addItem(i);
    }

    m_startStyleCombo->setCurrentIndex( m_lineAnn->lineStartStyle() );
    m_endStyleCombo->setCurrentIndex( m_lineAnn->lineEndStyle() );
    connect( m_startStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineAnnotationWidget::dataChanged );
    connect( m_endStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineAnnotationWidget::dataChanged );

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
    m_lineAnn->setLineStartStyle( (Okular::LineAnnotation::TermStyle)m_startStyleCombo->currentIndex() );
    m_lineAnn->setLineEndStyle( (Okular::LineAnnotation::TermStyle)m_endStyleCombo->currentIndex() );
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
    connect( m_innerColor, &KColorButton::changed, this, &AnnotationWidget::dataChanged );
    connect( m_useColor, &QAbstractButton::toggled, this, &AnnotationWidget::dataChanged );
    connect(m_useColor, &QCheckBox::toggled, m_innerColor, &KColorButton::setEnabled);
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
    : AnnotationWidget( ann ), m_pixmapSelector( nullptr )
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

    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Graph" ), QStringLiteral("graph") );
    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Push Pin" ), QStringLiteral("pushpin") );
    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Paperclip" ), QStringLiteral("paperclip") );
    m_pixmapSelector->addItem( i18nc( "Symbol for file attachment annotations", "Tag" ), QStringLiteral("tag") );
    m_pixmapSelector->setIcon( m_attachAnn->fileIconName() );

    connect( m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged );

    return widget;
}

QWidget * FileAttachmentAnnotationWidget::createExtraWidget()
{
    QWidget * widget = new QWidget();
    widget->setWindowTitle( i18nc( "'File' as normal file, that can be opened, saved, etc..", "File" ) );

    Okular::EmbeddedFile *ef = m_attachAnn->embeddedFile();
    const int size = ef->size();
    const QString sizeString = size <= 0 ? i18nc( "Not available size", "N/A" ) : KFormat().formatByteSize( size );
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

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile( ef->name(), QMimeDatabase::MatchExtension);
    if ( mime.isValid() )
    {
        tmplabel = new QLabel( widget );
        tmplabel->setPixmap( QIcon::fromTheme( mime.iconName() ).pixmap( FILEATTACH_ICONSIZE, FILEATTACH_ICONSIZE ) );
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
            return QStringLiteral( "caret-none" );
        case Okular::CaretAnnotation::P:
            return QStringLiteral( "caret-p" );
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
    : AnnotationWidget( ann ), m_pixmapSelector( nullptr )
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

    m_pixmapSelector->addItem( i18nc( "Symbol for caret annotations", "None" ), QStringLiteral("caret-none") );
    m_pixmapSelector->addItem( i18nc( "Symbol for caret annotations", "P" ), QStringLiteral("caret-p") );
    m_pixmapSelector->setIcon( caretSymbolToIcon( m_caretAnn->caretSymbol() ) );

    connect( m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged );

    return widget;
}

void CaretAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_caretAnn->setCaretSymbol( caretSymbolFromIcon( m_pixmapSelector->icon() ) );
}



#include "moc_annotationwidgets.cpp"
