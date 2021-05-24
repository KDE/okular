/*
    SPDX-FileCopyrightText: 2019 Simone Gaiarin <simgunz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _ANNOTATIONACTIONHANDLER_H_
#define _ANNOTATIONACTIONHANDLER_H_

#include <QObject>

class QAction;
class QColor;
class QFont;
class KActionCollection;
class PageViewAnnotator;
class AnnotationActionHandlerPrivate;

/**
 * @short Handles all the actions of the annotation toolbar
 */
class AnnotationActionHandler : public QObject
{
    Q_OBJECT

public:
    AnnotationActionHandler(PageViewAnnotator *parent, KActionCollection *ac);
    ~AnnotationActionHandler() override;

    /**
     * @short Reads the settings for the current annotation and rebuild the quick annotations menu
     *
     * This method is called each time okularpartrc is modified. This happens in the following
     * situations (among others): the quick annotations are modified from the KCM settings
     * page, a tool is modified using the "advanced settings" action, a quick annotation is
     * selected, an annotation property (line width, colors, opacity, font) is modified.
     */
    void setupAnnotationToolBarVisibilityAction();
    void reparseBuiltinToolsConfig();
    void reparseQuickToolsConfig();
    void setToolsEnabled(bool on);
    void setTextToolsEnabled(bool on);
    void deselectAllAnnotationActions();

private:
    class AnnotationActionHandlerPrivate *d;
};

#endif // _ANNOTATIONACTIONHANDLER_H_
