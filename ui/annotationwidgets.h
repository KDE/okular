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

#include <qwidget.h>

#include "core/annotations.h"

class QComboBox;
class QLabel;
class QWidget;
class AnnotationWidget;

class PixmapPreviewSelector
  : public QWidget
{
    Q_OBJECT

public:
    explicit PixmapPreviewSelector( QWidget * parent = 0 );
    virtual ~PixmapPreviewSelector();

    void setIcon( const QString& icon );
    QString icon() const;

    void setItems( const QStringList& items );

    void setPreviewSize( int size );
    int previewSize() const;

signals:
    void iconChanged( const QString& );

private slots:
    void iconComboChanged( const QString& icon );

private:
    QString m_icon;
    QLabel * m_iconLabel;
    QComboBox * m_comboItems;
    int m_previewSize;
};


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

signals:
    void dataChanged();

protected:
    AnnotationWidget( Annotation * ann );

    Annotation * m_ann;
};

class TextAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    TextAnnotationWidget( Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    TextAnnotation * m_textAnn;
    QWidget * m_widget;
    PixmapPreviewSelector * m_pixmapSelector;
};

class StampAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    StampAnnotationWidget( Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    StampAnnotation * m_stampAnn;
    QWidget * m_widget;
    PixmapPreviewSelector * m_pixmapSelector;
};

#endif
