/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_FORMWIDGETS_H_
#define _OKULAR_FORMWIDGETS_H_

#include "core/area.h"
#include "core/form.h"

#include <KTextEdit>
#include <KUrlRequester>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>

class ComboEdit;
class QMenu;
class QButtonGroup;
class FormWidgetIface;
class PageView;
class PageViewItem;
class RadioButtonEdit;
class QEvent;

namespace Okular
{
class Action;
class FormField;
class FormFieldButton;
class FormFieldChoice;
class FormFieldText;
class FormFieldSignature;
class Document;
}

struct RadioData {
    RadioData()
    {
    }

    QList<int> ids;
    QButtonGroup *group;
};

class FormWidgetsController : public QObject
{
    Q_OBJECT

public:
    explicit FormWidgetsController(Okular::Document *doc);
    ~FormWidgetsController() override;

    void signalAction(Okular::Action *action);

    void processScriptAction(Okular::Action *a, Okular::FormField *field, Okular::Annotation::AdditionalActionType type);

    void registerRadioButton(FormWidgetIface *fwButton, Okular::FormFieldButton *formButton);
    void dropRadioButtons();
    bool canUndo();
    bool canRedo();

    Okular::Document *document() const;

    static bool shouldFormWidgetBeShown(Okular::FormField *form);

Q_SIGNALS:
    void changed(int pageNumber);
    void requestUndo();
    void requestRedo();
    void canUndoChanged(bool undoAvailable);
    void canRedoChanged(bool redoAvailable);
    void formTextChangedByWidget(int pageNumber, Okular::FormFieldText *form, const QString &newContents, int newCursorPos, int prevCursorPos, int prevAnchorPos);

    void formTextChangedByUndoRedo(int pageNumber, Okular::FormFieldText *form, const QString &contents, int cursorPos, int anchorPos);

    void formListChangedByWidget(int pageNumber, Okular::FormFieldChoice *form, const QList<int> &newChoices);

    void formListChangedByUndoRedo(int pageNumber, Okular::FormFieldChoice *form, const QList<int> &choices);

    void formComboChangedByWidget(int pageNumber, Okular::FormFieldChoice *form, const QString &newText, int newCursorPos, int prevCursorPos, int prevAnchorPos);

    void formComboChangedByUndoRedo(int pageNumber, Okular::FormFieldChoice *form, const QString &text, int cursorPos, int anchorPos);

    void formButtonsChangedByWidget(int pageNumber, const QList<Okular::FormFieldButton *> &formButtons, const QList<bool> &newButtonStates);

    void action(Okular::Action *action);

    void refreshFormWidget(Okular::FormField *form);

private Q_SLOTS:
    void slotButtonClicked(QAbstractButton *button);
    void slotFormButtonsChangedByUndoRedo(int pageNumber, const QList<Okular::FormFieldButton *> &formButtons);

private:
    friend class TextAreaEdit;
    friend class FormLineEdit;
    friend class FileEdit;
    friend class ListEdit;
    friend class ComboEdit;
    friend class SignatureEdit;

    QList<RadioData> m_radios;
    QHash<int, QAbstractButton *> m_buttons;
    Okular::Document *m_doc;
};

class FormWidgetFactory
{
public:
    static FormWidgetIface *createWidget(Okular::FormField *ff, PageView *pageView);
};

class FormWidgetIface
{
public:
    FormWidgetIface(QWidget *w, Okular::FormField *ff);
    virtual ~FormWidgetIface();

    FormWidgetIface(const FormWidgetIface &) = delete;
    FormWidgetIface &operator=(const FormWidgetIface &) = delete;

    Okular::NormalizedRect rect() const;
    void setWidthHeight(int w, int h);
    void moveTo(int x, int y);
    bool setVisibility(bool visible);
    void setCanBeFilled(bool fill);

    void setPageItem(PageViewItem *pageItem);
    PageViewItem *pageItem() const;
    void setFormField(Okular::FormField *field);
    Okular::FormField *formField() const;

    virtual void setFormWidgetsController(FormWidgetsController *controller);

protected:
    virtual void slotRefresh(Okular::FormField *form);

    FormWidgetsController *m_controller;
    Okular::FormField *m_ff;

private:
    QWidget *m_widget;
    PageViewItem *m_pageItem;
};

#define DECLARE_ADDITIONAL_ACTIONS                                                                                                                                                                                                             \
protected:                                                                                                                                                                                                                                     \
    virtual void mousePressEvent(QMouseEvent *event) override;                                                                                                                                                                                 \
    virtual void mouseReleaseEvent(QMouseEvent *event) override;                                                                                                                                                                               \
    virtual void focusInEvent(QFocusEvent *event) override;                                                                                                                                                                                    \
    virtual void focusOutEvent(QFocusEvent *event) override;                                                                                                                                                                                   \
    virtual void leaveEvent(QEvent *event) override;                                                                                                                                                                                           \
    virtual void enterEvent(QEvent *event) override;

class PushButtonEdit : public QPushButton, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit PushButtonEdit(Okular::FormFieldButton *button, PageView *pageView);

    DECLARE_ADDITIONAL_ACTIONS
};

class CheckBoxEdit : public QCheckBox, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit CheckBoxEdit(Okular::FormFieldButton *button, PageView *pageView);

    // reimplemented from FormWidgetIface
    void setFormWidgetsController(FormWidgetsController *controller) override;

    void doActivateAction();

protected:
    void slotRefresh(Okular::FormField *form) override;
    DECLARE_ADDITIONAL_ACTIONS
};

class RadioButtonEdit : public QRadioButton, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit RadioButtonEdit(Okular::FormFieldButton *button, PageView *pageView);

    // reimplemented from FormWidgetIface
    void setFormWidgetsController(FormWidgetsController *controller) override;
    DECLARE_ADDITIONAL_ACTIONS
};

class FormLineEdit : public QLineEdit, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit FormLineEdit(Okular::FormFieldText *text, PageView *pageView);
    void setFormWidgetsController(FormWidgetsController *controller) override;
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

public Q_SLOTS:
    void slotHandleTextChangedByUndoRedo(int pageNumber, Okular::FormFieldText *textForm, const QString &contents, int cursorPos, int anchorPos);
private Q_SLOTS:
    void slotChanged();

protected:
    void slotRefresh(Okular::FormField *form) override;

private:
    int m_prevCursorPos;
    int m_prevAnchorPos;
    bool m_editing;
    DECLARE_ADDITIONAL_ACTIONS
};

class TextAreaEdit : public KTextEdit, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit TextAreaEdit(Okular::FormFieldText *text, PageView *pageView);
    ~TextAreaEdit() override;
    void setFormWidgetsController(FormWidgetsController *controller) override;
    bool event(QEvent *e) override;

public Q_SLOTS:
    void slotHandleTextChangedByUndoRedo(int pageNumber, Okular::FormFieldText *textForm, const QString &contents, int cursorPos, int anchorPos);
    void slotUpdateUndoAndRedoInContextMenu(QMenu *menu);

private Q_SLOTS:
    void slotChanged();

protected:
    void slotRefresh(Okular::FormField *form) override;

private:
    int m_prevCursorPos;
    int m_prevAnchorPos;
    bool m_editing;
    DECLARE_ADDITIONAL_ACTIONS
};

class FileEdit : public KUrlRequester, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit FileEdit(Okular::FormFieldText *text, PageView *pageView);
    void setFormWidgetsController(FormWidgetsController *controller) override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void slotChanged();
    void slotHandleFileChangedByUndoRedo(int pageNumber, Okular::FormFieldText *form, const QString &contents, int cursorPos, int anchorPos);

private:
    int m_prevCursorPos;
    int m_prevAnchorPos;
    DECLARE_ADDITIONAL_ACTIONS
};

class ListEdit : public QListWidget, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit ListEdit(Okular::FormFieldChoice *choice, PageView *pageView);
    void setFormWidgetsController(FormWidgetsController *controller) override;

private Q_SLOTS:
    void slotSelectionChanged();
    void slotHandleFormListChangedByUndoRedo(int pageNumber, Okular::FormFieldChoice *listForm, const QList<int> &choices);
    DECLARE_ADDITIONAL_ACTIONS
};

class ComboEdit : public QComboBox, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit ComboEdit(Okular::FormFieldChoice *choice, PageView *pageView);
    void setFormWidgetsController(FormWidgetsController *controller) override;
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private Q_SLOTS:
    void slotValueChanged();
    void slotHandleFormComboChangedByUndoRedo(int pageNumber, Okular::FormFieldChoice *comboForm, const QString &text, int cursorPos, int anchorPos);

private:
    int m_prevCursorPos;
    int m_prevAnchorPos;
    DECLARE_ADDITIONAL_ACTIONS
};

class SignatureEdit : public QAbstractButton, public FormWidgetIface
{
    Q_OBJECT

public:
    explicit SignatureEdit(Okular::FormFieldSignature *signature, PageView *pageView);

    // This will be called when an item in signature panel is clicked. Calling it changes the
    // widget state. If this widget was visible prior to calling this then background
    // color will change and borders will remain otherwise visibility of this widget will change.
    // During the change all interactions will be disabled.
    void setDummyMode(bool set);

protected:
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private Q_SLOTS:
    void slotViewProperties();
    void signUnsignedSignature();

private:
    bool m_widgetPressed;
    bool m_dummyMode;
    bool m_wasVisible; // this will help in deciding whether or not to paint border for this widget

    DECLARE_ADDITIONAL_ACTIONS
};

#undef DECLARE_ADDITIONAL_ACTIONS

#endif
