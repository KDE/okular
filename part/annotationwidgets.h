/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _ANNOTATIONWIDGETS_H_
#define _ANNOTATIONWIDGETS_H_

#include <qwidget.h>

#include "core/annotations.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QPushButton;
class QWidget;
class KColorButton;
class QSpinBox;
class KFontRequester;
class AnnotationWidget;

class PixmapPreviewSelector : public QWidget
{
    Q_OBJECT

public:
    enum PreviewPosition { Side, Below };

    explicit PixmapPreviewSelector(QWidget *parent = nullptr, PreviewPosition position = Side);
    ~PixmapPreviewSelector() override;

    void setIcon(const QString &icon);
    QString icon() const;

    void addItem(const QString &item, const QString &id);

    void setPreviewSize(int size);
    int previewSize() const;

    void setEditable(bool editable);

Q_SIGNALS:
    void iconChanged(const QString &);

private Q_SLOTS:
    void iconComboChanged(const QString &icon);
    void selectCustomStamp();

private:
    QString m_icon;
    QPushButton *m_stampPushButton;
    QLabel *m_iconLabel;
    QComboBox *m_comboItems;
    int m_previewSize;
    PreviewPosition m_previewPosition;
};

/**
 * A factory to create AnnotationWidget's.
 */
class AnnotationWidgetFactory
{
public:
    static AnnotationWidget *widgetFor(Okular::Annotation *ann);
};

class AnnotationWidget : public QObject
{
    Q_OBJECT

public:
    explicit AnnotationWidget(Okular::Annotation *ann);
    ~AnnotationWidget() override;

    virtual Okular::Annotation::SubType annotationType() const;

    QWidget *appearanceWidget();
    QWidget *extraWidget();

    virtual void applyChanges();

    void setAnnotTypeEditable(bool);

Q_SIGNALS:
    void dataChanged();

protected:
    QWidget *createAppearanceWidget();

    virtual void createStyleWidget(QFormLayout *formLayout);
    virtual QWidget *createExtraWidget();

    void addColorButton(QWidget *widget, QFormLayout *formlayout);
    void addOpacitySpinBox(QWidget *widget, QFormLayout *formlayout);
    void addVerticalSpacer(QFormLayout *formlayout);

    bool m_typeEditable;

private:
    Okular::Annotation *m_ann;
    QWidget *m_appearanceWidget {nullptr};
    QWidget *m_extraWidget {nullptr};
    KColorButton *m_colorBn {nullptr};
    QSpinBox *m_opacity {nullptr};
};

class QVBoxLayout;
class QFormLayout;

class TextAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit TextAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    void createPopupNoteStyleUi(QWidget *widget, QFormLayout *formlayout);
    void createInlineNoteStyleUi(QWidget *widget, QFormLayout *formlayout);
    void createTypewriterStyleUi(QWidget *widget, QFormLayout *formlayout);
    void addPixmapSelector(QWidget *widget, QFormLayout *formlayout);
    void addFontRequester(QWidget *widget, QFormLayout *formlayout);
    void addTextColorButton(QWidget *widget, QFormLayout *formlayout);
    void addTextAlignComboBox(QWidget *widget, QFormLayout *formlayout);
    void addWidthSpinBox(QWidget *widget, QFormLayout *formlayout);

    inline bool isTypewriter() const
    {
        return (m_textAnn->inplaceIntent() == Okular::TextAnnotation::TypeWriter);
    }

    Okular::TextAnnotation *m_textAnn;
    PixmapPreviewSelector *m_pixmapSelector {nullptr};
    KFontRequester *m_fontReq {nullptr};
    KColorButton *m_textColorBn {nullptr};
    QComboBox *m_textAlign {nullptr};
    QDoubleSpinBox *m_spinWidth {nullptr};
};

class StampAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    static const QList<QPair<QString, QString>> &defaultStamps();

    explicit StampAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    Okular::StampAnnotation *m_stampAnn;
    PixmapPreviewSelector *m_pixmapSelector;
};

class LineAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit LineAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    static QIcon endStyleIcon(Okular::LineAnnotation::TermStyle endStyle, const QColor &lineColor);

    Okular::LineAnnotation *m_lineAnn;
    int m_lineType;
    QDoubleSpinBox *m_spinLL {nullptr};
    QDoubleSpinBox *m_spinLLE {nullptr};
    QCheckBox *m_useColor {nullptr};
    KColorButton *m_innerColor {nullptr};
    QDoubleSpinBox *m_spinSize {nullptr};
    QComboBox *m_startStyleCombo {nullptr};
    QComboBox *m_endStyleCombo {nullptr};
};

class HighlightAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit HighlightAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    Okular::HighlightAnnotation *m_hlAnn;
    QComboBox *m_typeCombo;
};

class GeomAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit GeomAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    Okular::GeomAnnotation *m_geomAnn;
    QComboBox *m_typeCombo;
    QCheckBox *m_useColor;
    KColorButton *m_innerColor;
    QDoubleSpinBox *m_spinSize;
};

class FileAttachmentAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit FileAttachmentAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;
    QWidget *createExtraWidget() override;

private:
    Okular::FileAttachmentAnnotation *m_attachAnn;
    PixmapPreviewSelector *m_pixmapSelector;
};

class CaretAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit CaretAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    Okular::CaretAnnotation *m_caretAnn;
    PixmapPreviewSelector *m_pixmapSelector;
};

class InkAnnotationWidget : public AnnotationWidget
{
    Q_OBJECT

public:
    explicit InkAnnotationWidget(Okular::Annotation *ann);

    void applyChanges() override;

protected:
    void createStyleWidget(QFormLayout *formlayout) override;

private:
    Okular::InkAnnotation *m_inkAnn;
    QDoubleSpinBox *m_spinSize;
};

#endif
