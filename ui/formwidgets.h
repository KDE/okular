/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_FORMWIDGETS_H_
#define _OKULAR_FORMWIDGETS_H_

#include "core/area.h"

#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include <ktextedit.h>
#include <kurlrequester.h>

class FormWidgetIface;

namespace Okular {
class FormField;
class FormFieldButton;
class FormFieldChoice;
class FormFieldText;
}

class FormWidgetFactory
{
    public:
        static FormWidgetIface * createWidget( Okular::FormField * ff, QWidget * parent = 0 );
};


class FormWidgetIface
{
    public:
        FormWidgetIface( QWidget * w, Okular::FormField * ff );
        virtual ~FormWidgetIface();

        Okular::NormalizedRect rect() const;
        void setWidthHeight( int w, int h );
        void moveTo( int x, int y );
        bool setVisibility( bool visible );

    private:
        QWidget * m_widget;
        Okular::FormField * m_ff;
};


class FormLineEdit : public QLineEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        FormLineEdit( Okular::FormFieldText * text, QWidget * parent = 0 );

    private slots:
        void textEdited( const QString& );

    private:
        Okular::FormFieldText * m_form;
};

class TextAreaEdit : public KTextEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        TextAreaEdit( Okular::FormFieldText * text, QWidget * parent = 0 );

    private slots:
        void slotChanged();

    private:
        Okular::FormFieldText * m_form;
};


class FileEdit : public KUrlRequester, public FormWidgetIface
{
    Q_OBJECT

    public:
        FileEdit( Okular::FormFieldText * text, QWidget * parent = 0 );

    private slots:
        void slotChanged( const QString& );

    private:
        Okular::FormFieldText * m_form;
};


class ListEdit : public QListWidget, public FormWidgetIface
{
    Q_OBJECT

    public:
        ListEdit( Okular::FormFieldChoice * choice, QWidget * parent = 0 );

    private slots:
        void selectionChanged();

    private:
        Okular::FormFieldChoice * m_form;
};


class ComboEdit : public QComboBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        ComboEdit( Okular::FormFieldChoice * choice, QWidget * parent = 0 );

    private slots:
        void indexChanged( int );

    private:
        Okular::FormFieldChoice * m_form;
};

#endif
