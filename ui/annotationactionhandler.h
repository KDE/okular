/**************************************************************************
 *   Copyright (C) 2019 by Simone Gaiarin <simgunz@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 **************************************************************************/

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
    void reparseTools();
    void setToolsEnabled(bool on);
    void setTextToolsEnabled(bool on);
    void deselectAllAnnotationActions();

private:
    class AnnotationActionHandlerPrivate *d;
};

#endif // _ANNOTATIONACTIONHANDLER_H_
