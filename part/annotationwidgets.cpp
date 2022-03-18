/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "annotationwidgets.h"

// qt/kde includes
#include <KColorButton>
#include <KComboBox>
#include <KFontRequester>
#include <KFormat>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <QCheckBox>
#include <QDebug>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMimeDatabase>
#include <QPair>
#include <QSize>
#include <QSpinBox>
#include <QVariant>

#include "core/annotations.h"
#include "core/annotations_p.h"
#include "core/document.h"
#include "core/document_p.h"
#include "core/page_p.h"
#include "gui/guiutils.h"
#include "gui/pagepainter.h"

#define FILEATTACH_ICONSIZE 48

PixmapPreviewSelector::PixmapPreviewSelector(QWidget *parent, PreviewPosition position)
    : QWidget(parent)
    , m_previewPosition(position)
{
    QVBoxLayout *mainlay = new QVBoxLayout(this);
    mainlay->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *toplay = new QHBoxLayout(this);
    toplay->setContentsMargins(0, 0, 0, 0);
    mainlay->addLayout(toplay);
    m_comboItems = new KComboBox(this);
    toplay->addWidget(m_comboItems);
    m_stampPushButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), QString(), this);
    m_stampPushButton->setVisible(false);
    m_stampPushButton->setToolTip(i18nc("@info:tooltip", "Select a custom stamp symbol from file"));
    toplay->addWidget(m_stampPushButton);
    m_iconLabel = new QLabel(this);
    switch (m_previewPosition) {
    case Side:
        toplay->addWidget(m_iconLabel);
        break;
    case Below:
        mainlay->addWidget(m_iconLabel);
        mainlay->setAlignment(m_iconLabel, Qt::AlignHCenter);
        break;
    }
    m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFrameStyle(QFrame::StyledPanel);
    setPreviewSize(32);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(m_comboItems);

    connect(m_comboItems, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), this, &PixmapPreviewSelector::iconComboChanged);
    connect(m_comboItems, &QComboBox::editTextChanged, this, &PixmapPreviewSelector::iconComboChanged);
    connect(m_stampPushButton, &QPushButton::clicked, this, &PixmapPreviewSelector::selectCustomStamp);
}

PixmapPreviewSelector::~PixmapPreviewSelector()
{
}

void PixmapPreviewSelector::setIcon(const QString &icon)
{
    int id = m_comboItems->findData(QVariant(icon), Qt::UserRole, Qt::MatchFixedString);
    if (id == -1) {
        id = m_comboItems->findText(icon, Qt::MatchFixedString);
    }
    if (id > -1) {
        m_comboItems->setCurrentIndex(id);
    } else if (m_comboItems->isEditable()) {
        m_comboItems->addItem(icon, QVariant(icon));
        m_comboItems->setCurrentIndex(m_comboItems->findText(icon, Qt::MatchFixedString));
    }
}

QString PixmapPreviewSelector::icon() const
{
    return m_icon;
}

void PixmapPreviewSelector::addItem(const QString &item, const QString &id)
{
    m_comboItems->addItem(item, QVariant(id));
    setIcon(m_icon);
}

void PixmapPreviewSelector::setPreviewSize(int size)
{
    m_previewSize = size;
    switch (m_previewPosition) {
    case Side:
        m_iconLabel->setFixedSize(m_previewSize + 8, m_previewSize + 8);
        break;
    case Below:
        m_iconLabel->setFixedSize(3 * m_previewSize + 8, m_previewSize + 8);
        break;
    }
    iconComboChanged(m_icon);
}

int PixmapPreviewSelector::previewSize() const
{
    return m_previewSize;
}

void PixmapPreviewSelector::setEditable(bool editable)
{
    m_comboItems->setEditable(editable);
    m_stampPushButton->setVisible(editable);
}

void PixmapPreviewSelector::iconComboChanged(const QString &icon)
{
    int id = m_comboItems->findText(icon, Qt::MatchFixedString);
    if (id >= 0) {
        m_icon = m_comboItems->itemData(id).toString();
    } else {
        m_icon = icon;
    }

    QPixmap pixmap = Okular::AnnotationUtils::loadStamp(m_icon, m_previewSize);
    const QRect cr = m_iconLabel->contentsRect();
    if (pixmap.width() > cr.width() || pixmap.height() > cr.height()) {
        pixmap = pixmap.scaled(cr.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_iconLabel->setPixmap(pixmap);

    Q_EMIT iconChanged(m_icon);
}

void PixmapPreviewSelector::selectCustomStamp()
{
    const QString customStampFile = QFileDialog::getOpenFileName(this, i18nc("@title:window file chooser", "Select custom stamp symbol"), QString(), i18n("*.ico *.png *.xpm *.svg *.svgz | Icon Files (*.ico *.png *.xpm *.svg *.svgz)"));
    if (!customStampFile.isEmpty()) {
        QPixmap pixmap = Okular::AnnotationUtils::loadStamp(customStampFile, m_previewSize);
        if (pixmap.isNull()) {
            KMessageBox::error(this, xi18nc("@info", "Could not load the file <filename>%1</filename>", customStampFile), i18nc("@title:window", "Invalid file"));
        } else {
            m_comboItems->setEditText(customStampFile);
        }
    }
}

AnnotationWidget *AnnotationWidgetFactory::widgetFor(Okular::Annotation *ann)
{
    switch (ann->subType()) {
    case Okular::Annotation::AStamp:
        return new StampAnnotationWidget(ann);
        break;
    case Okular::Annotation::AText:
        return new TextAnnotationWidget(ann);
        break;
    case Okular::Annotation::ALine:
        return new LineAnnotationWidget(ann);
        break;
    case Okular::Annotation::AHighlight:
        return new HighlightAnnotationWidget(ann);
        break;
    case Okular::Annotation::AInk:
        return new InkAnnotationWidget(ann);
        break;
    case Okular::Annotation::AGeom:
        return new GeomAnnotationWidget(ann);
        break;
    case Okular::Annotation::AFileAttachment:
        return new FileAttachmentAnnotationWidget(ann);
        break;
    case Okular::Annotation::ACaret:
        return new CaretAnnotationWidget(ann);
        break;
    // shut up gcc
    default:;
    }
    // cases not covered yet: return a generic widget
    return new AnnotationWidget(ann);
}

AnnotationWidget::AnnotationWidget(Okular::Annotation *ann)
    : m_typeEditable(true)
    , m_ann(ann)
{
}

AnnotationWidget::~AnnotationWidget()
{
}

Okular::Annotation::SubType AnnotationWidget::annotationType() const
{
    return m_ann->subType();
}

QWidget *AnnotationWidget::appearanceWidget()
{
    if (m_appearanceWidget) {
        return m_appearanceWidget;
    }

    m_appearanceWidget = createAppearanceWidget();
    return m_appearanceWidget;
}

QWidget *AnnotationWidget::extraWidget()
{
    if (m_extraWidget) {
        return m_extraWidget;
    }

    m_extraWidget = createExtraWidget();
    return m_extraWidget;
}

void AnnotationWidget::applyChanges()
{
    if (m_colorBn) {
        m_ann->style().setColor(m_colorBn->color());
    }
    if (m_opacity) {
        m_ann->style().setOpacity((double)m_opacity->value() / 100.0);
    }
}

QWidget *AnnotationWidget::createAppearanceWidget()
{
    QWidget *widget = new QWidget();
    QFormLayout *formlayout = new QFormLayout(widget);
    formlayout->setLabelAlignment(Qt::AlignRight);
    formlayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    createStyleWidget(formlayout);

    return widget;
}

void AnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    Q_UNUSED(formlayout);
}

void AnnotationWidget::addColorButton(QWidget *widget, QFormLayout *formlayout)
{
    m_colorBn = new KColorButton(widget);
    m_colorBn->setColor(m_ann->style().color());
    formlayout->addRow(i18n("&Color:"), m_colorBn);
    connect(m_colorBn, &KColorButton::changed, this, &AnnotationWidget::dataChanged);
}

void AnnotationWidget::addOpacitySpinBox(QWidget *widget, QFormLayout *formlayout)
{
    m_opacity = new QSpinBox(widget);
    m_opacity->setRange(0, 100);
    m_opacity->setValue((int)(m_ann->style().opacity() * 100));
    m_opacity->setSuffix(i18nc("Suffix for the opacity level, eg '80%'", "%"));
    formlayout->addRow(i18n("&Opacity:"), m_opacity);
    connect(m_opacity, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnnotationWidget::dataChanged);
}

void AnnotationWidget::addVerticalSpacer(QFormLayout *formlayout)
{
    formlayout->addItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Fixed));
}

QWidget *AnnotationWidget::createExtraWidget()
{
    return nullptr;
}

TextAnnotationWidget::TextAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
{
    m_textAnn = static_cast<Okular::TextAnnotation *>(ann);
}

void TextAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    if (m_textAnn->textType() == Okular::TextAnnotation::Linked) {
        createPopupNoteStyleUi(widget, formlayout);
    } else if (m_textAnn->textType() == Okular::TextAnnotation::InPlace) {
        if (isTypewriter()) {
            createTypewriterStyleUi(widget, formlayout);
        } else {
            createInlineNoteStyleUi(widget, formlayout);
        }
    }
}

void TextAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    if (m_textAnn->textType() == Okular::TextAnnotation::Linked) {
        Q_ASSERT(m_pixmapSelector);
        m_textAnn->setTextIcon(m_pixmapSelector->icon());
    } else if (m_textAnn->textType() == Okular::TextAnnotation::InPlace) {
        Q_ASSERT(m_fontReq);
        m_textAnn->setTextFont(m_fontReq->font());
        if (!isTypewriter()) {
            Q_ASSERT(m_textAlign && m_spinWidth);
            m_textAnn->setInplaceAlignment(m_textAlign->currentIndex());
            m_textAnn->style().setWidth(m_spinWidth->value());
        } else {
            Q_ASSERT(m_textColorBn);
            m_textAnn->setTextColor(m_textColorBn->color());
        }
    }
}

void AnnotationWidget::setAnnotTypeEditable(bool editable)
{
    m_typeEditable = editable;
}

void TextAnnotationWidget::createPopupNoteStyleUi(QWidget *widget, QFormLayout *formlayout)
{
    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);
    addVerticalSpacer(formlayout);
    addPixmapSelector(widget, formlayout);
}

void TextAnnotationWidget::createInlineNoteStyleUi(QWidget *widget, QFormLayout *formlayout)
{
    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);
    addVerticalSpacer(formlayout);
    addFontRequester(widget, formlayout);
    addTextAlignComboBox(widget, formlayout);
    addVerticalSpacer(formlayout);
    addWidthSpinBox(widget, formlayout);
}

void TextAnnotationWidget::createTypewriterStyleUi(QWidget *widget, QFormLayout *formlayout)
{
    addFontRequester(widget, formlayout);
    addTextColorButton(widget, formlayout);
}

void TextAnnotationWidget::addPixmapSelector(QWidget *widget, QFormLayout *formlayout)
{
    m_pixmapSelector = new PixmapPreviewSelector(widget);
    formlayout->addRow(i18n("Icon:"), m_pixmapSelector);
    m_pixmapSelector->addItem(i18n("Comment"), QStringLiteral("Comment"));
    m_pixmapSelector->addItem(i18n("Help"), QStringLiteral("Help"));
    m_pixmapSelector->addItem(i18n("Insert"), QStringLiteral("Insert"));
    m_pixmapSelector->addItem(i18n("Key"), QStringLiteral("Key"));
    m_pixmapSelector->addItem(i18n("New paragraph"), QStringLiteral("NewParagraph"));
    m_pixmapSelector->addItem(i18n("Note"), QStringLiteral("Note"));
    m_pixmapSelector->addItem(i18n("Paragraph"), QStringLiteral("Paragraph"));
    m_pixmapSelector->setIcon(m_textAnn->textIcon());
    connect(m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged);
}

void TextAnnotationWidget::addFontRequester(QWidget *widget, QFormLayout *formlayout)
{
    m_fontReq = new KFontRequester(widget);
    formlayout->addRow(i18n("Font:"), m_fontReq);
    m_fontReq->setFont(m_textAnn->textFont());
    connect(m_fontReq, &KFontRequester::fontSelected, this, &AnnotationWidget::dataChanged);
}

void TextAnnotationWidget::addTextColorButton(QWidget *widget, QFormLayout *formlayout)
{
    m_textColorBn = new KColorButton(widget);
    m_textColorBn->setColor(m_textAnn->textColor());
    formlayout->addRow(i18n("Text &color:"), m_textColorBn);
    connect(m_textColorBn, &KColorButton::changed, this, &AnnotationWidget::dataChanged);
}

void TextAnnotationWidget::addTextAlignComboBox(QWidget *widget, QFormLayout *formlayout)
{
    m_textAlign = new KComboBox(widget);
    formlayout->addRow(i18n("&Align:"), m_textAlign);
    m_textAlign->addItem(i18n("Left"));
    m_textAlign->addItem(i18n("Center"));
    m_textAlign->addItem(i18n("Right"));
    m_textAlign->setCurrentIndex(m_textAnn->inplaceAlignment());
    connect(m_textAlign, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &AnnotationWidget::dataChanged);
}

void TextAnnotationWidget::addWidthSpinBox(QWidget *widget, QFormLayout *formlayout)
{
    m_spinWidth = new QDoubleSpinBox(widget);
    formlayout->addRow(i18n("Border &width:"), m_spinWidth);
    m_spinWidth->setRange(0, 100);
    m_spinWidth->setValue(m_textAnn->style().width());
    m_spinWidth->setSingleStep(0.1);
    connect(m_spinWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnnotationWidget::dataChanged);
}

const QList<QPair<QString, QString>> &StampAnnotationWidget::defaultStamps()
{
    static const QList<QPair<QString, QString>> defaultStampsList = {{i18n("Approved"), QStringLiteral("Approved")},
                                                                     {i18n("As Is"), QStringLiteral("AsIs")},
                                                                     {i18n("Confidential"), QStringLiteral("Confidential")},
                                                                     {i18n("Departmental"), QStringLiteral("Departmental")},
                                                                     {i18n("Draft"), QStringLiteral("Draft")},
                                                                     {i18n("Experimental"), QStringLiteral("Experimental")},
                                                                     {i18n("Expired"), QStringLiteral("Expired")},
                                                                     {i18n("Final"), QStringLiteral("Final")},
                                                                     {i18n("For Comment"), QStringLiteral("ForComment")},
                                                                     {i18n("For Public Release"), QStringLiteral("ForPublicRelease")},
                                                                     {i18n("Not Approved"), QStringLiteral("NotApproved")},
                                                                     {i18n("Not For Public Release"), QStringLiteral("NotForPublicRelease")},
                                                                     {i18n("Sold"), QStringLiteral("Sold")},
                                                                     {i18n("Top Secret"), QStringLiteral("TopSecret")},
                                                                     {i18n("Bookmark"), QStringLiteral("bookmark-new")},
                                                                     {i18n("Information"), QStringLiteral("help-about")},
                                                                     {i18n("KDE"), QStringLiteral("kde")},
                                                                     {i18n("Okular"), QStringLiteral("okular")}};

    return defaultStampsList;
}

StampAnnotationWidget::StampAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
    , m_pixmapSelector(nullptr)
{
    m_stampAnn = static_cast<Okular::StampAnnotation *>(ann);
}

void StampAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    addOpacitySpinBox(widget, formlayout);
    addVerticalSpacer(formlayout);

    m_pixmapSelector = new PixmapPreviewSelector(widget, PixmapPreviewSelector::Below);
    formlayout->addRow(i18n("Stamp symbol:"), m_pixmapSelector);
    m_pixmapSelector->setEditable(true);

    for (const QPair<QString, QString> &pair : defaultStamps()) {
        m_pixmapSelector->addItem(pair.first, pair.second);
    }

    m_pixmapSelector->setIcon(m_stampAnn->stampIconName());
    m_pixmapSelector->setPreviewSize(64);

    connect(m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged);
}

void StampAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_stampAnn->setStampIconName(m_pixmapSelector->icon());
}

LineAnnotationWidget::LineAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
{
    m_lineAnn = static_cast<Okular::LineAnnotation *>(ann);
    if (m_lineAnn->linePoints().count() == 2) {
        m_lineType = 0; // line
    } else if (m_lineAnn->lineClosed()) {
        m_lineType = 1; // polygon
    } else {
        m_lineType = 2; // polyline
    }
}

void LineAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);

    m_spinSize = new QDoubleSpinBox(widget);
    m_spinSize->setRange(1, 100);
    m_spinSize->setValue(m_lineAnn->style().width());

    connect(m_spinSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineAnnotationWidget::dataChanged);

    // Straight line
    if (m_lineType == 0) {
        addVerticalSpacer(formlayout);
        formlayout->addRow(i18n("&Width:"), m_spinSize);

        // Line Term Styles
        addVerticalSpacer(formlayout);
        m_startStyleCombo = new QComboBox(widget);
        formlayout->addRow(i18n("Line start:"), m_startStyleCombo);
        m_endStyleCombo = new QComboBox(widget);
        formlayout->addRow(i18n("Line end:"), m_endStyleCombo);
        // FIXME: Where does the tooltip goes??

        const QList<QPair<Okular::LineAnnotation::TermStyle, QString>> termStyles {{Okular::LineAnnotation::Square, i18n("Square")},
                                                                                   {Okular::LineAnnotation::Circle, i18n("Circle")},
                                                                                   {Okular::LineAnnotation::Diamond, i18n("Diamond")},
                                                                                   {Okular::LineAnnotation::OpenArrow, i18n("Open Arrow")},
                                                                                   {Okular::LineAnnotation::ClosedArrow, i18n("Closed Arrow")},
                                                                                   {Okular::LineAnnotation::None, i18n("None")},
                                                                                   {Okular::LineAnnotation::Butt, i18n("Butt")},
                                                                                   {Okular::LineAnnotation::ROpenArrow, i18n("Right Open Arrow")},
                                                                                   {Okular::LineAnnotation::RClosedArrow, i18n("Right Closed Arrow")},
                                                                                   {Okular::LineAnnotation::Slash, i18n("Slash")}};
        for (const auto &item : termStyles) {
            const QIcon icon = endStyleIcon(item.first, QGuiApplication::palette().color(QPalette::WindowText));
            m_startStyleCombo->addItem(icon, item.second);
            m_endStyleCombo->addItem(icon, item.second);
        }

        m_startStyleCombo->setCurrentIndex(m_lineAnn->lineStartStyle());
        m_endStyleCombo->setCurrentIndex(m_lineAnn->lineEndStyle());

        // Leaders lengths
        addVerticalSpacer(formlayout);
        m_spinLL = new QDoubleSpinBox(widget);
        formlayout->addRow(i18n("Leader line length:"), m_spinLL);
        m_spinLLE = new QDoubleSpinBox(widget);
        formlayout->addRow(i18n("Leader line extensions length:"), m_spinLLE);

        m_spinLL->setRange(-500, 500);
        m_spinLL->setValue(m_lineAnn->lineLeadingForwardPoint());
        m_spinLLE->setRange(0, 500);
        m_spinLLE->setValue(m_lineAnn->lineLeadingBackwardPoint());

        connect(m_startStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineAnnotationWidget::dataChanged);
        connect(m_endStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineAnnotationWidget::dataChanged);
        connect(m_spinLL, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineAnnotationWidget::dataChanged);
        connect(m_spinLLE, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineAnnotationWidget::dataChanged);
    } else if (m_lineType == 1) // Polygon
    {
        QHBoxLayout *colorlay = new QHBoxLayout();
        m_useColor = new QCheckBox(i18n("Enabled"), widget);
        colorlay->addWidget(m_useColor);
        m_innerColor = new KColorButton(widget);
        colorlay->addWidget(m_innerColor);
        formlayout->addRow(i18n("Shape fill:"), colorlay);

        m_innerColor->setColor(m_lineAnn->lineInnerColor());
        if (m_lineAnn->lineInnerColor().isValid()) {
            m_useColor->setChecked(true);
        } else {
            m_innerColor->setEnabled(false);
        }

        addVerticalSpacer(formlayout);
        formlayout->addRow(i18n("&Width:"), m_spinSize);

        connect(m_innerColor, &KColorButton::changed, this, &AnnotationWidget::dataChanged);
        connect(m_useColor, &QAbstractButton::toggled, this, &AnnotationWidget::dataChanged);
        connect(m_useColor, &QCheckBox::toggled, m_innerColor, &KColorButton::setEnabled);
    }
}

void LineAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    if (m_lineType == 0) {
        Q_ASSERT(m_spinLL && m_spinLLE && m_startStyleCombo && m_endStyleCombo);
        m_lineAnn->setLineLeadingForwardPoint(m_spinLL->value());
        m_lineAnn->setLineLeadingBackwardPoint(m_spinLLE->value());
        m_lineAnn->setLineStartStyle((Okular::LineAnnotation::TermStyle)m_startStyleCombo->currentIndex());
        m_lineAnn->setLineEndStyle((Okular::LineAnnotation::TermStyle)m_endStyleCombo->currentIndex());
    } else if (m_lineType == 1) {
        Q_ASSERT(m_useColor && m_innerColor);
        if (!m_useColor->isChecked()) {
            m_lineAnn->setLineInnerColor(QColor());
        } else {
            m_lineAnn->setLineInnerColor(m_innerColor->color());
        }
    }
    Q_ASSERT(m_spinSize);
    m_lineAnn->style().setWidth(m_spinSize->value());
}

QIcon LineAnnotationWidget::endStyleIcon(Okular::LineAnnotation::TermStyle endStyle, const QColor &lineColor)
{
    const int iconSize {48};
    QImage image {iconSize, iconSize, QImage::Format_ARGB32};
    image.fill(qRgba(0, 0, 0, 0));
    Okular::LineAnnotation prototype;
    prototype.setLinePoints({{0, 0.5}, {0.65, 0.5}});
    prototype.setLineStartStyle(Okular::LineAnnotation::TermStyle::None);
    prototype.setLineEndStyle(endStyle);
    prototype.style().setWidth(4);
    prototype.style().setColor(lineColor);
    prototype.style().setLineStyle(Okular::Annotation::LineStyle::Solid);
    prototype.setBoundingRectangle({0, 0, 1, 1});
    LineAnnotPainter linepainter {&prototype, QSize {iconSize, iconSize}, 1, QTransform()};
    linepainter.draw(image);
    return QIcon(QPixmap::fromImage(image));
}

InkAnnotationWidget::InkAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
{
    m_inkAnn = static_cast<Okular::InkAnnotation *>(ann);
}

void InkAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);

    addVerticalSpacer(formlayout);

    m_spinSize = new QDoubleSpinBox(widget);
    formlayout->addRow(i18n("&Width:"), m_spinSize);

    m_spinSize->setRange(1, 100);
    m_spinSize->setValue(m_inkAnn->style().width());

    connect(m_spinSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnnotationWidget::dataChanged);
}

void InkAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_inkAnn->style().setWidth(m_spinSize->value());
}

HighlightAnnotationWidget::HighlightAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
{
    m_hlAnn = static_cast<Okular::HighlightAnnotation *>(ann);
}

void HighlightAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    m_typeCombo = new KComboBox(widget);
    m_typeCombo->setVisible(m_typeEditable);
    if (m_typeEditable) {
        formlayout->addRow(i18n("Type:"), m_typeCombo);
    }
    m_typeCombo->addItem(i18n("Highlight"));
    m_typeCombo->addItem(i18n("Squiggle"));
    m_typeCombo->addItem(i18n("Underline"));
    m_typeCombo->addItem(i18n("Strike out"));
    m_typeCombo->setCurrentIndex(m_hlAnn->highlightType());

    addVerticalSpacer(formlayout);
    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);

    connect(m_typeCombo, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &AnnotationWidget::dataChanged);
}

void HighlightAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_hlAnn->setHighlightType((Okular::HighlightAnnotation::HighlightType)m_typeCombo->currentIndex());
}

GeomAnnotationWidget::GeomAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
{
    m_geomAnn = static_cast<Okular::GeomAnnotation *>(ann);
}

void GeomAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    m_typeCombo = new KComboBox(widget);
    m_typeCombo->setVisible(m_typeEditable);
    if (m_typeEditable) {
        formlayout->addRow(i18n("Type:"), m_typeCombo);
    }
    addVerticalSpacer(formlayout);
    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);
    QHBoxLayout *colorlay = new QHBoxLayout();
    m_useColor = new QCheckBox(i18n("Enabled"), widget);
    colorlay->addWidget(m_useColor);
    m_innerColor = new KColorButton(widget);
    colorlay->addWidget(m_innerColor);
    formlayout->addRow(i18n("Shape fill:"), colorlay);
    addVerticalSpacer(formlayout);
    m_spinSize = new QDoubleSpinBox(widget);
    formlayout->addRow(i18n("&Width:"), m_spinSize);

    m_typeCombo->addItem(i18n("Rectangle"));
    m_typeCombo->addItem(i18n("Ellipse"));
    m_typeCombo->setCurrentIndex(m_geomAnn->geometricalType());
    m_innerColor->setColor(m_geomAnn->geometricalInnerColor());
    if (m_geomAnn->geometricalInnerColor().isValid()) {
        m_useColor->setChecked(true);
    } else {
        m_innerColor->setEnabled(false);
    }
    m_spinSize->setRange(0, 100);
    m_spinSize->setValue(m_geomAnn->style().width());

    connect(m_typeCombo, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &AnnotationWidget::dataChanged);
    connect(m_innerColor, &KColorButton::changed, this, &AnnotationWidget::dataChanged);
    connect(m_useColor, &QAbstractButton::toggled, this, &AnnotationWidget::dataChanged);
    connect(m_useColor, &QCheckBox::toggled, m_innerColor, &KColorButton::setEnabled);
    connect(m_spinSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnnotationWidget::dataChanged);
}

void GeomAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_geomAnn->setGeometricalType((Okular::GeomAnnotation::GeomType)m_typeCombo->currentIndex());
    if (!m_useColor->isChecked()) {
        m_geomAnn->setGeometricalInnerColor(QColor());
    } else {
        m_geomAnn->setGeometricalInnerColor(m_innerColor->color());
    }
    m_geomAnn->style().setWidth(m_spinSize->value());
}

FileAttachmentAnnotationWidget::FileAttachmentAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
    , m_pixmapSelector(nullptr)
{
    m_attachAnn = static_cast<Okular::FileAttachmentAnnotation *>(ann);
}

void FileAttachmentAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    addOpacitySpinBox(widget, formlayout);

    m_pixmapSelector = new PixmapPreviewSelector(widget);
    formlayout->addRow(i18n("File attachment symbol:"), m_pixmapSelector);
    m_pixmapSelector->setEditable(true);

    m_pixmapSelector->addItem(i18nc("Symbol for file attachment annotations", "Graph"), QStringLiteral("graph"));
    m_pixmapSelector->addItem(i18nc("Symbol for file attachment annotations", "Push Pin"), QStringLiteral("pushpin"));
    m_pixmapSelector->addItem(i18nc("Symbol for file attachment annotations", "Paperclip"), QStringLiteral("paperclip"));
    m_pixmapSelector->addItem(i18nc("Symbol for file attachment annotations", "Tag"), QStringLiteral("tag"));
    m_pixmapSelector->setIcon(m_attachAnn->fileIconName());

    connect(m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged);
}

QWidget *FileAttachmentAnnotationWidget::createExtraWidget()
{
    QWidget *widget = new QWidget();
    widget->setWindowTitle(i18nc("'File' as normal file, that can be opened, saved, etc..", "File"));

    Okular::EmbeddedFile *ef = m_attachAnn->embeddedFile();
    const int size = ef->size();
    const QString sizeString = size <= 0 ? i18nc("Not available size", "N/A") : KFormat().formatByteSize(size);
    const QString descString = ef->description().isEmpty() ? i18n("No description available.") : ef->description();

    QHBoxLayout *mainLay = new QHBoxLayout(widget);
    QFormLayout *lay = new QFormLayout();
    mainLay->addLayout(lay);

    QLabel *tmplabel = new QLabel(ef->name(), widget);
    tmplabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addRow(i18n("Name:"), tmplabel);

    tmplabel = new QLabel(sizeString, widget);
    tmplabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addRow(i18n("&Width:"), tmplabel);

    tmplabel = new QLabel(widget);
    tmplabel->setTextFormat(Qt::PlainText);
    tmplabel->setWordWrap(true);
    tmplabel->setText(descString);
    tmplabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addRow(i18n("Description:"), tmplabel);

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(ef->name(), QMimeDatabase::MatchExtension);
    if (mime.isValid()) {
        tmplabel = new QLabel(widget);
        tmplabel->setPixmap(QIcon::fromTheme(mime.iconName()).pixmap(FILEATTACH_ICONSIZE, FILEATTACH_ICONSIZE));
        tmplabel->setFixedSize(FILEATTACH_ICONSIZE, FILEATTACH_ICONSIZE);
        QVBoxLayout *tmpLayout = new QVBoxLayout(widget);
        tmpLayout->setAlignment(Qt::AlignTop);
        mainLay->addLayout(tmpLayout);
        tmpLayout->addWidget(tmplabel);
    }

    return widget;
}

void FileAttachmentAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_attachAnn->setFileIconName(m_pixmapSelector->icon());
}

static QString caretSymbolToIcon(Okular::CaretAnnotation::CaretSymbol symbol)
{
    switch (symbol) {
    case Okular::CaretAnnotation::None:
        return QStringLiteral("caret-none");
    case Okular::CaretAnnotation::P:
        return QStringLiteral("caret-p");
    }
    return QString();
}

static Okular::CaretAnnotation::CaretSymbol caretSymbolFromIcon(const QString &icon)
{
    if (icon == QLatin1String("caret-none")) {
        return Okular::CaretAnnotation::None;
    } else if (icon == QLatin1String("caret-p")) {
        return Okular::CaretAnnotation::P;
    }
    return Okular::CaretAnnotation::None;
}

CaretAnnotationWidget::CaretAnnotationWidget(Okular::Annotation *ann)
    : AnnotationWidget(ann)
    , m_pixmapSelector(nullptr)
{
    m_caretAnn = static_cast<Okular::CaretAnnotation *>(ann);
}

void CaretAnnotationWidget::createStyleWidget(QFormLayout *formlayout)
{
    QWidget *widget = qobject_cast<QWidget *>(formlayout->parent());

    addColorButton(widget, formlayout);
    addOpacitySpinBox(widget, formlayout);

    m_pixmapSelector = new PixmapPreviewSelector(widget);
    formlayout->addRow(i18n("Caret symbol:"), m_pixmapSelector);

    m_pixmapSelector->addItem(i18nc("Symbol for caret annotations", "None"), QStringLiteral("caret-none"));
    m_pixmapSelector->addItem(i18nc("Symbol for caret annotations", "P"), QStringLiteral("caret-p"));
    m_pixmapSelector->setIcon(caretSymbolToIcon(m_caretAnn->caretSymbol()));

    connect(m_pixmapSelector, &PixmapPreviewSelector::iconChanged, this, &AnnotationWidget::dataChanged);
}

void CaretAnnotationWidget::applyChanges()
{
    AnnotationWidget::applyChanges();
    m_caretAnn->setCaretSymbol(caretSymbolFromIcon(m_pixmapSelector->icon()));
}

#include "moc_annotationwidgets.cpp"
