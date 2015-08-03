/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _WIDGETANNOTTOOLS_H_
#define _WIDGETANNOTTOOLS_H_

#include <QDialog>
#include <qdom.h>
#include <qwidget.h>

class KLineEdit;
class KComboBox;
class QPushButton;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QGroupBox;
class AnnotationWidget;

namespace Okular
{
class Annotation;
}

class WidgetAnnotTools : public QWidget
{
    Q_OBJECT

    Q_PROPERTY( QStringList tools READ tools WRITE setTools NOTIFY changed USER true )

    public:
        WidgetAnnotTools( QWidget * parent = 0 );
        ~WidgetAnnotTools();

        QStringList tools() const;
        void setTools(const QStringList& items);

    Q_SIGNALS:
        void changed();

    private:
        QListWidget *m_list;
        QPushButton *m_btnAdd;
        QPushButton *m_btnEdit;
        QPushButton *m_btnRemove;
        QPushButton *m_btnMoveUp;
        QPushButton *m_btnMoveDown;

    private slots:
        void updateButtons();
        void slotAdd();
        void slotEdit();
        void slotRemove();
        void slotMoveUp();
        void slotMoveDown();
};

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

        EditAnnotToolDialog( QWidget *parent = 0, const QDomElement &initialState = QDomElement() );
        ~EditAnnotToolDialog();
        QString name() const;
        QDomDocument toolXml() const;

    private:
        void createStubAnnotation();
        void rebuildAppearanceBox();
        void updateDefaultNameAndIcon();
        void setToolType( ToolType newType );
        void loadTool( const QDomElement &toolElement );

        KLineEdit *m_name;
        KComboBox *m_type;
        QLabel *m_toolIcon;
        QGroupBox *m_appearanceBox;

        Okular::Annotation *m_stubann;
        AnnotationWidget *m_annotationWidget;

    private slots:
        void slotTypeChanged();
        void slotDataChanged();
};

Q_DECLARE_METATYPE( EditAnnotToolDialog::ToolType );

#endif
