/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
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

        void registerRadioButton( FormWidgetIface *fwButton, Okular::FormFieldButton *formButton );
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
        QHash< int, QAbstractButton* > m_buttons;
        Okular::Document* m_doc;
};


class FormWidgetFactory
{
    public:
        static FormWidgetIface * createWidget( Okular::FormField * ff, QWidget * parent = nullptr );
};


class FormWidgetIface
{
    public:
        FormWidgetIface( QWidget * w, Okular::FormField * ff, bool canBeEnabled );
        virtual ~FormWidgetIface();

        Okular::NormalizedRect rect() const;
        void setWidthHeight( int w, int h );
        void moveTo( int x, int y );
        bool setVisibility( bool visible );
        void setCanBeFilled( bool fill );

        void setPageItem( PageViewItem *pageItem );
        PageViewItem* pageItem() const;
        void setFormField( Okular::FormField *field );
        Okular::FormField* formField() const;

        virtual void setFormWidgetsController( FormWidgetsController *controller );

    protected:
        FormWidgetsController * m_controller;
        Okular::FormField * m_ff;

    private:
        QWidget * m_widget;
        PageViewItem * m_pageItem;
        bool m_canBeEnabled;
};


class PushButtonEdit : public QPushButton, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit PushButtonEdit( Okular::FormFieldButton * button, QWidget * parent = nullptr );

    private Q_SLOTS:
        void slotClicked();
};

class CheckBoxEdit : public QCheckBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit CheckBoxEdit( Okular::FormFieldButton * button, QWidget * parent = nullptr );

        // reimplemented from FormWidgetIface
        void setFormWidgetsController( FormWidgetsController *controller ) override;

    private Q_SLOTS:
        void slotStateChanged( int state );
};

class RadioButtonEdit : public QRadioButton, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit RadioButtonEdit( Okular::FormFieldButton * button, QWidget * parent = nullptr );

        // reimplemented from FormWidgetIface
        void setFormWidgetsController( FormWidgetsController *controller ) override;
};

class FormLineEdit : public QLineEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit FormLineEdit( Okular::FormFieldText * text, QWidget * parent = nullptr );
        void setFormWidgetsController( FormWidgetsController *controller ) override;
        bool event ( QEvent * e ) override;
        void contextMenuEvent( QContextMenuEvent* event ) override;


    public Q_SLOTS:
        void slotHandleTextChangedByUndoRedo( int pageNumber,
                                              Okular::FormFieldText* textForm,
                                              const QString & contents,
                                              int cursorPos,
                                              int anchorPos );
    private Q_SLOTS:
        void slotChanged();

    private:
        int m_prevCursorPos;
        int m_prevAnchorPos;
};

class TextAreaEdit : public KTextEdit, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit TextAreaEdit( Okular::FormFieldText * text, QWidget * parent = nullptr );
        void setFormWidgetsController( FormWidgetsController *controller ) override;
        bool event ( QEvent * e ) override;


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
        int m_prevCursorPos;
        int m_prevAnchorPos;
};


class FileEdit : public KUrlRequester, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit FileEdit( Okular::FormFieldText * text, QWidget * parent = nullptr );
        void setFormWidgetsController( FormWidgetsController *controller ) override;

    protected:
        bool eventFilter( QObject *obj, QEvent *event ) override;


    private Q_SLOTS:
        void slotChanged();
        void slotHandleFileChangedByUndoRedo( int pageNumber,
                                              Okular::FormFieldText * form,
                                              const QString & contents,
                                              int cursorPos,
                                              int anchorPos );
    private:
        int m_prevCursorPos;
        int m_prevAnchorPos;
};


class ListEdit : public QListWidget, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit ListEdit( Okular::FormFieldChoice * choice, QWidget * parent = nullptr );
        void setFormWidgetsController( FormWidgetsController *controller ) override;

    private Q_SLOTS:
        void slotSelectionChanged();
        void slotHandleFormListChangedByUndoRedo( int pageNumber,
                                                  Okular::FormFieldChoice * listForm,
                                                  const QList< int > & choices );
};


class ComboEdit : public QComboBox, public FormWidgetIface
{
    Q_OBJECT

    public:
        explicit ComboEdit( Okular::FormFieldChoice * choice, QWidget * parent = nullptr );
        void setFormWidgetsController( FormWidgetsController *controller ) override;
        bool event ( QEvent * e ) override;
        void contextMenuEvent( QContextMenuEvent* event ) override;

    private Q_SLOTS:
        void slotValueChanged();
        void slotHandleFormComboChangedByUndoRedo( int pageNumber,
                                                   Okular::FormFieldChoice * comboForm,
                                                   const QString & text,
                                                   int cursorPos,
                                                   int anchorPos
                                                 );

    private:
        int m_prevCursorPos;
        int m_prevAnchorPos;
};

#endif
