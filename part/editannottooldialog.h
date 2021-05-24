/*
    SPDX-FileCopyrightText: 2012 Fabio D 'Urso <fabiodurso@hotmail.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef EDITANNOTTOOLDIALOG_H
#define EDITANNOTTOOLDIALOG_H

#include <QDialog>
#include <QDomElement>
class KLineEdit;
class KComboBox;
class QLabel;
class QListWidget;
class QGroupBox;
class AnnotationWidget;

namespace Okular
{
class Annotation;
}

class EditAnnotToolDialog : public QDialog
{
    Q_OBJECT

public:
    enum ToolType { ToolNoteLinked, ToolNoteInline, ToolInk, ToolStraightLine, ToolPolygon, ToolTextMarkup, ToolGeometricalShape, ToolStamp, ToolTypewriter };

    explicit EditAnnotToolDialog(QWidget *parent = nullptr, const QDomElement &initialState = QDomElement(), bool builtinTool = false);
    ~EditAnnotToolDialog() override;
    QString name() const;
    QDomDocument toolXml() const;

private:
    void createStubAnnotation();
    void rebuildAppearanceBox();
    void updateDefaultNameAndIcon();
    void setToolType(ToolType newType);
    void loadTool(const QDomElement &toolElement);

    KLineEdit *m_name;
    KComboBox *m_type;
    QLabel *m_toolIcon;
    QGroupBox *m_appearanceBox;

    Okular::Annotation *m_stubann;
    AnnotationWidget *m_annotationWidget;

    bool m_builtinTool;

private Q_SLOTS:
    void slotTypeChanged();
    void slotDataChanged();
};

Q_DECLARE_METATYPE(EditAnnotToolDialog::ToolType)

#endif // EDITANNOTTOOLDIALOG_H
