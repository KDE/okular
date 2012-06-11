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

#include <kdialog.h>
#include <qwidget.h>

class KLineEdit;
class KComboBox;
class KColorButton;
class KIntNumInput;
class KPushButton;
class QListWidget;

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
        void updateButtons();

        QListWidget *m_list;
        KPushButton *m_btnAdd;
        KPushButton *m_btnRemove;
        KPushButton *m_btnMoveUp;
        KPushButton *m_btnMoveDown;

    private slots:
        void slotRowChanged( int );
        void slotAdd( bool );
        void slotRemove( bool );
        void slotMoveUp( bool );
        void slotMoveDown( bool );
};

class NewAnnotToolDialog : public KDialog
{
    Q_OBJECT

    public:
        NewAnnotToolDialog( QWidget *parent = 0 );
        QString name() const;
        QString toolXml() const;

    private:
        KLineEdit * m_name;
        KComboBox * m_type;
        KColorButton * m_color;
        KIntNumInput * m_opacity;

    private slots:
        void slotNameEdited( const QString &new_name );
};

#endif
