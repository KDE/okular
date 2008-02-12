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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <ktextedit.h>
#include <kurlrequester.h>

class QButtonGroup;
class FormWidgetIface;
class PageViewItem;
class RadioButtonEdit;

namespace Okular {
class FormField;
class FormFieldButton;
class FormFieldChoice;
class FormFieldText;
}

struct RadioData
{
    RadioData() {}

    QList< int > ids;
    QButtonGroup *group;
};

class FormWidgetsController : public QObject
{
    Q_OBJECT

    public:
        FormWidgetsController( QObject *parent = 0 );
        virtual ~FormWidgetsController();

        void signalChanged( FormWidgetIface *w );

        QButtonGroup* registerRadioButton( FormWidgetIface* widget, const QList< int >& siblings );
        void dropRadioButtons();

    signals:
        void changed( FormWidgetIface *w );

    private:
        QList< RadioData > m_radios;
};


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
        void setCanBeFilled( bool fill );

        void setPageItem( PageViewItem *pageItem );
        Okular::FormField* formField() const;
        PageViewItem* pageItem() const;

        virtual void setFormWidgetsController( FormWidgetsController *controller );
        virtual QAbstractButton* button();

    protected:
        FormWidgetsController * m_controller;

    private:
        QWidget * m_widget;
        Okular::FormField * m_ff;
        PageViewItem * m_pageItem;
};


class PushButtonEdit : public QPushButton, public FormWidgetIface
{
    Q_OBJECT

    public:
        PushButtonEdit( Okular::FormFieldButton * button, QWidget * parent = 0 );

    private:
        Okular::FormFieldButton * m_form;
};

class CheckBoxEdit : public QCheckBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        CheckBoxEdit( Okular::FormFieldButton * button, QWidget * parent = 0 );

        // reimplemented from FormWidgetIface
        void setFormWidgetsController( FormWidgetsController *controller );
        QAbstractButton* button();

    private slots:
        void slotStateChanged( int state );

    private:
        Okular::FormFieldButton * m_form;
};

class RadioButtonEdit : public QRadioButton, public FormWidgetIface
{
    Q_OBJECT

    public:
        RadioButtonEdit( Okular::FormFieldButton * button, QWidget * parent = 0 );

        // reimplemented from FormWidgetIface
        void setFormWidgetsController( FormWidgetsController *controller );
        QAbstractButton* button();

    private slots:
        void slotToggled( bool checked );

    private:
        Okular::FormFieldButton * m_form;
};

class FormLineEdit : public QLineEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit FormLineEdit( Okular::FormFieldText * text, QWidget * parent = 0 );

    private slots:
        void textEdited( const QString& );

    private:
        Okular::FormFieldText * m_form;
};

class TextAreaEdit : public KTextEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit TextAreaEdit( Okular::FormFieldText * text, QWidget * parent = 0 );

    private slots:
        void slotChanged();

    private:
        Okular::FormFieldText * m_form;
};


class FileEdit : public KUrlRequester, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit FileEdit( Okular::FormFieldText * text, QWidget * parent = 0 );

    private slots:
        void slotChanged( const QString& );

    private:
        Okular::FormFieldText * m_form;
};


class ListEdit : public QListWidget, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit ListEdit( Okular::FormFieldChoice * choice, QWidget * parent = 0 );

    private slots:
        void slotSelectionChanged();

    private:
        Okular::FormFieldChoice * m_form;
};


class ComboEdit : public QComboBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit ComboEdit( Okular::FormFieldChoice * choice, QWidget * parent = 0 );

    private slots:
        void indexChanged( int );

    private:
        Okular::FormFieldChoice * m_form;
};

#endif
