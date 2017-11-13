
#include "editannottooldialog.h"

#include <kcolorbutton.h>
#include <kcombobox.h>
#include <klocalizedstring.h>
#include <klineedit.h>

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/annotations.h"
#include "ui/annotationwidgets.h"
#include "ui/pageviewannotator.h"


EditAnnotToolDialog::EditAnnotToolDialog( QWidget *parent, const QDomElement &initialState )
    : QDialog( parent ), m_stubann( nullptr ), m_annotationWidget( nullptr )
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditAnnotToolDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EditAnnotToolDialog::reject);
    okButton->setDefault(true);

    QLabel * tmplabel;
    QWidget *widget = new QWidget( this );
    QGridLayout * widgetLayout = new QGridLayout( widget );

    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);


    m_name = new KLineEdit( widget );
    mainLayout->addWidget(m_name);
    tmplabel = new QLabel( i18n( "&Name:" ), widget );
    mainLayout->addWidget(tmplabel);
    tmplabel->setBuddy( m_name );
    widgetLayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_name, 0, 1 );
    
    m_key = new KLineEdit( widget );
    mainLayout->addWidget(m_key);
    tmplabel = new QLabel("Key:", widget );
    mainLayout->addWidget(tmplabel);
    tmplabel->setBuddy( m_key );
    widgetLayout->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_key, 1, 1 );  
    

    m_type = new KComboBox( false, widget );
    mainLayout->addWidget(m_type);
    connect(m_type, static_cast<void (KComboBox::*)(int)>(&KComboBox::currentIndexChanged), this, &EditAnnotToolDialog::slotTypeChanged);
    tmplabel = new QLabel( i18n( "&Type:" ), widget );
    mainLayout->addWidget(tmplabel);
    tmplabel->setBuddy( m_type );
    widgetLayout->addWidget( tmplabel, 2, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_type, 2, 1 );

    m_toolIcon = new QLabel( widget );
    mainLayout->addWidget(m_toolIcon);
    m_toolIcon->setAlignment( Qt::AlignRight | Qt::AlignTop );
    m_toolIcon->setMinimumSize( 40, 32 );
    widgetLayout->addWidget( m_toolIcon, 0, 2, 2, 1 );

    m_appearanceBox = new QGroupBox( i18n( "Appearance" ), widget );
    mainLayout->addWidget(m_appearanceBox);
    m_appearanceBox->setLayout( new QVBoxLayout( m_appearanceBox ) );
    widgetLayout->addWidget( m_appearanceBox, 3, 0, 1, 3 );

    // Populate combobox with annotation types
    m_type->addItem( i18n("Pop-up Note"), qVariantFromValue( ToolNoteLinked ) );
    m_type->addItem( i18n("Inline Note"), qVariantFromValue( ToolNoteInline ) );
    m_type->addItem( i18n("Freehand Line"), qVariantFromValue( ToolInk ) );
    m_type->addItem( i18n("Straight Line"), qVariantFromValue( ToolStraightLine ) );
    m_type->addItem( i18n("Polygon"), qVariantFromValue( ToolPolygon ) );
    m_type->addItem( i18n("Text markup"), qVariantFromValue( ToolTextMarkup ) );
    m_type->addItem( i18n("Geometrical shape"), qVariantFromValue( ToolGeometricalShape ) );
    m_type->addItem( i18n("Stamp"), qVariantFromValue( ToolStamp ) );
    
    createStubAnnotation();

    if ( initialState.isNull() )
    {
        setWindowTitle( i18n("Create annotation tool") );
    }
    else
    {
        setWindowTitle( i18n("Edit annotation tool") );
        loadTool( initialState );
    }

    rebuildAppearanceBox();
    updateDefaultNameAndIcon();
}

EditAnnotToolDialog::~EditAnnotToolDialog()
{
    delete m_annotationWidget;
}

QString EditAnnotToolDialog::name() const
{
    return m_name->text();
}

 QString EditAnnotToolDialog::key() const
 {
     return m_key->text();
 }

QDomDocument EditAnnotToolDialog::toolXml() const
{
    const ToolType toolType = m_type->itemData( m_type->currentIndex() ).value<ToolType>();

    QDomDocument doc;
    QDomElement toolElement = doc.createElement( QStringLiteral("tool") );
    QDomElement engineElement = doc.createElement( QStringLiteral("engine") );
    QDomElement annotationElement = doc.createElement( QStringLiteral("annotation") );
    doc.appendChild( toolElement );
    toolElement.appendChild( engineElement );
    engineElement.appendChild( annotationElement );

    const QString color = m_stubann->style().color().name();
    const QString key = m_key->text();
    const QString opacity = QString::number( m_stubann->style().opacity() );
    const QString width = QString::number( m_stubann->style().width() );

    if ( toolType == ToolNoteLinked )
    {
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("note-linked") );
        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("PickPoint") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        engineElement.setAttribute( QStringLiteral("hoverIcon"), QStringLiteral("tool-note") );
        annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Text") );
        annotationElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("icon"), ta->textIcon() );
    }
    else if ( toolType == ToolNoteInline )
    {
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("note-inline") );
        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("PickPoint") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        engineElement.setAttribute( QStringLiteral("hoverIcon"), QStringLiteral("tool-note-inline") );
        engineElement.setAttribute( QStringLiteral("block"), QStringLiteral("true") );
        annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("FreeText") );
        annotationElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("width"), width );
        if ( ta->inplaceAlignment() != 0 )
            annotationElement.setAttribute( QStringLiteral("align"), ta->inplaceAlignment() );
        if ( ta->textFont() != QApplication::font() )
            annotationElement.setAttribute( QStringLiteral("font"), ta->textFont().toString() );
    }
    else if ( toolType == ToolInk )
    {
        toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("ink") );
        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("SmoothLine") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Ink") );
        annotationElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("width"), width );
    }
    else if ( toolType == ToolStraightLine )
    {
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("straight-line") );
        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("PolyLine") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        engineElement.setAttribute( QStringLiteral("points"), QStringLiteral("2") );
        annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Line") );
        annotationElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("width"), width );
        if ( la->lineLeadingForwardPoint() != 0 || la->lineLeadingBackwardPoint() != 0 )
        {
            annotationElement.setAttribute( QStringLiteral("leadFwd"), QString::number( la->lineLeadingForwardPoint() ) );
            annotationElement.setAttribute( QStringLiteral("leadBack"), QString::number( la->lineLeadingBackwardPoint() ) );
        }
    }
    else if ( toolType == ToolPolygon )
    {
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("polygon") );
        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("PolyLine") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        engineElement.setAttribute( QStringLiteral("points"), QStringLiteral("-1") );
        annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Line") );
        annotationElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("width"), width );
        if ( la->lineInnerColor().isValid() )
        {
            annotationElement.setAttribute( QStringLiteral("innerColor"), la->lineInnerColor().name() );
        }
    }
    else if ( toolType == ToolTextMarkup )
    {
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );

        switch ( ha->highlightType() )
        {
            case Okular::HighlightAnnotation::Highlight:
                toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("highlight") );
                annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Highlight") );
                break;
            case Okular::HighlightAnnotation::Squiggly:
                toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("squiggly") );
                annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Squiggly") );
                break;
            case Okular::HighlightAnnotation::Underline:
                toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("underline") );
                annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Underline") );
                break;
            case Okular::HighlightAnnotation::StrikeOut:
                toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("strikeout") );
                annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("StrikeOut") );
                break;
        }

        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("TextSelector") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("color"), color );
    }
    else if ( toolType == ToolGeometricalShape )
    {
        Okular::GeomAnnotation * ga = static_cast<Okular::GeomAnnotation*>( m_stubann );

        if ( ga->geometricalType() == Okular::GeomAnnotation::InscribedCircle )
        {
            toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("ellipse") );
            annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("GeomCircle") );
        }
        else
        {
            toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("rectangle") );
            annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("GeomSquare") );
        }

        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("PickPoint") );
        engineElement.setAttribute( QStringLiteral("color"), color );
        engineElement.setAttribute( QStringLiteral("block"), QStringLiteral("true") );
        annotationElement.setAttribute( QStringLiteral("color"), color );
        annotationElement.setAttribute( QStringLiteral("width"), width );

        if ( ga->geometricalInnerColor().isValid() )
            annotationElement.setAttribute( QStringLiteral("innerColor"), ga->geometricalInnerColor().name() );
    }
    else if ( toolType == ToolStamp )
    {
        Okular::StampAnnotation * sa = static_cast<Okular::StampAnnotation*>( m_stubann );
        toolElement.setAttribute( QStringLiteral("type"), QStringLiteral("stamp") );
        engineElement.setAttribute( QStringLiteral("type"), QStringLiteral("PickPoint") );
        engineElement.setAttribute( QStringLiteral("hoverIcon"), sa->stampIconName() );
        engineElement.setAttribute( QStringLiteral("size"), QStringLiteral("64") );
        engineElement.setAttribute( QStringLiteral("block"), QStringLiteral("true") );
        annotationElement.setAttribute( QStringLiteral("type"), QStringLiteral("Stamp") );
        annotationElement.setAttribute( QStringLiteral("icon"), sa->stampIconName() );
    }

    if ( opacity != QStringLiteral("1") )
        annotationElement.setAttribute( QStringLiteral("opacity"), opacity );
    
    annotationElement.setAttribute( QStringLiteral("key"), key );      

    return doc;
}

void EditAnnotToolDialog::createStubAnnotation()
{
    const ToolType toolType = m_type->itemData( m_type->currentIndex() ).value<ToolType>();

    // Delete previous stub annotation, if any
    delete m_stubann;

    // Create stub annotation
    if ( toolType == ToolNoteLinked )
    {
        Okular::TextAnnotation * ta = new Okular::TextAnnotation();
        ta->setTextType( Okular::TextAnnotation::Linked );
        ta->setTextIcon( QStringLiteral("Note") );
        ta->style().setColor( Qt::yellow );
        m_stubann = ta;
    }
    else if ( toolType == ToolNoteInline )
    {
        Okular::TextAnnotation * ta = new Okular::TextAnnotation();
        ta->setTextType( Okular::TextAnnotation::InPlace );
        ta->style().setWidth( 1.0 );
        ta->style().setColor( Qt::yellow );
        m_stubann = ta;
    }
    else if ( toolType == ToolInk )
    {
        m_stubann = new Okular::InkAnnotation();
        m_stubann->style().setWidth( 2.0 );
        m_stubann->style().setColor( Qt::green );
    }
    else if ( toolType == ToolStraightLine )
    {
        Okular::LineAnnotation * la = new Okular::LineAnnotation();
        la->setLinePoints( QLinkedList<Okular::NormalizedPoint>() <<
                                Okular::NormalizedPoint(0,0) <<
                                Okular::NormalizedPoint(1,0) );
        la->style().setColor( QColor( 0xff, 0xe0, 0x00 ) );
        m_stubann = la;
    }
    else if ( toolType == ToolPolygon )
    {
        Okular::LineAnnotation * la = new Okular::LineAnnotation();
        la->setLinePoints( QLinkedList<Okular::NormalizedPoint>() <<
                                Okular::NormalizedPoint(0,0) <<
                                Okular::NormalizedPoint(1,0) <<
                                Okular::NormalizedPoint(1,1) );
        la->setLineClosed( true );
        la->style().setColor( QColor( 0x00, 0x7e, 0xee ) );
        m_stubann = la;
    }
    else if ( toolType == ToolTextMarkup )
    {
        m_stubann = new Okular::HighlightAnnotation();
        m_stubann->style().setColor( Qt::yellow );
    }
    else if ( toolType == ToolGeometricalShape )
    {
        Okular::GeomAnnotation * ga = new Okular::GeomAnnotation();
        ga->setGeometricalType( Okular::GeomAnnotation::InscribedCircle );
        ga->style().setWidth( 5.0 );
        ga->style().setColor( Qt::cyan );
        m_stubann = ga;
    }
    else if ( toolType == ToolStamp )
    {
        Okular::StampAnnotation * sa = new Okular::StampAnnotation();
        sa->setStampIconName( QStringLiteral("okular") );
        m_stubann = sa;
    }
}

void EditAnnotToolDialog::rebuildAppearanceBox()
{
    // Remove previous widget (if any)
    if ( m_annotationWidget )
    {
        delete m_annotationWidget->appearanceWidget();
        delete m_annotationWidget;
    }

    m_annotationWidget = AnnotationWidgetFactory::widgetFor( m_stubann );
    m_appearanceBox->layout()->addWidget( m_annotationWidget->appearanceWidget() );

    connect(m_annotationWidget, &AnnotationWidget::dataChanged, this, &EditAnnotToolDialog::slotDataChanged);
}

void EditAnnotToolDialog::updateDefaultNameAndIcon()
{
    QDomDocument doc = toolXml();
    QDomElement toolElement = doc.documentElement();
    m_name->setPlaceholderText( PageViewAnnotator::defaultToolName( toolElement ) );
    m_toolIcon->setPixmap( PageViewAnnotator::makeToolPixmap( toolElement ) );
}

void EditAnnotToolDialog::setToolType( ToolType newType )
{
    int idx = -1;

    for ( int i = 0; idx == -1 && i < m_type->count(); ++i )
    {
        if ( m_type->itemData( i ).value<ToolType>() == newType )
            idx = i;
    }

    // The following call also results in createStubAnnotation being called
    m_type->setCurrentIndex( idx );
}

void EditAnnotToolDialog::loadTool( const QDomElement &toolElement )
{
    const QDomElement engineElement = toolElement.elementsByTagName( QStringLiteral("engine") ).item( 0 ).toElement();
    const QDomElement annotationElement = engineElement.elementsByTagName( QStringLiteral("annotation") ).item( 0 ).toElement();
    const QString annotType = toolElement.attribute( QStringLiteral("type") );    

    if ( annotType == QLatin1String("ellipse") )
    {
        setToolType( ToolGeometricalShape );
        Okular::GeomAnnotation * ga = static_cast<Okular::GeomAnnotation*>( m_stubann );
        ga->setGeometricalType( Okular::GeomAnnotation::InscribedCircle );
        if ( annotationElement.hasAttribute( QStringLiteral("innerColor") ) )
            ga->setGeometricalInnerColor( QColor( annotationElement.attribute( QStringLiteral("innerColor") ) ) );
    }
    else if ( annotType == QLatin1String("highlight") )
    {
        setToolType( ToolTextMarkup );
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );
        ha->setHighlightType( Okular::HighlightAnnotation::Highlight );
    }
    else if ( annotType == QLatin1String("ink") )
    {
        setToolType( ToolInk );
    }
    else if ( annotType == QLatin1String("note-inline") )
    {
        setToolType( ToolNoteInline );
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        if ( annotationElement.hasAttribute( QStringLiteral("align") ) )
            ta->setInplaceAlignment( annotationElement.attribute( QStringLiteral("align") ).toInt() );
        if ( annotationElement.hasAttribute( QStringLiteral("font") ) )
        {
            QFont f;
            f.fromString( annotationElement.attribute( QStringLiteral("font") ) );
            ta->setTextFont( f );
        }
    }
    else if ( annotType == QLatin1String("note-linked") )
    {
        setToolType( ToolNoteLinked );
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        ta->setTextIcon( annotationElement.attribute( QStringLiteral("icon") ) );
    }
    else if ( annotType == QLatin1String("polygon") )
    {
        setToolType( ToolPolygon );
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        if ( annotationElement.hasAttribute( QStringLiteral("innerColor") ) )
            la->setLineInnerColor( QColor( annotationElement.attribute( QStringLiteral("innerColor") ) ) );
    }
    else if ( annotType == QLatin1String("rectangle") )
    {
        setToolType( ToolGeometricalShape );
        Okular::GeomAnnotation * ga = static_cast<Okular::GeomAnnotation*>( m_stubann );
        ga->setGeometricalType( Okular::GeomAnnotation::InscribedSquare );
        if ( annotationElement.hasAttribute( QStringLiteral("innerColor") ) )
            ga->setGeometricalInnerColor( QColor( annotationElement.attribute( QStringLiteral("innerColor") ) ) );
    }
    else if ( annotType == QLatin1String("squiggly") )
    {
        setToolType( ToolTextMarkup );
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );
        ha->setHighlightType( Okular::HighlightAnnotation::Squiggly );
    }
    else if ( annotType == QLatin1String("stamp") )
    {
        setToolType( ToolStamp );
        Okular::StampAnnotation * sa = static_cast<Okular::StampAnnotation*>( m_stubann );
        sa->setStampIconName( annotationElement.attribute( QStringLiteral("icon") ) );
    }
    else if ( annotType == QLatin1String("straight-line") )
    {
        setToolType( ToolStraightLine );
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        if ( annotationElement.hasAttribute( QStringLiteral("leadFwd") ) )
            la->setLineLeadingForwardPoint( annotationElement.attribute( QStringLiteral("leadFwd") ).toDouble() );
        if ( annotationElement.hasAttribute( QStringLiteral("leadBack") ) )
            la->setLineLeadingBackwardPoint( annotationElement.attribute( QStringLiteral("leadBack") ).toDouble() );
    }
    else if ( annotType == QLatin1String("strikeout") )
    {
        setToolType( ToolTextMarkup );
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );
        ha->setHighlightType( Okular::HighlightAnnotation::StrikeOut );
    }
    else if ( annotType == QLatin1String("underline") )
    {
        setToolType( ToolTextMarkup );
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );
        ha->setHighlightType( Okular::HighlightAnnotation::Underline );
    }

    // Common properties
    if ( annotationElement.hasAttribute( QStringLiteral("color") ) )
        m_stubann->style().setColor( QColor( annotationElement.attribute( QStringLiteral("color") ) ) );
    if ( annotationElement.hasAttribute( QStringLiteral("opacity") ) )
        m_stubann->style().setOpacity( annotationElement.attribute( QStringLiteral("opacity") ).toDouble() );
    if ( annotationElement.hasAttribute( QStringLiteral("width") ) )
        m_stubann->style().setWidth( annotationElement.attribute( QStringLiteral("width") ).toDouble() );

    if ( toolElement.hasAttribute( QStringLiteral("name") ) )
        m_name->setText( toolElement.attribute( QStringLiteral("name") ) ); 
    if ( annotationElement.hasAttribute( QStringLiteral("key") ) )
        m_key->setText( annotationElement.attribute( QStringLiteral("key") ) );     
    
}

void EditAnnotToolDialog::slotTypeChanged()
{
    createStubAnnotation();
    rebuildAppearanceBox();
    updateDefaultNameAndIcon();
}

void EditAnnotToolDialog::slotDataChanged()
{
    // Mirror changes back in the stub annotation
    m_annotationWidget->applyChanges();

    updateDefaultNameAndIcon();
}
