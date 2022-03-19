/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_PAGEVIEWANNOTATOR_H_
#define _OKULAR_PAGEVIEWANNOTATOR_H_

#include <QObject>
#include <qdom.h>

#include <KActionCollection>

#include "annotationtools.h"
#include "pageviewutils.h"

class QKeyEvent;
class QMouseEvent;
class QPainter;
class AnnotationActionHandler;

namespace Okular
{
class Document;
}

// engines are defined and implemented in the cpp
class AnnotatorEngine;
class AnnotationTools;
class PageView;

/**
 * @short PageView object devoted to annotation creation/handling.
 *
 * PageViewAnnotator is the okular class used for visually creating annotations.
 * It uses internal 'engines' for interacting with user events and attaches
 * the newly created annotation to the document when the creation is complete.
 * In the meanwhile all PageView events (actually mouse/paint ones) are routed
 * to this class that performs a rough visual representation of what the
 * annotation will become when finished.
 *
 * m_builtinToolsDefinition is a AnnotationTools object that wraps a DOM object that
 * contains Annotations/Engine association for the items placed in the toolbar.
 * The XML is parsed after selecting a toolbar item, in which case an Ann is
 * initialized with the values in the XML and an engine is created to handle
 * that annotation. m_builtinToolsDefinition is created in reparseConfig according to
 * user configuration. m_builtinToolsDefinition is updated (and saved to disk) (1) each
 * time a property of an annotation (color, font, etc) is changed by the user,
 * and (2) each time a "quick annotation" is selected, in which case the properties
 * of the selected quick annotation are written over those of the corresponding
 * builtin tool
 */
class PageViewAnnotator : public QObject
{
    Q_OBJECT
public:
    static const int STAMP_TOOL_ID;

    PageViewAnnotator(PageView *parent, Okular::Document *storage);
    ~PageViewAnnotator() override;

    // methods used when creating the annotation
    // @return Is a tool currently selected?
    bool active() const;
    // @return Are we currently annotating (using the selected tool)?
    bool annotating() const;

    void setSignatureMode(bool enabled);
    bool signatureMode() const;

    // returns the preferred cursor for the current tool. call this only
    // if active() == true
    QCursor cursor() const;

    QRect routeMouseEvent(QMouseEvent *event, PageViewItem *item);
    QRect routeTabletEvent(QTabletEvent *event, PageViewItem *item, const QPoint localOriginInGlobal);
    QRect performRouteMouseOrTabletEvent(const AnnotatorEngine::EventType eventType, const AnnotatorEngine::Button button, const AnnotatorEngine::Modifiers modifiers, const QPointF pos, PageViewItem *item);
    bool routeKeyEvent(QKeyEvent *event);
    bool routePaints(const QRect wantedRect) const;
    void routePaint(QPainter *painter, const QRect paintRect);

    void reparseConfig();

    static QString defaultToolName(const QDomElement &toolElement);
    static QPixmap makeToolPixmap(const QDomElement &toolElement);

    // methods related to the annotation actions
    void setupActions(KActionCollection *ac);
    // setup those actions that first require the GUI is fully created
    void setupActionsPostGUIActivated();
    // @return Is continuous mode active (pin annotation)?
    bool continuousMode();
    /**
     * State of constrain ratio and angle action.
     * While annotating, this value is XOR-ed with the Shift modifier.
     */
    bool constrainRatioAndAngleActive();
    // enable/disable the annotation actions
    void setToolsEnabled(bool enabled);
    // enable/disable the text-selection annotation actions
    void setTextToolsEnabled(bool enabled);

    enum class ShowTip { Yes, No };
    // selects the active tool
    void selectBuiltinTool(int toolId, ShowTip showTip);
    // selects a stamp tool and sets the stamp symbol
    void selectStampTool(const QString &stampSymbol);
    // selects the active quick tool
    void selectQuickTool(int toolId);
    // selects the last used tool
    void selectLastTool();
    // deselects the tool and uncheck all the annotation actions
    void detachAnnotation();

    // returns the builtin annotation tool with the given Id
    QDomElement builtinTool(int toolId);
    // returns the quick annotation tool with the given Id
    QDomElement quickTool(int toolId);

    // methods that write the properties
    void setAnnotationWidth(double width);
    void setAnnotationColor(const QColor &color);
    void setAnnotationInnerColor(const QColor &color);
    void setAnnotationOpacity(double opacity);
    void setAnnotationFont(const QFont &font);

public Q_SLOTS:
    void setContinuousMode(bool enabled);
    /**
     * State of constrain ratio and angle action.
     * While annotating, this value is XOR-ed with the Shift modifier.
     */
    void setConstrainRatioAndAngle(bool enabled);
    void addToQuickAnnotations();
    void slotAdvancedSettings();

Q_SIGNALS:
    /**
     * This signal is emitted whenever an annotation tool is activated or all the tools get deactivated
     */
    void toolActive(bool active);
    void requestOpenFile(const QString &filePath, int pageNumber);

private:
    void reparseBuiltinToolsConfig();
    void reparseQuickToolsConfig();
    // save the builtin annotation tools to Okular settings
    void saveBuiltinAnnotationTools();
    // selects the active tool
    void selectTool(AnnotationTools *toolsDefinition, int toolId, ShowTip showTip);
    // returns the engine QDomElement of the the currently active tool
    QDomElement currentEngineElement();
    // returns the annotation QDomElement of the the currently active tool
    QDomElement currentAnnotationElement();

    // global class pointers
    Okular::Document *m_document;
    PageView *m_pageView;
    AnnotationActionHandler *m_actionHandler;
    AnnotatorEngine *m_engine;
    AnnotationTools *m_builtinToolsDefinition;
    AnnotationTools *m_quickToolsDefinition;
    bool m_continuousMode;
    bool m_constrainRatioAndAngle;
    bool m_signatureMode;

    // creation related variables
    AnnotationTools *m_lastToolsDefinition;
    int m_lastToolId;
    QRect m_lastDrawnRect;
    PageViewItem *m_lockedItem;
    // selected annotation name
    // QString m_selectedAnnotationName;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
