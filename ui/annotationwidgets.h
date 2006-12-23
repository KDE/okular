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

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QWidget;
class KColorButton;
class KFontRequester;
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

    void addItem( const QString& item, const QString& id );

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
    static AnnotationWidget * widgetFor( Okular::Annotation * ann );
};

class AnnotationWidget
  : public QObject
{
    Q_OBJECT

public:
    virtual ~AnnotationWidget();

    virtual Okular::Annotation::SubType annotationType() const;

    virtual QWidget * widget() = 0;

    virtual void applyChanges() = 0;

signals:
    void dataChanged();

protected:
    AnnotationWidget( Okular::Annotation * ann );

    Okular::Annotation * m_ann;
};

class TextAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    TextAnnotationWidget( Okular::Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    Okular::TextAnnotation * m_textAnn;
    QWidget * m_widget;
    PixmapPreviewSelector * m_pixmapSelector;
    KFontRequester * m_fontReq;
};

class StampAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    StampAnnotationWidget( Okular::Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    Okular::StampAnnotation * m_stampAnn;
    QWidget * m_widget;
    PixmapPreviewSelector * m_pixmapSelector;
};

class LineAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    LineAnnotationWidget( Okular::Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    Okular::LineAnnotation * m_lineAnn;
    int m_lineType;
    QWidget * m_widget;
    QDoubleSpinBox * m_spinLL;
    QDoubleSpinBox * m_spinLLE;
};

class HighlightAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    HighlightAnnotationWidget( Okular::Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    Okular::HighlightAnnotation * m_hlAnn;
    QWidget * m_widget;
    QComboBox * m_typeCombo;
};

class GeomAnnotationWidget
  : public AnnotationWidget
{
    Q_OBJECT

public:
    GeomAnnotationWidget( Okular::Annotation * ann );

    virtual QWidget * widget();

    virtual void applyChanges();

private:
    Okular::GeomAnnotation * m_geomAnn;
    QWidget * m_widget;
    QComboBox * m_typeCombo;
    QCheckBox * m_useColor;
    KColorButton * m_innerColor;
};

#endif
