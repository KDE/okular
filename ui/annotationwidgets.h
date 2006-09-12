/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ANNOTATIONWIDGETS_H_
#define _ANNOTATIONWIDGETS_H_

#include "core/annotations.h"

class QLabel;
class QWidget;
class AnnotationWidget;

/**
 * A factory to create AnnotationWidget's.
 */
class AnnotationWidgetFactory
{
public:
    static AnnotationWidget * widgetFor( Annotation * ann );
};

class AnnotationWidget
  : public QObject
{
    Q_OBJECT

public:
    virtual ~AnnotationWidget();

    virtual Annotation::SubType annotationType();

    virtual QWidget * widget() = 0;

    virtual void applyChanges() = 0;

protected:
    AnnotationWidget( Annotation * ann );

    Annotation * m_ann;
};

class StampAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    StampAnnotationWidget( Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private slots:
    void iconChanged( const QString& );

private:
    StampAnnotation * m_stampAnn;
    QWidget * m_widget;
    QLabel * m_iconLabel;
    QString m_currentIcon;
};

#endif
