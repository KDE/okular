/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef EDITANNOTTOOLDIALOG_H
#define EDITANNOTTOOLDIALOG_H

#include <QDialog>
#include <QDomElement>
class KLineEdit;
class KComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
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
        enum ToolType
        {
            ToolNoteLinked,
            ToolNoteInline,
            ToolInk,
            ToolStraightLine,
            ToolPolygon,
            ToolTextMarkup,
            ToolGeometricalShape,
            ToolStamp
        };

        EditAnnotToolDialog( QWidget *parent = nullptr, const QDomElement &initialState = QDomElement() );
        ~EditAnnotToolDialog();
        QString name() const;
        QString key() const;        
        QDomDocument toolXml() const;

    private:
        void createStubAnnotation();
        void rebuildAppearanceBox();
        void updateDefaultNameAndIcon();
        void setToolType( ToolType newType );
        void loadTool( const QDomElement &toolElement );

        KLineEdit *m_name;
        KLineEdit *m_key;        
        KComboBox *m_type;
        QLabel *m_toolIcon;
        QGroupBox *m_appearanceBox;

        Okular::Annotation *m_stubann;
        AnnotationWidget *m_annotationWidget;

    private Q_SLOTS:
        void slotTypeChanged();
        void slotDataChanged();
};

Q_DECLARE_METATYPE( EditAnnotToolDialog::ToolType )

#endif // EDITANNOTTOOLDIALOG_H
