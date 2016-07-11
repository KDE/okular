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

class ComboEdit;
class QMenu;
class QButtonGroup;
class FormWidgetIface;
class PageViewItem;
class RadioButtonEdit;
class QEvent;

namespace Okular {
class Action;
class FormField;
class FormFieldButton;
class FormFieldChoice;
class FormFieldText;
class Document;
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
        FormWidgetsController( Okular::Document *doc );
        virtual ~FormWidgetsController();

        void signalAction( Okular::Action *action );

        QButtonGroup* registerRadioButton( QAbstractButton *button, Okular::FormFieldButton *formButton );
        void dropRadioButtons();
        bool canUndo();
        bool canRedo();

    Q_SIGNALS:
        void changed( int pageNumber );
        void requestUndo();
        void requestRedo();
        void canUndoChanged( bool undoAvailable );
        void canRedoChanged( bool redoAvailable);
        void formTextChangedByWidget( int pageNumber,
                                      Okular::FormFieldText *form,
                                      const QString & newContents,
                                      int newCursorPos,
                                      int prevCursorPos,
                                      int prevAnchorPos );

        void formTextChangedByUndoRedo( int pageNumber,
                                        Okular::FormFieldText *form,
                                        const QString & contents,
                                        int cursorPos,
                                        int anchorPos );

        void formListChangedByWidget( int pageNumber,
                                      Okular::FormFieldChoice *form,
                                      const QList< int > & newChoices );

        void formListChangedByUndoRedo( int pageNumber,
                                        Okular::FormFieldChoice *form,
                                        const QList< int > & choices );

        void formComboChangedByWidget( int pageNumber,
                                       Okular::FormFieldChoice *form,
                                       const QString & newText,
                                       int newCursorPos,
                                       int prevCursorPos,
                                       int prevAnchorPos
                                     );

        void formComboChangedByUndoRedo( int pageNumber,
                                         Okular::FormFieldChoice *form,
                                         const QString & text,
                                         int cursorPos,
                                         int anchorPos
                                       );

        void formButtonsChangedByWidget( int pageNumber,
                                         const QList< Okular::FormFieldButton* > & formButtons,
                                         const QList< bool > & newButtonStates );


        void action( Okular::Action *action );

    private Q_SLOTS:
        void slotButtonClicked( QAbstractButton *button );
        void slotFormButtonsChangedByUndoRedo( int pageNumber,
                                               const QList< Okular::FormFieldButton* > & formButtons );

    private:
        friend class TextAreaEdit;
        friend class FormLineEdit;
        friend class FileEdit;
        friend class ListEdit;
        friend class ComboEdit;

        QList< RadioData > m_radios;
        QHash< int, Okular::FormFieldButton* > m_formButtons;
        QHash< int, QAbstractButton* > m_buttons;
        Okular::Document* m_doc;
};


class FormWidgetFactory
{
    public:
        static FormWidgetIface * createWidget( Okular::FormField * ff, QWidget * parent = Q_NULLPTR );
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
        explicit PushButtonEdit( Okular::FormFieldButton * button, QWidget * parent = Q_NULLPTR );

    private Q_SLOTS:
        void slotClicked();

    private:
        Okular::FormFieldButton * m_form;
};

class CheckBoxEdit : public QCheckBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit CheckBoxEdit( Okular::FormFieldButton * button, QWidget * parent = Q_NULLPTR );

        // reimplemented from FormWidgetIface
        void setFormWidgetsController( FormWidgetsController *controller );
        QAbstractButton* button();

    private Q_SLOTS:
        void slotStateChanged( int state );

    private:
        Okular::FormFieldButton * m_form;
};

class RadioButtonEdit : public QRadioButton, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit RadioButtonEdit( Okular::FormFieldButton * button, QWidget * parent = Q_NULLPTR );

        // reimplemented from FormWidgetIface
        void setFormWidgetsController( FormWidgetsController *controller );
        QAbstractButton* button();

    private:
        Okular::FormFieldButton * m_form;
};

class FormLineEdit : public QLineEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit FormLineEdit( Okular::FormFieldText * text, QWidget * parent = Q_NULLPTR );
        void setFormWidgetsController( FormWidgetsController *controller );
        virtual bool event ( QEvent * e );
        virtual void contextMenuEvent( QContextMenuEvent* event );


    public Q_SLOTS:
        void slotHandleTextChangedByUndoRedo( int pageNumber,
                                              Okular::FormFieldText* textForm,
                                              const QString & contents,
                                              int cursorPos,
                                              int anchorPos );
    private Q_SLOTS:
        void slotChanged();

    private:
        Okular::FormFieldText * m_form;
        int m_prevCursorPos;
        int m_prevAnchorPos;
};

class TextAreaEdit : public KTextEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit TextAreaEdit( Okular::FormFieldText * text, QWidget * parent = Q_NULLPTR );
        void setFormWidgetsController( FormWidgetsController *controller );
        virtual bool event ( QEvent * e );


    public Q_SLOTS:
        void slotHandleTextChangedByUndoRedo( int pageNumber,
                                              Okular::FormFieldText * textForm,
                                              const QString & contents,
                                              int cursorPos,
                                              int anchorPos );
        void slotUpdateUndoAndRedoInContextMenu( QMenu* menu );

    private Q_SLOTS:
        void slotChanged();

    private:
        Okular::FormFieldText * m_form;
        int m_prevCursorPos;
        int m_prevAnchorPos;
};


class FileEdit : public KUrlRequester, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit FileEdit( Okular::FormFieldText * text, QWidget * parent = Q_NULLPTR );
        void setFormWidgetsController( FormWidgetsController *controller );

    protected:
        bool eventFilter( QObject *obj, QEvent *event );


    private Q_SLOTS:
        void slotChanged();
        void slotHandleFileChangedByUndoRedo( int pageNumber,
                                              Okular::FormFieldText * form,
                                              const QString & contents,
                                              int cursorPos,
                                              int anchorPos );
    private:
        Okular::FormFieldText * m_form;
        int m_prevCursorPos;
        int m_prevAnchorPos;
};


class ListEdit : public QListWidget, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit ListEdit( Okular::FormFieldChoice * choice, QWidget * parent = Q_NULLPTR );
        void setFormWidgetsController( FormWidgetsController *controller );

    private Q_SLOTS:
        void slotSelectionChanged();
        void slotHandleFormListChangedByUndoRedo( int pageNumber,
                                                  Okular::FormFieldChoice * listForm,
                                                  const QList< int > & choices );

    private:
        Okular::FormFieldChoice * m_form;
};


class ComboEdit : public QComboBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit ComboEdit( Okular::FormFieldChoice * choice, QWidget * parent = Q_NULLPTR );
        void setFormWidgetsController( FormWidgetsController *controller );
        virtual bool event ( QEvent * e );
        virtual void contextMenuEvent( QContextMenuEvent* event );

    private Q_SLOTS:
        void slotValueChanged();
        void slotHandleFormComboChangedByUndoRedo( int pageNumber,
                                                   Okular::FormFieldChoice * comboForm,
                                                   const QString & text,
                                                   int cursorPos,
                                                   int anchorPos
                                                 );

    private:
        Okular::FormFieldChoice * m_form;
        int m_prevCursorPos;
        int m_prevAnchorPos;
};

#endif
