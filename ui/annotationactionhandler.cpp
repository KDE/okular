/**************************************************************************
 *   Copyright (C) 2019 by Simone Gaiarin <simgunz@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 **************************************************************************/

#include "annotationactionhandler.h"

// qt includes
#include <QBitmap>
#include <QColorDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QPainter>
#include <QPen>

// kde includes
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KParts/MainWindow>
#include <KSelectAction>
#include <KToolBar>

// local includes
#include "annotationwidgets.h"
#include "guiutils.h"
#include "pageview.h"
#include "pageviewannotator.h"
#include "toggleactionmenu.h"

class AnnotationActionHandlerPrivate
{
public:
    enum class AnnotationColor { Color, InnerColor };
    static const QList<QPair<QString, QColor>> defaultColors;
    static const QList<double> widthStandardValues;
    static const QList<double> opacityStandardValues;

    explicit AnnotationActionHandlerPrivate(AnnotationActionHandler *qq)
        : q(qq)
        , annotator(nullptr)
        , textTools(nullptr)
        , textQuickTools(nullptr)
        , agTools(nullptr)
        , agLastAction(nullptr)
        , aQuickTools(nullptr)
        , aGeomShapes(nullptr)
        , aStamp(nullptr)
        , aAddToQuickTools(nullptr)
        , aContinuousMode(nullptr)
        , aConstrainRatioAndAngle(nullptr)
        , aWidth(nullptr)
        , aColor(nullptr)
        , aInnerColor(nullptr)
        , aOpacity(nullptr)
        , aFont(nullptr)
        , aAdvancedSettings(nullptr)
        , aHideToolBar(nullptr)
        , aShowToolBar(nullptr)
        , aCustomStamp(nullptr)
        , aCustomWidth(nullptr)
        , aCustomOpacity(nullptr)
        , currentColor(QColor())
        , currentInnerColor(QColor())
        , currentFont(QFont())
        , currentWidth(-1)
        , selectedTool(-1)
        , textToolsEnabled(false)
    {
    }

    QAction *selectActionItem(KSelectAction *aList, QAction *aCustomCurrent, double value, const QList<double> &defaultValues, const QIcon &icon, const QString &label);
    /**
     * @short Adds a custom stamp annotation action to the stamp list when the stamp is not a default stamp
     *
     * When stampIconName cannot be found among the default stamps, this method creates a new action
     * for the custom stamp annotation and adds it to the stamp action combo box. If a custom action
     * is already present in the list, it is removed before adding the new custom action. If stampIconName
     * matches a default stamp, any existing stamp annotation action is removed.
     */
    void maybeUpdateCustomStampAction(const QString &stampIconName);
    void parseTool(int toolID);

    void updateConfigActions(const QString &annotType = QLatin1String(""));
    void populateQuickAnnotations();
    KSelectAction *colorPickerAction(AnnotationColor colorType);

    const QIcon widthIcon(double width);
    const QIcon stampIcon(const QString &stampIconName);

    void selectTool(int toolID);
    void slotStampToolSelected(const QString &stamp);
    void slotQuickToolSelected(int favToolID);
    void slotSetColor(AnnotationColor colorType, const QColor &color = QColor());
    void slotSelectAnnotationFont();
    void slotToolBarVisibilityChanged(bool checked);

    AnnotationActionHandler *q;

    PageViewAnnotator *annotator;

    QList<QAction *> *textTools;
    QList<QAction *> *textQuickTools;
    QActionGroup *agTools;
    QAction *agLastAction;

    KSelectAction *aQuickTools;
    ToggleActionMenu *aGeomShapes;
    ToggleActionMenu *aStamp;
    QAction *aAddToQuickTools;
    KToggleAction *aContinuousMode;
    KToggleAction *aConstrainRatioAndAngle;
    KSelectAction *aWidth;
    KSelectAction *aColor;
    KSelectAction *aInnerColor;
    KSelectAction *aOpacity;
    QAction *aFont;
    QAction *aAdvancedSettings;
    QAction *aHideToolBar;
    KToggleAction *aShowToolBar;

    QAction *aCustomStamp;
    QAction *aCustomWidth;
    QAction *aCustomOpacity;

    QColor currentColor;
    QColor currentInnerColor;
    QFont currentFont;
    int currentWidth;

    int selectedTool;
    bool textToolsEnabled;
};

const QList<QPair<QString, QColor>> AnnotationActionHandlerPrivate::defaultColors = {{i18n("Red"), Qt::red},
                                                                                     {i18n("Orange"), QColor(255, 85, 0)},
                                                                                     {i18n("Yellow"), Qt::yellow},
                                                                                     {i18n("Green"), Qt::green},
                                                                                     {i18n("Cyan"), Qt::cyan},
                                                                                     {i18n("Blue"), Qt::blue},
                                                                                     {i18n("Magenta"), Qt::magenta},
                                                                                     {i18n("White"), Qt::white},
                                                                                     {i18n("Gray"), Qt::gray},
                                                                                     {i18n("Black"), Qt::black}

};

const QList<double> AnnotationActionHandlerPrivate::widthStandardValues = {1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5};

const QList<double> AnnotationActionHandlerPrivate::opacityStandardValues = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};

QAction *AnnotationActionHandlerPrivate::selectActionItem(KSelectAction *aList, QAction *aCustomCurrent, double value, const QList<double> &defaultValues, const QIcon &icon, const QString &label)
{
    if (aCustomCurrent) {
        aList->removeAction(aCustomCurrent);
        delete aCustomCurrent;
    }
    QAction *aCustom = nullptr;
    const int defaultValueIdx = defaultValues.indexOf(value);
    if (defaultValueIdx >= 0) {
        aList->setCurrentItem(defaultValueIdx);
    } else {
        aCustom = new KToggleAction(icon, label, q);
        const int aBeforeIdx = std::lower_bound(defaultValues.begin(), defaultValues.end(), value) - defaultValues.begin();
        QAction *aBefore = aBeforeIdx < defaultValues.size() ? aList->actions().at(aBeforeIdx) : nullptr;
        aList->insertAction(aBefore, aCustom);
        aList->setCurrentAction(aCustom);
    }
    return aCustom;
}

void AnnotationActionHandlerPrivate::maybeUpdateCustomStampAction(const QString &stampIconName)
{
    auto it = std::find_if(StampAnnotationWidget::defaultStamps.begin(), StampAnnotationWidget::defaultStamps.end(), [&stampIconName](const QPair<QString, QString> &element) { return element.second == stampIconName; });
    bool defaultStamp = it != StampAnnotationWidget::defaultStamps.end();

    if (aCustomStamp) {
        aStamp->removeAction(aCustomStamp);
        agTools->removeAction(aCustomStamp);
        delete aCustomStamp;
        aCustomStamp = nullptr;
    }
    if (!defaultStamp) {
        QFileInfo info(stampIconName);
        QString stampActionName = info.fileName();
        aCustomStamp = new KToggleAction(stampIcon(stampIconName), stampActionName, q);
        aStamp->addAction(aCustomStamp);
        aStamp->setDefaultAction(aCustomStamp);
        agTools->addAction(aCustomStamp);
        aCustomStamp->setChecked(true);
        QObject::connect(aCustomStamp, &QAction::triggered, q, [this, stampIconName]() { slotStampToolSelected(stampIconName); });
    }
}

void AnnotationActionHandlerPrivate::parseTool(int toolID)
{
    if (toolID == -1) {
        updateConfigActions();
        return;
    }

    QDomElement toolElement = annotator->builtinTool(toolID);
    const QString annotType = toolElement.attribute(QStringLiteral("type"));
    QDomElement engineElement = toolElement.firstChildElement(QStringLiteral("engine"));
    QDomElement annElement = engineElement.firstChildElement(QStringLiteral("annotation"));

    QColor color, innerColor, textColor;
    if (annElement.hasAttribute(QStringLiteral("color"))) {
        color = QColor(annElement.attribute(QStringLiteral("color")));
    }
    if (annElement.hasAttribute(QStringLiteral("innerColor"))) {
        innerColor = QColor(annElement.attribute(QStringLiteral("innerColor")));
    }
    if (annElement.hasAttribute(QStringLiteral("textColor"))) {
        textColor = QColor(annElement.attribute(QStringLiteral("textColor")));
    }
    if (textColor.isValid()) {
        currentColor = textColor;
        currentInnerColor = color;
    } else {
        currentColor = color;
        currentInnerColor = innerColor;
    }

    if (annElement.hasAttribute(QStringLiteral("font"))) {
        currentFont.fromString(annElement.attribute(QStringLiteral("font")));
    }

    // if the width value is not a default one, insert a new action in the width list
    if (annElement.hasAttribute(QStringLiteral("width"))) {
        double width = annElement.attribute(QStringLiteral("width")).toDouble();
        aCustomWidth = selectActionItem(aWidth, aCustomWidth, width, widthStandardValues, widthIcon(width), i18nc("@item:inlistbox", "Width %1", width));
    }

    // if the opacity value is not a default one, insert a new action in the opacity list
    if (annElement.hasAttribute(QStringLiteral("opacity"))) {
        double opacity = annElement.attribute(QStringLiteral("opacity")).toDouble();
        aCustomOpacity = selectActionItem(aOpacity, aCustomOpacity, opacity, opacityStandardValues, GuiUtils::createOpacityIcon(opacity), i18nc("@item:inlistbox", "%1\%", opacity * 100));
    } else {
        aOpacity->setCurrentItem(opacityStandardValues.size() - 1); // 100 %
    }

    // if the tool is a custom stamp, insert a new action in the stamp list
    if (annotType == QStringLiteral("stamp")) {
        QString stampIconName = annElement.attribute(QStringLiteral("icon"));
        maybeUpdateCustomStampAction(stampIconName);
    }

    updateConfigActions(annotType);
}

void AnnotationActionHandlerPrivate::updateConfigActions(const QString &annotType)
{
    const bool isAnnotationSelected = !annotType.isEmpty();
    const bool isTypewriter = annotType == QStringLiteral("typewriter");
    const bool isInlineNote = annotType == QStringLiteral("note-inline");
    const bool isText = isInlineNote || isTypewriter;
    const bool isPolygon = annotType == QStringLiteral("polygon");
    const bool isShape = annotType == QStringLiteral("rectangle") || annotType == QStringLiteral("ellipse") || isPolygon;
    const bool isStraightLine = annotType == QStringLiteral("straight-line");
    const bool isLine = annotType == QStringLiteral("ink") || isStraightLine;
    const bool isStamp = annotType == QStringLiteral("stamp");

    if (isTypewriter) {
        aColor->setIcon(GuiUtils::createColorIcon({currentColor}, QIcon::fromTheme(QStringLiteral("format-text-color"))));
    } else {
        aColor->setIcon(GuiUtils::createColorIcon({currentColor}, QIcon::fromTheme(QStringLiteral("format-stroke-color"))));
    }
    aInnerColor->setIcon(GuiUtils::createColorIcon({currentInnerColor}, QIcon::fromTheme(QStringLiteral("format-fill-color"))));

    aAddToQuickTools->setEnabled(isAnnotationSelected);
    aWidth->setEnabled(isLine || isShape);
    aColor->setEnabled(isAnnotationSelected && !isStamp);
    aInnerColor->setEnabled(isShape);
    aOpacity->setEnabled(isAnnotationSelected);
    aFont->setEnabled(isText);
    aConstrainRatioAndAngle->setEnabled(isStraightLine || isShape);
    aAdvancedSettings->setEnabled(isAnnotationSelected);

    // set tooltips
    if (!isAnnotationSelected) {
        aWidth->setToolTip(i18nc("@info:tooltip", "Annotation line width (No annotation selected)"));
        aColor->setToolTip(i18nc("@info:tooltip", "Annotation color (No annotation selected)"));
        aInnerColor->setToolTip(i18nc("@info:tooltip", "Annotation fill color (No annotation selected)"));
        aOpacity->setToolTip(i18nc("@info:tooltip", "Annotation opacity (No annotation selected)"));
        aFont->setToolTip(i18nc("@info:tooltip", "Annotation font (No annotation selected)"));
        aAddToQuickTools->setToolTip(i18nc("@info:tooltip", "Add the current annotation to the quick annotations menu (No annotation selected)"));
        aConstrainRatioAndAngle->setToolTip(i18nc("@info:tooltip", "Constrain shape ratio to 1:1 or line angle to 15° steps (No annotation selected)"));
        aAdvancedSettings->setToolTip(i18nc("@info:tooltip", "Advanced settings for the current annotation tool (No annotation selected)"));
        return;
    }

    if (isLine || isShape) {
        aWidth->setToolTip(i18nc("@info:tooltip", "Annotation line width"));
    } else {
        aWidth->setToolTip(i18nc("@info:tooltip", "Annotation line width (Current annotation has no line width)"));
    }

    if (isTypewriter) {
        aColor->setToolTip(i18nc("@info:tooltip", "Annotation text color"));
    } else if (isShape) {
        aColor->setToolTip(i18nc("@info:tooltip", "Annotation border color"));
    } else {
        aColor->setToolTip(i18nc("@info:tooltip", "Annotation color"));
    }

    if (isShape) {
        aInnerColor->setToolTip(i18nc("@info:tooltip", "Annotation fill color"));
    } else {
        aInnerColor->setToolTip(i18nc("@info:tooltip", "Annotation fill color (Current annotation has no fill color)"));
    }

    if (isText) {
        aFont->setToolTip(i18nc("@info:tooltip", "Annotation font"));
    } else {
        aFont->setToolTip(i18nc("@info:tooltip", "Annotation font (Current annotation has no font)"));
    }

    if (isStraightLine || isPolygon) {
        aConstrainRatioAndAngle->setToolTip(i18nc("@info:tooltip", "Constrain line angle to 15° steps"));
    } else if (isShape) {
        aConstrainRatioAndAngle->setToolTip(i18nc("@info:tooltip", "Constrain shape ratio to 1:1"));
    } else {
        aConstrainRatioAndAngle->setToolTip(i18nc("@info:tooltip", "Constrain shape ratio to 1:1 or line angle to 15° steps (Not supported by current annotation)"));
    }

    aOpacity->setToolTip(i18nc("@info:tooltip", "Annotation opacity"));
    aAddToQuickTools->setToolTip(i18nc("@info:tooltip", "Add the current annotation to the quick annotations menu"));
    aAdvancedSettings->setToolTip(i18nc("@info:tooltip", "Advanced settings for the current annotation tool"));
}

void AnnotationActionHandlerPrivate::populateQuickAnnotations()
{
    if (!aQuickTools->isEnabled()) {
        return;
    }

    const QList<int> numberKeys = {Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_0};

    textQuickTools->clear();
    aQuickTools->removeAllActions();

    int favToolId = 1;
    QList<int>::const_iterator shortcutNumber = numberKeys.begin();
    QDomElement favToolElement = annotator->quickTool(favToolId);
    while (!favToolElement.isNull()) {
        QString itemText = favToolElement.attribute(QStringLiteral("name"));
        if (itemText.isEmpty()) {
            itemText = PageViewAnnotator::defaultToolName(favToolElement);
        }
        QIcon toolIcon = QIcon(PageViewAnnotator::makeToolPixmap(favToolElement));
        QAction *annFav = new QAction(toolIcon, itemText, q);
        aQuickTools->addAction(annFav);
        if (shortcutNumber != numberKeys.end())
            annFav->setShortcut(QKeySequence(Qt::ALT + *(shortcutNumber++)));
        QObject::connect(annFav, &QAction::triggered, q, [this, favToolId]() { slotQuickToolSelected(favToolId); });

        QDomElement engineElement = favToolElement.firstChildElement(QStringLiteral("engine"));
        if (engineElement.attribute(QStringLiteral("type")) == QStringLiteral("TextSelector")) {
            textQuickTools->append(annFav);
            annFav->setEnabled(textToolsEnabled);
        }

        favToolElement = annotator->quickTool(++favToolId);
    }
    QAction *separator = new QAction();
    separator->setSeparator(true);
    aQuickTools->addAction(separator);
    // add action to open "Configure Annotation" settings dialog
    KActionCollection *ac = qobject_cast<PageView *>(q->parent()->parent())->actionCollection();
    QAction *aConfigAnnotation = ac->action(QStringLiteral("options_configure_annotations"));
    if (aConfigAnnotation) {
        aQuickTools->addAction(aConfigAnnotation);
    }
}

KSelectAction *AnnotationActionHandlerPrivate::colorPickerAction(AnnotationColor colorType)
{
    auto colorList = defaultColors;
    QString aText(i18nc("@action:intoolbar Current annotation config option", "Color"));
    if (colorType == AnnotationColor::InnerColor) {
        aText = i18nc("@action:intoolbar Current annotation config option", "Fill Color");
        colorList.append(QPair<QString, Qt::GlobalColor>(QStringLiteral("Transparent"), Qt::transparent));
    }
    KSelectAction *aColorPicker = new KSelectAction(QIcon(), aText, q);
    aColorPicker->setToolBarMode(KSelectAction::MenuMode);
    for (const auto &colorNameValue : colorList) {
        QColor color(colorNameValue.second);
        QAction *aColor = new QAction(GuiUtils::createColorIcon({color}, QIcon(), GuiUtils::VisualizeTransparent), i18nc("@item:inlistbox Color name", "%1", colorNameValue.first), q);
        aColorPicker->addAction(aColor);
        QObject::connect(aColor, &QAction::triggered, q, [this, colorType, color]() { slotSetColor(colorType, color); });
    }
    QAction *aCustomColor = new QAction(QIcon::fromTheme(QStringLiteral("color-picker")), i18nc("@item:inlistbox", "Custom Color..."), q);
    aColorPicker->addAction(aCustomColor);
    QObject::connect(aCustomColor, &QAction::triggered, q, [this, colorType]() { slotSetColor(colorType); });
    return aColorPicker;
}

const QIcon AnnotationActionHandlerPrivate::widthIcon(double width)
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black, 2 * width, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(0, pm.height() / 2, pm.width(), pm.height() / 2);
    p.end();
    return QIcon(pm);
}

const QIcon AnnotationActionHandlerPrivate::stampIcon(const QString &stampIconName)
{
    QPixmap stampPix = GuiUtils::loadStamp(stampIconName, 32);
    if (stampPix.width() == stampPix.height())
        return QIcon(stampPix);
    else
        return QIcon::fromTheme(QStringLiteral("tag"));
}

void AnnotationActionHandlerPrivate::selectTool(int toolID)
{
    selectedTool = toolID;
    annotator->selectTool(toolID);
    parseTool(toolID);
}

void AnnotationActionHandlerPrivate::slotStampToolSelected(const QString &stamp)
{
    KMessageBox::information(nullptr, i18nc("@info", "Stamps inserted in PDF documents are not visible in PDF readers other than Okular"), i18nc("@title:window", "Experimental feature"), QStringLiteral("stampAnnotationWarning"));
    selectedTool = PageViewAnnotator::STAMP_TOOL_ID;
    annotator->selectStampTool(stamp); // triggers a reparsing thus calling parseTool
}

void AnnotationActionHandlerPrivate::slotQuickToolSelected(int favToolID)
{
    int toolID = annotator->setQuickTool(favToolID); // always triggers an unuseful reparsing
    if (toolID == -1) {
        qWarning("Corrupted configuration for quick annotation tool with id: %d", favToolID);
        return;
    }
    int indexOfActionInGroup = toolID - 1;
    if (toolID == PageViewAnnotator::STAMP_TOOL_ID) {
        // if the quick tool is a stamp we need to find its corresponding built-in tool action and select it
        QDomElement favToolElement = annotator->quickTool(favToolID);
        QDomElement engineElement = favToolElement.firstChildElement(QStringLiteral("engine"));
        QDomElement annotationElement = engineElement.firstChildElement(QStringLiteral("annotation"));
        QString stampIconName = annotationElement.attribute(QStringLiteral("icon"));

        auto it = std::find_if(StampAnnotationWidget::defaultStamps.begin(), StampAnnotationWidget::defaultStamps.end(), [&stampIconName](const QPair<QString, QString> &element) { return element.second == stampIconName; });
        if (it != StampAnnotationWidget::defaultStamps.end()) {
            int stampActionIndex = std::distance(StampAnnotationWidget::defaultStamps.begin(), it);
            indexOfActionInGroup = PageViewAnnotator::STAMP_TOOL_ID + stampActionIndex - 1;
        } else {
            maybeUpdateCustomStampAction(stampIconName);
            indexOfActionInGroup = agTools->actions().size() - 1;
        }
    }
    QAction *favToolAction = agTools->actions().at(indexOfActionInGroup);
    if (!favToolAction->isChecked()) {
        // action group workaround: activates the action slot calling selectTool
        //                          when new tool if different from the selected one
        favToolAction->trigger();
    } else {
        selectTool(toolID);
    }
    aShowToolBar->setChecked(true);
}

void AnnotationActionHandlerPrivate::slotSetColor(AnnotationColor colorType, const QColor &color)
{
    QColor selectedColor(color);
    if (!selectedColor.isValid()) {
        selectedColor = QColorDialog::getColor(currentColor, nullptr, i18nc("@title:window", "Select color"));
        if (!selectedColor.isValid()) {
            return;
        }
    }
    if (colorType == AnnotationColor::Color) {
        currentColor = selectedColor;
        annotator->setAnnotationColor(selectedColor);
    } else if (colorType == AnnotationColor::InnerColor) {
        currentInnerColor = selectedColor;
        annotator->setAnnotationInnerColor(selectedColor);
    }
}

void AnnotationActionHandlerPrivate::slotSelectAnnotationFont()
{
    bool ok;
    QFont selectedFont = QFontDialog::getFont(&ok, currentFont);
    if (ok) {
        currentFont = selectedFont;
        annotator->setAnnotationFont(currentFont);
    }
}

void AnnotationActionHandlerPrivate::slotToolBarVisibilityChanged(bool checked)
{
    if (!checked) {
        q->deselectAllAnnotationActions();
    }
}

AnnotationActionHandler::AnnotationActionHandler(PageViewAnnotator *parent, KActionCollection *ac)
    : QObject(parent)
    , d(new AnnotationActionHandlerPrivate(this))
{
    d->annotator = parent;

    // toolbar visibility actions
    d->aShowToolBar = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-freehand")), i18n("&Annotations"), this);
    d->aHideToolBar = new QAction(QIcon::fromTheme(QStringLiteral("dialog-close")), i18nc("@action:intoolbar Hide the toolbar", "Hide"), this);
    connect(d->aHideToolBar, &QAction::triggered, this, [this]() { d->aShowToolBar->setChecked(false); });

    // Text markup actions
    KToggleAction *aHighlighter = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-highlight")), i18nc("@action:intoolbar Annotation tool", "Highlighter"), this);
    KToggleAction *aUnderline = new KToggleAction(QIcon::fromTheme(QStringLiteral("format-text-underline")), i18nc("@action:intoolbar Annotation tool", "Underline"), this);
    KToggleAction *aSquiggle = new KToggleAction(QIcon::fromTheme(QStringLiteral("format-text-underline-squiggle")), i18nc("@action:intoolbar Annotation tool", "Squiggle"), this);
    KToggleAction *aStrikeout = new KToggleAction(QIcon::fromTheme(QStringLiteral("format-text-strikethrough")), i18nc("@action:intoolbar Annotation tool", "Strike Out"), this);
    // Notes actions
    KToggleAction *aTypewriter = new KToggleAction(QIcon::fromTheme(QStringLiteral("tool-text")), i18nc("@action:intoolbar Annotation tool", "Typewriter"), this);
    KToggleAction *aInlineNote = new KToggleAction(QIcon::fromTheme(QStringLiteral("note")), i18nc("@action:intoolbar Annotation tool", "Inline Note"), this);
    KToggleAction *aPopupNote = new KToggleAction(QIcon::fromTheme(QStringLiteral("edit-comment")), i18nc("@action:intoolbar Annotation tool", "Popup Note"), this);
    KToggleAction *aFreehandLine = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-freehand")), i18nc("@action:intoolbar Annotation tool", "Freehand Line"), this);
    // Geometrical shapes actions
    KToggleAction *aStraightLine = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-line")), i18nc("@action:intoolbar Annotation tool", "Straight line"), this);
    KToggleAction *aArrow = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-arrow")), i18nc("@action:intoolbar Annotation tool", "Arrow"), this);
    KToggleAction *aRectangle = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-rectangle")), i18nc("@action:intoolbar Annotation tool", "Rectangle"), this);
    KToggleAction *aEllipse = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-ellipse")), i18nc("@action:intoolbar Annotation tool", "Ellipse"), this);
    KToggleAction *aPolygon = new KToggleAction(QIcon::fromTheme(QStringLiteral("draw-polyline")), i18nc("@action:intoolbar Annotation tool", "Polygon"), this);
    d->aGeomShapes = new ToggleActionMenu(QIcon(), QString(), this, ToggleActionMenu::MenuButtonPopup, ToggleActionMenu::ImplicitDefaultAction);
    d->aGeomShapes->setText(i18nc("@action", "Geometrical shapes"));
    d->aGeomShapes->setEnabled(true); // Need to explicitly set this once, or refreshActions() in part.cpp will disable this action
    d->aGeomShapes->addAction(aArrow);
    d->aGeomShapes->addAction(aStraightLine);
    d->aGeomShapes->addAction(aRectangle);
    d->aGeomShapes->addAction(aEllipse);
    d->aGeomShapes->addAction(aPolygon);
    d->aGeomShapes->setDefaultAction(aArrow);

    // The order in which the actions are added is relevant to connect
    // them to the correct toolId defined in tools.xml
    d->agTools = new QActionGroup(this);
    d->agTools->addAction(aHighlighter);
    d->agTools->addAction(aUnderline);
    d->agTools->addAction(aSquiggle);
    d->agTools->addAction(aStrikeout);
    d->agTools->addAction(aTypewriter);
    d->agTools->addAction(aInlineNote);
    d->agTools->addAction(aPopupNote);
    d->agTools->addAction(aFreehandLine);
    d->agTools->addAction(aArrow);
    d->agTools->addAction(aStraightLine);
    d->agTools->addAction(aRectangle);
    d->agTools->addAction(aEllipse);
    d->agTools->addAction(aPolygon);

    d->textQuickTools = new QList<QAction *>();
    d->textTools = new QList<QAction *>();
    d->textTools->append(aHighlighter);
    d->textTools->append(aUnderline);
    d->textTools->append(aSquiggle);
    d->textTools->append(aStrikeout);

    int toolId = 1;
    const QList<QAction *> tools = d->agTools->actions();
    for (const auto &ann : tools) {
        // action group workaround: connecting to toggled instead of triggered
        connect(ann, &QAction::toggled, this, [this, toolId](bool checked) {
            if (checked)
                d->selectTool(toolId);
        });
        toolId++;
    }

    // Stamp action
    d->aStamp = new ToggleActionMenu(QIcon::fromTheme(QStringLiteral("tag")), QString(), this, ToggleActionMenu::MenuButtonPopup, ToggleActionMenu::ImplicitDefaultAction);
    d->aStamp->setText(i18nc("@action", "Stamp"));

    for (const auto &stamp : StampAnnotationWidget::defaultStamps) {
        KToggleAction *ann = new KToggleAction(d->stampIcon(stamp.second), stamp.first, this);
        if (!d->aStamp->defaultAction())
            d->aStamp->setDefaultAction(ann);
        d->aStamp->addAction(ann);
        d->agTools->addAction(ann);
        // action group workaround: connecting to toggled instead of triggered
        // (because deselectAllAnnotationActions has to call triggered)
        connect(ann, &QAction::toggled, this, [this, stamp](bool checked) {
            if (checked)
                d->slotStampToolSelected(stamp.second);
        });
    }

    // Quick annotations action
    d->aQuickTools = new KSelectAction(QIcon::fromTheme(QStringLiteral("draw-freehand")), i18nc("@action:intoolbar Show list of quick annotation tools", "Quick Annotations"), this);
    d->aQuickTools->setToolTip(i18nc("@info:tooltip", "Choose an annotation tool from the quick annotations"));
    d->aQuickTools->setToolBarMode(KSelectAction::MenuMode);
    d->aQuickTools->setEnabled(true); // required to ensure that populateQuickAnnotations is executed the first time
    d->populateQuickAnnotations();

    // Add to quick annotation action
    d->aAddToQuickTools = new QAction(QIcon::fromTheme(QStringLiteral("favorite")), i18nc("@action:intoolbar Add current annotation tool to the quick annotations list", "Add to Quick Annotations"), this);

    // Pin action
    d->aContinuousMode = new KToggleAction(QIcon::fromTheme(QStringLiteral("pin")), i18nc("@action:intoolbar When checked keep the current annotation tool active after use", "Keep Active"), this);
    d->aContinuousMode->setToolTip(i18nc("@info:tooltip", "Keep the annotation tool active after use"));
    d->aContinuousMode->setChecked(d->annotator->continuousMode());

    // Constrain angle action
    d->aConstrainRatioAndAngle =
        new KToggleAction(QIcon::fromTheme(QStringLiteral("snap-angle")), i18nc("@action When checked, line annotations are constrained to 15° steps, shape annotations to 1:1 ratio", "Constrain Ratio and Angle of Annotation Tools"), this);
    d->aConstrainRatioAndAngle->setChecked(d->annotator->constrainRatioAndAngleActive());

    // Annotation settings actions
    d->aColor = d->colorPickerAction(AnnotationActionHandlerPrivate::AnnotationColor::Color);
    d->aInnerColor = d->colorPickerAction(AnnotationActionHandlerPrivate::AnnotationColor::InnerColor);
    d->aFont = new QAction(QIcon::fromTheme(QStringLiteral("font-face")), i18nc("@action:intoolbar Current annotation config option", "Font"), this);
    d->aAdvancedSettings = new QAction(QIcon::fromTheme(QStringLiteral("settings-configure")), i18nc("@action:intoolbar Current annotation advanced settings", "Annotation Settings"), this);

    // Width list
    d->aWidth = new KSelectAction(QIcon::fromTheme(QStringLiteral("edit-line-width")), i18nc("@action:intoolbar Current annotation config option", "Line width"), this);
    d->aWidth->setToolBarMode(KSelectAction::MenuMode);
    for (auto width : d->widthStandardValues) {
        KToggleAction *ann = new KToggleAction(d->widthIcon(width), i18nc("@item:inlistbox", "Width %1", width), this);
        d->aWidth->addAction(ann);
        connect(ann, &QAction::triggered, this, [this, width]() { d->annotator->setAnnotationWidth(width); });
    }

    // Opacity list
    d->aOpacity = new KSelectAction(QIcon::fromTheme(QStringLiteral("edit-opacity")), i18nc("@action:intoolbar Current annotation config option", "Opacity"), this);
    d->aOpacity->setToolBarMode(KSelectAction::MenuMode);
    for (double opacity : d->opacityStandardValues) {
        KToggleAction *ann = new KToggleAction(GuiUtils::createOpacityIcon(opacity), QStringLiteral("%1\%").arg(opacity * 100), this);
        d->aOpacity->addAction(ann);
        connect(ann, &QAction::triggered, this, [this, opacity]() { d->annotator->setAnnotationOpacity(opacity); });
    }

    connect(d->aAddToQuickTools, &QAction::triggered, d->annotator, &PageViewAnnotator::addToQuickAnnotations);
    connect(d->aContinuousMode, &QAction::toggled, d->annotator, &PageViewAnnotator::setContinuousMode);
    connect(d->aConstrainRatioAndAngle, &QAction::toggled, d->annotator, &PageViewAnnotator::setConstrainRatioAndAngle);
    connect(d->aAdvancedSettings, &QAction::triggered, d->annotator, &PageViewAnnotator::slotAdvancedSettings);
    connect(d->aFont, &QAction::triggered, std::bind(&AnnotationActionHandlerPrivate::slotSelectAnnotationFont, d));

    // action group workaround: allows unchecking the currently selected annotation action.
    // Other parts of code dependent to this workaround are marked with "action group workaround".
    connect(d->agTools, &QActionGroup::triggered, this, [this](QAction *action) {
        if (action == d->agLastAction) {
            d->agLastAction = nullptr;
            d->agTools->checkedAction()->setChecked(false);
            d->selectTool(-1);
        } else {
            d->agLastAction = action;
            // Show the annotation toolbar whenever actions are triggered (e.g using shortcuts)
            d->aShowToolBar->setChecked(true);
        }
    });

    ac->addAction(QStringLiteral("mouse_toggle_annotate"), d->aShowToolBar);
    ac->addAction(QStringLiteral("hide_annotation_toolbar"), d->aHideToolBar);
    ac->addAction(QStringLiteral("annotation_highlighter"), aHighlighter);
    ac->addAction(QStringLiteral("annotation_underline"), aUnderline);
    ac->addAction(QStringLiteral("annotation_squiggle"), aSquiggle);
    ac->addAction(QStringLiteral("annotation_strike_out"), aStrikeout);
    ac->addAction(QStringLiteral("annotation_typewriter"), aTypewriter);
    ac->addAction(QStringLiteral("annotation_inline_note"), aInlineNote);
    ac->addAction(QStringLiteral("annotation_popup_note"), aPopupNote);
    ac->addAction(QStringLiteral("annotation_freehand_line"), aFreehandLine);
    ac->addAction(QStringLiteral("annotation_arrow"), aArrow);
    ac->addAction(QStringLiteral("annotation_straight_line"), aStraightLine);
    ac->addAction(QStringLiteral("annotation_rectangle"), aRectangle);
    ac->addAction(QStringLiteral("annotation_ellipse"), aEllipse);
    ac->addAction(QStringLiteral("annotation_polygon"), aPolygon);
    ac->addAction(QStringLiteral("annotation_geometrical_shape"), d->aGeomShapes);
    ac->addAction(QStringLiteral("annotation_stamp"), d->aStamp);
    ac->addAction(QStringLiteral("annotation_favorites"), d->aQuickTools);
    ac->addAction(QStringLiteral("annotation_bookmark"), d->aAddToQuickTools);
    ac->addAction(QStringLiteral("annotation_settings_pin"), d->aContinuousMode);
    ac->addAction(QStringLiteral("annotation_constrain_ratio_angle"), d->aConstrainRatioAndAngle);
    ac->addAction(QStringLiteral("annotation_settings_width"), d->aWidth);
    ac->addAction(QStringLiteral("annotation_settings_color"), d->aColor);
    ac->addAction(QStringLiteral("annotation_settings_inner_color"), d->aInnerColor);
    ac->addAction(QStringLiteral("annotation_settings_opacity"), d->aOpacity);
    ac->addAction(QStringLiteral("annotation_settings_font"), d->aFont);
    ac->addAction(QStringLiteral("annotation_settings_advanced"), d->aAdvancedSettings);

    ac->setDefaultShortcut(d->aShowToolBar, Qt::Key_F6);
    ac->setDefaultShortcut(aHighlighter, Qt::Key_1);
    ac->setDefaultShortcut(aUnderline, Qt::Key_2);
    ac->setDefaultShortcut(aSquiggle, Qt::Key_3);
    ac->setDefaultShortcut(aStrikeout, Qt::Key_4);
    ac->setDefaultShortcut(aTypewriter, Qt::Key_5);
    ac->setDefaultShortcut(aInlineNote, Qt::Key_6);
    ac->setDefaultShortcut(aPopupNote, Qt::Key_7);
    ac->setDefaultShortcut(aFreehandLine, Qt::Key_8);
    ac->setDefaultShortcut(aArrow, Qt::Key_9);
    ac->setDefaultShortcut(aRectangle, Qt::Key_0);
    ac->setDefaultShortcut(d->aAddToQuickTools, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    d->updateConfigActions();
}

AnnotationActionHandler::~AnnotationActionHandler()
{
    // delete the private data storage structure
    delete d;
}

void AnnotationActionHandler::setupAnnotationToolBarVisibilityAction()
{
    // find the main window associated to the toggle toolbar action
    QList<QWidget *> widgets = d->aShowToolBar->associatedWidgets();
    auto itMainWindow = std::find_if(widgets.begin(), widgets.end(), [](const QWidget *widget) { return qobject_cast<const KParts::MainWindow *>(widget) != nullptr; });
    Q_ASSERT(itMainWindow != widgets.end());
    KParts::MainWindow *mw = qobject_cast<KParts::MainWindow *>(*itMainWindow);
    // ensure that the annotation toolbar has been created and retrieve it
    QList<KToolBar *> toolbars = mw->toolBars();
    auto itToolBar = std::find_if(toolbars.begin(), toolbars.end(), [](const KToolBar *toolBar) { return toolBar->objectName() == QStringLiteral("annotationToolBar"); });
    Q_ASSERT(itToolBar != toolbars.end());
    KToolBar *annotationToolBar = mw->toolBar(QStringLiteral("annotationToolBar"));
    d->aShowToolBar->setChecked(annotationToolBar->isVisible());
    connect(annotationToolBar, &QToolBar::visibilityChanged, d->aShowToolBar, &QAction::setChecked, Qt::UniqueConnection);
    connect(d->aShowToolBar, &QAction::toggled, annotationToolBar, &KToolBar::setVisible, Qt::UniqueConnection);
    connect(d->aShowToolBar, &QAction::toggled, this, [this](bool checked) { d->slotToolBarVisibilityChanged(checked); });
}

void AnnotationActionHandler::reparseTools()
{
    d->parseTool(d->selectedTool);
    d->populateQuickAnnotations();
}

void AnnotationActionHandler::setToolsEnabled(bool on)
{
    const QList<QAction *> tools = d->agTools->actions();
    for (QAction *ann : tools) {
        ann->setEnabled(on);
    }
    d->aQuickTools->setEnabled(on);
    d->aGeomShapes->setEnabled(on);
    d->aStamp->setEnabled(on);
    d->aContinuousMode->setEnabled(on);
}

void AnnotationActionHandler::setTextToolsEnabled(bool on)
{
    d->textToolsEnabled = on;
    for (QAction *ann : qAsConst(*d->textTools)) {
        ann->setEnabled(on);
    }
    for (QAction *ann : qAsConst(*d->textQuickTools)) {
        ann->setEnabled(on);
    }
}

void AnnotationActionHandler::deselectAllAnnotationActions()
{
    QAction *checkedAction = d->agTools->checkedAction();
    if (checkedAction) {
        checkedAction->trigger(); // action group workaround: using trigger instead of setChecked
    }
}

#include "moc_annotationactionhandler.cpp"
