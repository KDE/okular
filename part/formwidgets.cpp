/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "formwidgets.h"
#include "core/page.h"
#include "pageview.h"
#include "pageviewutils.h"
#include "revisionviewer.h"
#include "signaturepartutils.h"
#include "signaturepropertiesdialog.h"

#include <KLineEdit>
#include <KLocalizedString>
#include <KStandardAction>
#include <QAction>
#include <QButtonGroup>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QStylePainter>
#include <QUrl>

// local includes
#include "core/action.h"
#include "core/document.h"
#include "gui/debug_ui.h"

FormWidgetsController::FormWidgetsController(Okular::Document *doc)
    : QObject(doc)
    , m_doc(doc)
{
    // Q_EMIT changed signal when a form has changed
    connect(this, &FormWidgetsController::formTextChangedByUndoRedo, this, &FormWidgetsController::changed);
    connect(this, &FormWidgetsController::formListChangedByUndoRedo, this, &FormWidgetsController::changed);
    connect(this, &FormWidgetsController::formComboChangedByUndoRedo, this, &FormWidgetsController::changed);

    // connect form modification signals to and from document
    // connect to the newer editFormText method and not the deprecated one
    using EditFormTextType = void (Okular::Document::*)(int, Okular::FormFieldText *, const QString &, int, int, int, const QString &);
    connect(this, &FormWidgetsController::formTextChangedByWidget, doc, static_cast<EditFormTextType>(&Okular::Document::editFormText));
    connect(doc, &Okular::Document::formTextChangedByUndoRedo, this, &FormWidgetsController::formTextChangedByUndoRedo);

    connect(this, &FormWidgetsController::formListChangedByWidget, doc, &Okular::Document::editFormList);
    connect(doc, &Okular::Document::formListChangedByUndoRedo, this, &FormWidgetsController::formListChangedByUndoRedo);

    connect(this, &FormWidgetsController::formComboChangedByWidget, doc, &Okular::Document::editFormCombo);
    connect(doc, &Okular::Document::formComboChangedByUndoRedo, this, &FormWidgetsController::formComboChangedByUndoRedo);

    connect(this, &FormWidgetsController::formButtonsChangedByWidget, doc, &Okular::Document::editFormButtons);
    connect(doc, &Okular::Document::formButtonsChangedByUndoRedo, this, &FormWidgetsController::slotFormButtonsChangedByUndoRedo);

    // Connect undo/redo signals
    connect(this, &FormWidgetsController::requestUndo, doc, &Okular::Document::undo);
    connect(this, &FormWidgetsController::requestRedo, doc, &Okular::Document::redo);

    connect(doc, &Okular::Document::canUndoChanged, this, &FormWidgetsController::canUndoChanged);
    connect(doc, &Okular::Document::canRedoChanged, this, &FormWidgetsController::canRedoChanged);

    // Connect the generic formWidget refresh signal
    connect(doc, &Okular::Document::refreshFormWidget, this, &FormWidgetsController::refreshFormWidget);
}

FormWidgetsController::~FormWidgetsController()
{
}

void FormWidgetsController::signalAction(Okular::Action *a)
{
    Q_EMIT action(a);
}

void FormWidgetsController::signalMouseAction(Okular::Action *action, Okular::FormField *form, Okular::Document::MouseEventType fieldMouseEventType)
{
    Q_EMIT mouseAction(action, form, fieldMouseEventType);
}

void FormWidgetsController::processScriptAction(Okular::Action *a, Okular::FormField *field, Okular::Annotation::AdditionalActionType type)
{
    // If it's not a Action Script or if the field is neither a FormText nor a combobox FormChoice, handle it normally
    if (a->actionType() != Okular::Action::Script ||
        (field->type() != Okular::FormField::FormText && !(field->type() == Okular::FormField::FormChoice && (dynamic_cast<Okular::FormFieldChoice *>(field)->choiceType() == Okular::FormFieldChoice::ComboBox)))) {
        Q_EMIT action(a);
        return;
    }
    switch (type) {
    // These cases are to be handled by the FormFields themselves, so we let it happen.
    case Okular::Annotation::FocusIn:
    case Okular::Annotation::FocusOut:
        return;
    case Okular::Annotation::PageOpening:
    case Okular::Annotation::PageClosing:
    case Okular::Annotation::CursorEntering:
    case Okular::Annotation::CursorLeaving:
    case Okular::Annotation::MousePressed:
    case Okular::Annotation::MouseReleased:
        Q_EMIT action(a);
    }
}

void FormWidgetsController::registerRadioButton(FormWidgetIface *fwButton, Okular::FormFieldButton *formButton)
{
    if (!fwButton) {
        return;
    }

    QAbstractButton *button = dynamic_cast<QAbstractButton *>(fwButton);
    if (!button) {
        qWarning() << "fwButton is not a QAbstractButton" << fwButton;
        return;
    }

    QList<RadioData>::iterator it = m_radios.begin(), itEnd = m_radios.end();
    const int id = formButton->id();
    m_buttons.insert(id, button);
    for (; it != itEnd; ++it) {
        const RadioData &rd = *it;
        const QList<int>::const_iterator idsIt = std::find(rd.ids.begin(), rd.ids.end(), id);
        if (idsIt != rd.ids.constEnd()) {
            qCDebug(OkularUiDebug) << "Adding id" << id << "To group including" << rd.ids;
            rd.group->addButton(button);
            rd.group->setId(button, id);
            return;
        }
    }

    const QList<int> siblings = formButton->siblings();

    RadioData newdata;
    newdata.ids = siblings;
    newdata.ids.append(id);
    newdata.group = new QButtonGroup();
    newdata.group->addButton(button);
    newdata.group->setId(button, id);

    // Groups of 1 (like checkboxes) can't be exclusive
    if (siblings.isEmpty()) {
        newdata.group->setExclusive(false);
    }

    connect(newdata.group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &FormWidgetsController::slotButtonClicked);
    m_radios.append(newdata);
}

void FormWidgetsController::dropRadioButtons()
{
    QList<RadioData>::iterator it = m_radios.begin(), itEnd = m_radios.end();
    for (; it != itEnd; ++it) {
        delete (*it).group;
    }
    m_radios.clear();
    m_buttons.clear();
}

bool FormWidgetsController::canUndo()
{
    return m_doc->canUndo();
}

bool FormWidgetsController::canRedo()
{
    return m_doc->canRedo();
}

bool FormWidgetsController::shouldFormWidgetBeShown(Okular::FormField *form)
{
    return !form->isReadOnly() || form->type() == Okular::FormField::FormSignature;
}

void FormWidgetsController::slotButtonClicked(QAbstractButton *button)
{
    int pageNumber = -1;
    CheckBoxEdit *check = qobject_cast<CheckBoxEdit *>(button);
    if (check) {
        // Checkboxes need to be uncheckable so if clicking a checked one
        // disable the exclusive status temporarily and uncheck it
        Okular::FormFieldButton *formButton = static_cast<Okular::FormFieldButton *>(check->formField());
        if (formButton->state()) {
            const bool wasExclusive = button->group()->exclusive();
            button->group()->setExclusive(false);
            check->setChecked(false);
            button->group()->setExclusive(wasExclusive);
        }
        pageNumber = check->pageItem()->pageNumber();
    } else if (const RadioButtonEdit *radio = qobject_cast<RadioButtonEdit *>(button)) {
        pageNumber = radio->pageItem()->pageNumber();
    }

    const QList<QAbstractButton *> buttons = button->group()->buttons();
    QList<bool> checked;
    QList<bool> prevChecked;
    QList<Okular::FormFieldButton *> formButtons;

    for (QAbstractButton *btn : buttons) {
        checked.append(btn->isChecked());
        Okular::FormFieldButton *formButton = static_cast<Okular::FormFieldButton *>(dynamic_cast<FormWidgetIface *>(btn)->formField());
        formButtons.append(formButton);
        prevChecked.append(formButton->state());
    }
    if (checked != prevChecked) {
        Q_EMIT formButtonsChangedByWidget(pageNumber, formButtons, checked);
    }
    if (check) {
        // The formButtonsChangedByWidget signal changes the value of the underlying
        // Okular::FormField of the checkbox. We need to execute the activation
        // action after this.
        check->doActivateAction();
    }
}

void FormWidgetsController::slotFormButtonsChangedByUndoRedo(int pageNumber, const QList<Okular::FormFieldButton *> &formButtons)
{
    QList<int> extraPages;
    for (const Okular::FormFieldButton *formButton : formButtons) {
        int id = formButton->id();
        QAbstractButton *button = m_buttons[id];
        int itemPageNumber = -1;
        if (const CheckBoxEdit *check = qobject_cast<CheckBoxEdit *>(button)) {
            itemPageNumber = check->pageItem()->pageNumber();
            Q_EMIT refreshFormWidget(check->formField());
        } else if (const RadioButtonEdit *radio = qobject_cast<RadioButtonEdit *>(button)) {
            itemPageNumber = radio->pageItem()->pageNumber();
        }
        // temporarily disable exclusiveness of the button group
        // since it breaks doing/redoing steps into which all the checkboxes
        // are unchecked
        const bool wasExclusive = button->group()->exclusive();
        button->group()->setExclusive(false);
        bool checked = formButton->state();
        button->setChecked(checked);
        button->group()->setExclusive(wasExclusive);
        button->setFocus();
        if (itemPageNumber != -1 && itemPageNumber != pageNumber) {
            extraPages << itemPageNumber;
        }
    }
    Q_EMIT changed(pageNumber);
    for (auto page : std::as_const(extraPages)) {
        Q_EMIT changed(page);
    }
}

Okular::Document *FormWidgetsController::document() const
{
    return m_doc;
}

FormWidgetIface *FormWidgetFactory::createWidget(Okular::FormField *ff, PageView *pageView)
{
    FormWidgetIface *widget = nullptr;

    switch (ff->type()) {
    case Okular::FormField::FormButton: {
        Okular::FormFieldButton *ffb = static_cast<Okular::FormFieldButton *>(ff);
        switch (ffb->buttonType()) {
        case Okular::FormFieldButton::Push:
            widget = new PushButtonEdit(ffb, pageView);
            break;
        case Okular::FormFieldButton::CheckBox:
            widget = new CheckBoxEdit(ffb, pageView);
            break;
        case Okular::FormFieldButton::Radio:
            widget = new RadioButtonEdit(ffb, pageView);
            break;
        default:;
        }
        break;
    }
    case Okular::FormField::FormText: {
        Okular::FormFieldText *fft = static_cast<Okular::FormFieldText *>(ff);
        switch (fft->textType()) {
        case Okular::FormFieldText::Multiline:
            widget = new TextAreaEdit(fft, pageView);
            break;
        case Okular::FormFieldText::Normal:
            widget = new FormLineEdit(fft, pageView);
            break;
        case Okular::FormFieldText::FileSelect:
            widget = new FileEdit(fft, pageView);
            break;
        }
        break;
    }
    case Okular::FormField::FormChoice: {
        Okular::FormFieldChoice *ffc = static_cast<Okular::FormFieldChoice *>(ff);
        switch (ffc->choiceType()) {
        case Okular::FormFieldChoice::ListBox:
            widget = new ListEdit(ffc, pageView);
            break;
        case Okular::FormFieldChoice::ComboBox:
            widget = new ComboEdit(ffc, pageView);
            break;
        }
        break;
    }
    case Okular::FormField::FormSignature: {
        Okular::FormFieldSignature *ffs = static_cast<Okular::FormFieldSignature *>(ff);
        if (ffs->isVisible() && ffs->signatureType() != Okular::FormFieldSignature::UnknownType) {
            widget = new SignatureEdit(ffs, pageView);
        }
        break;
    }
    default:;
    }

    if (!FormWidgetsController::shouldFormWidgetBeShown(ff)) {
        widget->setVisibility(false);
    }

    return widget;
}

FormWidgetIface::FormWidgetIface(QWidget *w, Okular::FormField *ff)
    : m_controller(nullptr)
    , m_ff(ff)
    , m_widget(w)
    , m_pageItem(nullptr)
{
    if (!m_ff->uiName().isEmpty()) {
        m_widget->setToolTip(m_ff->uiName());
    }
}

FormWidgetIface::~FormWidgetIface()
{
}

Okular::NormalizedRect FormWidgetIface::rect() const
{
    return m_ff->rect();
}

void FormWidgetIface::setWidthHeight(int w, int h)
{
    m_widget->resize(w, h);
}

void FormWidgetIface::moveTo(int x, int y)
{
    m_widget->move(x, y);
}

bool FormWidgetIface::setVisibility(bool visible)
{
    bool hadfocus = m_widget->hasFocus();
    if (hadfocus && !visible) {
        m_widget->clearFocus();
    }
    m_widget->setVisible(visible);
    return hadfocus;
}

void FormWidgetIface::setCanBeFilled(bool fill)
{
    m_widget->setEnabled(fill);
}

void FormWidgetIface::setPageItem(PageViewItem *pageItem)
{
    m_pageItem = pageItem;
}

void FormWidgetIface::setFormField(Okular::FormField *field)
{
    m_ff = field;
}

Okular::FormField *FormWidgetIface::formField() const
{
    return m_ff;
}

PageViewItem *FormWidgetIface::pageItem() const
{
    return m_pageItem;
}

void FormWidgetIface::setFormWidgetsController(FormWidgetsController *controller)
{
    m_controller = controller;
    QObject *obj = dynamic_cast<QObject *>(this);
    QObject::connect(m_controller, &FormWidgetsController::refreshFormWidget, obj, [this](Okular::FormField *form) { slotRefresh(form); });
}

void FormWidgetIface::slotRefresh(Okular::FormField *form)
{
    if (m_ff != form) {
        return;
    }
    setVisibility(form->isVisible() && m_controller->shouldFormWidgetBeShown(form));

    m_widget->setEnabled(!form->isReadOnly());
}

PushButtonEdit::PushButtonEdit(Okular::FormFieldButton *button, PageView *pageView)
    : QPushButton(pageView->viewport())
    , FormWidgetIface(this, button)
{
    setText(button->caption());

    if (button->caption().isEmpty()) {
        setFlat(true);
    }

    setVisible(button->isVisible());
    setCursor(Qt::ArrowCursor);
}

CheckBoxEdit::CheckBoxEdit(Okular::FormFieldButton *button, PageView *pageView)
    : QCheckBox(pageView->viewport())
    , FormWidgetIface(this, button)
{
    setVisible(button->isVisible());
    setCursor(Qt::ArrowCursor);
}

void CheckBoxEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    Okular::FormFieldButton *form = static_cast<Okular::FormFieldButton *>(m_ff);
    FormWidgetIface::setFormWidgetsController(controller);
    m_controller->registerRadioButton(this, form);
    setChecked(form->state());
}

void CheckBoxEdit::doActivateAction()
{
    Okular::FormFieldButton *form = static_cast<Okular::FormFieldButton *>(m_ff);
    if (form->activationAction()) {
        m_controller->signalAction(form->activationAction());
    }
}

void CheckBoxEdit::slotRefresh(Okular::FormField *form)
{
    if (form != m_ff) {
        return;
    }
    FormWidgetIface::slotRefresh(form);

    Okular::FormFieldButton *button = static_cast<Okular::FormFieldButton *>(m_ff);
    bool oldState = isChecked();
    bool newState = button->state();
    if (oldState != newState) {
        setChecked(button->state());
        doActivateAction();
    }
}

void CheckBoxEdit::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStylePainter p(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);

    int indicatorSize = qMin(width(), height());
    QRect indicatorRect = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, this);
    indicatorRect.setSize(QSize(indicatorSize, indicatorSize));
    indicatorRect.moveCenter(opt.rect.center());

    opt.rect = indicatorRect;

    // TODO draw different shapes of check boxes depending upon their style.
    p.drawPrimitive(QStyle::PE_IndicatorCheckBox, opt);
}

bool CheckBoxEdit::hitButton(const QPoint &pos) const
{
    return QWidget::rect().contains(pos);
}

RadioButtonEdit::RadioButtonEdit(Okular::FormFieldButton *button, PageView *pageView)
    : QRadioButton(pageView->viewport())
    , FormWidgetIface(this, button)
{
    setVisible(button->isVisible());
    setCursor(Qt::ArrowCursor);
}

void RadioButtonEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    Okular::FormFieldButton *form = static_cast<Okular::FormFieldButton *>(m_ff);
    FormWidgetIface::setFormWidgetsController(controller);
    m_controller->registerRadioButton(this, form);
    setChecked(form->state());
}

void RadioButtonEdit::slotRefresh(Okular::FormField *form)
{
    if (form != m_ff) {
        return;
    }
    FormWidgetIface::slotRefresh(form);

    Okular::FormFieldButton *button = static_cast<Okular::FormFieldButton *>(m_ff);
    bool oldState = isChecked();
    bool newState = button->state();
    if (oldState != newState) {
        setChecked(button->state());
        if (form->activationAction()) {
            m_controller->signalAction(form->activationAction());
        }
    }
}

void RadioButtonEdit::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStylePainter p(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);

    int indicatorSize = qMin(width(), height());
    QRect indicatorRect = style()->subElementRect(QStyle::SE_RadioButtonIndicator, &opt, this);
    indicatorRect.setSize(QSize(indicatorSize, indicatorSize));
    indicatorRect.moveCenter(opt.rect.center());

    opt.rect = indicatorRect;

    // TODO draw different shapes of radio buttons depending upon their style.
    p.drawPrimitive(QStyle::PE_IndicatorRadioButton, opt);
}

bool RadioButtonEdit::hitButton(const QPoint &pos) const
{
    return QWidget::rect().contains(pos);
}

FormLineEdit::FormLineEdit(Okular::FormFieldText *text, PageView *pageView)
    : QLineEdit(pageView->viewport())
    , FormWidgetIface(this, text)
{
    int maxlen = text->maximumLength();
    if (maxlen >= 0) {
        setMaxLength(maxlen);
    }
    setAlignment(text->textAlignment());
    setText(text->text());
    if (text->isPassword()) {
        setEchoMode(QLineEdit::Password);
    }

    m_prevCursorPos = cursorPosition();
    m_prevAnchorPos = cursorPosition();
    m_editing = false;

    connect(this, &QLineEdit::textEdited, this, &FormLineEdit::slotChanged);
    connect(this, &QLineEdit::cursorPositionChanged, this, &FormLineEdit::slotChanged);

    setVisible(text->isVisible());
}

void FormLineEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect(m_controller, &FormWidgetsController::formTextChangedByUndoRedo, this, &FormLineEdit::slotHandleTextChangedByUndoRedo);
}

bool FormLineEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
            m_controller->document()->processKVCFActions(m_ff);
            return true;
        }
        if (keyEvent == QKeySequence::Undo) {
            Q_EMIT m_controller->requestUndo();
            return true;
        } else if (keyEvent == QKeySequence::Redo) {
            Q_EMIT m_controller->requestRedo();
            return true;
        }
    } else if (e->type() == QEvent::FocusIn) {
        const auto fft = static_cast<Okular::FormFieldText *>(m_ff);
        if (text() != fft->text()) {
            setText(fft->text());
        }
        m_editing = true;

        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(e);
        if (focusEvent->reason() != Qt::ActiveWindowFocusReason) {
            if (const Okular::Action *action = m_ff->additionalAction(Okular::Annotation::FocusIn)) {
                m_controller->document()->processFocusAction(action, fft);
            }
        }
        setFocus();
    } else if (e->type() == QEvent::FocusOut) {
        m_editing = false;

        // Don't worry about focus events from other sources than the user FocusEvent to edit the field
        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(e);
        if (focusEvent->reason() == Qt::OtherFocusReason || focusEvent->reason() == Qt::ActiveWindowFocusReason) {
            return true;
        }

        m_controller->document()->processKVCFActions(m_ff);

        if (const Okular::Action *action = m_ff->additionalAction(Okular::Annotation::FocusOut)) {
            m_controller->document()->processFocusAction(action, static_cast<Okular::FormFieldText *>(m_ff));
        }
    }
    return QLineEdit::event(e);
}

void FormLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, DeleteAct, SelectAllAct };

    QAction *kundo = KStandardAction::create(KStandardAction::Undo, m_controller, SIGNAL(requestUndo()), menu);
    QAction *kredo = KStandardAction::create(KStandardAction::Redo, m_controller, SIGNAL(requestRedo()), menu);
    connect(m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled);
    connect(m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(m_controller->canUndo());
    kredo->setEnabled(m_controller->canRedo());

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction(oldUndo, kundo);
    menu->insertAction(oldRedo, kredo);

    menu->removeAction(oldUndo);
    menu->removeAction(oldRedo);

    menu->exec(event->globalPos());
    delete menu;
}

void FormLineEdit::slotChanged()
{
    Okular::FormFieldText *form = static_cast<Okular::FormFieldText *>(m_ff);
    QString newWidgetValueBeforeKeystroke = text();
    QString oldContents = form->text();
    int cursorPos;

    if (newWidgetValueBeforeKeystroke != oldContents) {
        if (form->additionalAction(Okular::FormField::FieldModified) && m_editing && !form->isReadOnly()) {
            m_controller->document()->processKeystrokeAction(form->additionalAction(Okular::FormField::FieldModified), form, text(), m_prevCursorPos, m_prevAnchorPos);
        }
        // calculate cursor position after keystroke action since it is possible that the change was not accepted and widget was refreshed.
        cursorPos = cursorPosition();
        if (newWidgetValueBeforeKeystroke == text()) {
            // if widget was not refreshed then emit this signal.
            Q_EMIT m_controller->formTextChangedByWidget(pageItem()->pageNumber(), form, text(), cursorPos, m_prevCursorPos, m_prevAnchorPos, oldContents);
        }
    } else {
        // we still evaluate the cursor position if no change occurs in widget contents.
        cursorPos = cursorPosition();
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = cursorPos;
    if (hasSelectedText()) {
        if (cursorPos == selectionStart()) {
            m_prevAnchorPos = selectionStart() + selectedText().size();
        } else {
            m_prevAnchorPos = selectionStart();
        }
    }
}

void FormLineEdit::slotHandleTextChangedByUndoRedo(int pageNumber, Okular::FormFieldText *textForm, const QString &contents, int cursorPos, int anchorPos)
{
    Q_UNUSED(pageNumber);
    if (textForm != m_ff || contents == text()) {
        return;
    }
    disconnect(this, &QLineEdit::cursorPositionChanged, this, &FormLineEdit::slotChanged);
    setText(contents);
    setCursorPosition(anchorPos);
    cursorForward(true, cursorPos - anchorPos);
    connect(this, &QLineEdit::cursorPositionChanged, this, &FormLineEdit::slotChanged);
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;

    // If the contents of the box have already lost focus, we need to run all the keystroke, validation, formatting scripts again.
    if (!hasFocus()) { // if lineEdit already had focus, undoing/redoing will still retain the focus and these scripts would execute when focus is lost or enter key is pressed.
        m_controller->document()->processKVCFActions(textForm);
    }
}

void FormLineEdit::slotRefresh(Okular::FormField *form)
{
    if (form != m_ff) {
        return;
    }
    FormWidgetIface::slotRefresh(form);

    Okular::FormFieldText *text = static_cast<Okular::FormFieldText *>(form);
    setText(text->text());
}

TextAreaEdit::TextAreaEdit(Okular::FormFieldText *text, PageView *pageView)
    : KTextEdit(pageView->viewport())
    , FormWidgetIface(this, text)
{
    setAcceptRichText(text->isRichText());
    setCheckSpellingEnabled(text->canBeSpellChecked());
    setAlignment(text->textAlignment());
    setPlainText(text->text());
    setUndoRedoEnabled(false);

    connect(this, &QTextEdit::textChanged, this, &TextAreaEdit::slotChanged);
    connect(this, &QTextEdit::cursorPositionChanged, this, &TextAreaEdit::slotChanged);
    connect(this, &KTextEdit::aboutToShowContextMenu, this, &TextAreaEdit::slotUpdateUndoAndRedoInContextMenu);
    m_prevCursorPos = textCursor().position();
    m_prevAnchorPos = textCursor().anchor();
    m_editing = false;
    setVisible(text->isVisible());
}

TextAreaEdit::~TextAreaEdit()
{
    // We need this because otherwise on destruction we destruct the syntax highlighter
    // that ends up calling text changed but then we go to slotChanged and we're already
    // half destructed and all is bad
    disconnect(this, &QTextEdit::textChanged, this, &TextAreaEdit::slotChanged);
}

bool TextAreaEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e); /* cppcheck-suppress constVariablePointer ; Qt doesn't have const equality operator*/
        if (keyEvent == QKeySequence::Undo) {
            Q_EMIT m_controller->requestUndo();
            return true;
        } else if (keyEvent == QKeySequence::Redo) {
            Q_EMIT m_controller->requestRedo();
            return true;
        }
    } else if (e->type() == QEvent::FocusIn) {
        const auto fft = static_cast<Okular::FormFieldText *>(m_ff);
        if (toPlainText() != fft->text()) {
            setText(fft->text());
        }
        m_editing = true;

        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(e);
        if (focusEvent->reason() != Qt::ActiveWindowFocusReason) {
            if (const Okular::Action *action = m_ff->additionalAction(Okular::Annotation::FocusIn)) {
                m_controller->document()->processFocusAction(action, fft);
            }
        }
        setFocus();
    } else if (e->type() == QEvent::FocusOut) {
        m_editing = false;

        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(e);
        if (focusEvent->reason() == Qt::OtherFocusReason || focusEvent->reason() == Qt::ActiveWindowFocusReason) {
            return true;
        }

        m_controller->document()->processKVCFActions(m_ff);

        if (const Okular::Action *action = m_ff->additionalAction(Okular::Annotation::FocusOut)) {
            m_controller->document()->processFocusAction(action, static_cast<Okular::FormFieldText *>(m_ff));
        }
    }
    return KTextEdit::event(e);
}

void TextAreaEdit::slotUpdateUndoAndRedoInContextMenu(QMenu *menu)
{
    if (!menu) {
        return;
    }

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, ClearAct, SelectAllAct, NCountActs };

    QAction *kundo = KStandardAction::create(KStandardAction::Undo, m_controller, SIGNAL(requestUndo()), menu);
    QAction *kredo = KStandardAction::create(KStandardAction::Redo, m_controller, SIGNAL(requestRedo()), menu);
    connect(m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled);
    connect(m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(m_controller->canUndo());
    kredo->setEnabled(m_controller->canRedo());

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction(oldUndo, kundo);
    menu->insertAction(oldRedo, kredo);

    menu->removeAction(oldUndo);
    menu->removeAction(oldRedo);
}

void TextAreaEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect(m_controller, &FormWidgetsController::formTextChangedByUndoRedo, this, &TextAreaEdit::slotHandleTextChangedByUndoRedo);
}

void TextAreaEdit::slotHandleTextChangedByUndoRedo(int pageNumber, Okular::FormFieldText *textForm, const QString &contents, int cursorPos, int anchorPos)
{
    Q_UNUSED(pageNumber);
    if (textForm != m_ff) {
        return;
    }
    setPlainText(contents);
    QTextCursor c = textCursor();
    c.setPosition(anchorPos);
    c.setPosition(cursorPos, QTextCursor::KeepAnchor);
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    setTextCursor(c);

    // If the contents of the box have already lost focus, we need to run all the keystroke, validation, formatting scripts again.
    if (!hasFocus()) { // if lineEdit already had focus, undoing/redoing will still retain the focus and these scripts would execute when focus is lost or enter key is pressed.
        m_controller->document()->processKVCFActions(textForm);
    }
}

void TextAreaEdit::slotChanged()
{
    Okular::FormFieldText *form = static_cast<Okular::FormFieldText *>(m_ff);
    QString newWidgetValueBeforeKeystroke = toPlainText();
    QString oldContents = form->text();
    int cursorPos;

    if (newWidgetValueBeforeKeystroke != oldContents) {
        if (form->additionalAction(Okular::FormField::FieldModified) && m_editing && !form->isReadOnly()) {
            m_controller->document()->processKeystrokeAction(form->additionalAction(Okular::FormField::FieldModified), form, toPlainText(), m_prevCursorPos, m_prevAnchorPos);
        }
        // calculate cursor position after keystroke action since it is possible that the change was not accepted and widget was refreshed.
        cursorPos = textCursor().position();
        if (newWidgetValueBeforeKeystroke == toPlainText()) {
            // if widget was not refreshed then emit this signal.
            Q_EMIT m_controller->formTextChangedByWidget(pageItem()->pageNumber(), form, toPlainText(), cursorPos, m_prevCursorPos, m_prevAnchorPos, oldContents);
        }
    } else {
        // we still evaluate the cursor position if no change occurs in widget contents.
        cursorPos = textCursor().position();
    }
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = textCursor().anchor();
}

void TextAreaEdit::slotRefresh(Okular::FormField *form)
{
    if (form != m_ff) {
        return;
    }
    FormWidgetIface::slotRefresh(form);

    Okular::FormFieldText *text = static_cast<Okular::FormFieldText *>(form);
    setPlainText(text->text());
}

FileEdit::FileEdit(Okular::FormFieldText *text, PageView *pageView)
    : KUrlRequester(pageView->viewport())
    , FormWidgetIface(this, text)
{
    setMode(KFile::File | KFile::ExistingOnly | KFile::LocalOnly);
    setNameFilter(i18n("All Files (*)"));
    setUrl(QUrl::fromUserInput(text->text()));
    lineEdit()->setAlignment(text->textAlignment());

    m_prevCursorPos = lineEdit()->cursorPosition();
    m_prevAnchorPos = lineEdit()->cursorPosition();

    connect(this, &KUrlRequester::textChanged, this, &FileEdit::slotChanged);
    connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &FileEdit::slotChanged);
    setVisible(text->isVisible());
}

void FileEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect(m_controller, &FormWidgetsController::formTextChangedByUndoRedo, this, &FileEdit::slotHandleFileChangedByUndoRedo);
}

bool FileEdit::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == lineEdit()) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event); /* cppcheck-suppress constVariablePointer */
            if (keyEvent == QKeySequence::Undo) {
                Q_EMIT m_controller->requestUndo();
                return true;
            } else if (keyEvent == QKeySequence::Redo) {
                Q_EMIT m_controller->requestRedo();
                return true;
            }
        } else if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent *>(event);

            QMenu *menu = static_cast<QLineEdit *>(lineEdit())->createStandardContextMenu(); // we actually want the base class here and not the additions from KLineEdit

            QList<QAction *> actionList = menu->actions();
            enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, DeleteAct, SelectAllAct };

            QAction *kundo = KStandardAction::create(KStandardAction::Undo, m_controller, SIGNAL(requestUndo()), menu);
            QAction *kredo = KStandardAction::create(KStandardAction::Redo, m_controller, SIGNAL(requestRedo()), menu);
            connect(m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled);
            connect(m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled);
            kundo->setEnabled(m_controller->canUndo());
            kredo->setEnabled(m_controller->canRedo());

            QAction *oldUndo, *oldRedo;
            oldUndo = actionList[UndoAct];
            oldRedo = actionList[RedoAct];

            menu->insertAction(oldUndo, kundo);
            menu->insertAction(oldRedo, kredo);

            menu->removeAction(oldUndo);
            menu->removeAction(oldRedo);

            menu->exec(contextMenuEvent->globalPos());
            delete menu;
            return true;
        }
    }
    return KUrlRequester::eventFilter(obj, event);
}

void FileEdit::slotChanged()
{
    // Make sure line edit's text matches url expansion
    if (text() != url().toLocalFile()) {
        this->setText(url().toLocalFile());
    }

    Okular::FormFieldText *form = static_cast<Okular::FormFieldText *>(m_ff);

    QString contents = text();
    QString oldContents = form->text();
    int cursorPos = lineEdit()->cursorPosition();
    if (contents != oldContents) {
        Q_EMIT m_controller->formTextChangedByWidget(pageItem()->pageNumber(), form, contents, cursorPos, m_prevCursorPos, m_prevAnchorPos, oldContents);
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = cursorPos;
    if (lineEdit()->hasSelectedText()) {
        if (cursorPos == lineEdit()->selectionStart()) {
            m_prevAnchorPos = lineEdit()->selectionStart() + lineEdit()->selectedText().size();
        } else {
            m_prevAnchorPos = lineEdit()->selectionStart();
        }
    }
}

void FileEdit::slotHandleFileChangedByUndoRedo(int pageNumber, Okular::FormFieldText *form, const QString &contents, int cursorPos, int anchorPos)
{
    Q_UNUSED(pageNumber);
    if (form != m_ff || contents == text()) {
        return;
    }
    disconnect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &FileEdit::slotChanged);
    setText(contents);
    lineEdit()->setCursorPosition(anchorPos);
    lineEdit()->cursorForward(true, cursorPos - anchorPos);
    connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &FileEdit::slotChanged);
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    setFocus();
}

ListEdit::ListEdit(Okular::FormFieldChoice *choice, PageView *pageView)
    : QListWidget(pageView->viewport())
    , FormWidgetIface(this, choice)
{
    addItems(choice->choices());
    setSelectionMode(choice->multiSelect() ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    const QList<int> selectedItems = choice->currentChoices();
    if (choice->multiSelect()) {
        for (const int index : selectedItems) {
            if (index >= 0 && index < count()) {
                item(index)->setSelected(true);
            }
        }
    } else {
        if (selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count()) {
            setCurrentRow(selectedItems.at(0));
            scrollToItem(item(selectedItems.at(0)));
        }
    }

    connect(this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged);

    setVisible(choice->isVisible());
    setCursor(Qt::ArrowCursor);
}

void ListEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect(m_controller, &FormWidgetsController::formListChangedByUndoRedo, this, &ListEdit::slotHandleFormListChangedByUndoRedo);
}

void ListEdit::slotSelectionChanged()
{
    const QList<QListWidgetItem *> selection = selectedItems();
    QList<int> rows;
    for (const QListWidgetItem *item : selection) {
        rows.append(row(item));
    }
    Okular::FormFieldChoice *form = static_cast<Okular::FormFieldChoice *>(m_ff);
    if (rows != form->currentChoices()) {
        Q_EMIT m_controller->formListChangedByWidget(pageItem()->pageNumber(), form, rows);
    }
}

void ListEdit::slotHandleFormListChangedByUndoRedo(int pageNumber, Okular::FormFieldChoice *listForm, const QList<int> &choices)
{
    Q_UNUSED(pageNumber);

    if (m_ff != listForm) {
        return;
    }

    disconnect(this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged);
    for (int i = 0; i < count(); i++) {
        item(i)->setSelected(choices.contains(i));
    }
    connect(this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged);

    setFocus();
}

void ListEdit::slotRefresh(Okular::FormField *form)
{
    if (m_ff != form) {
        return;
    }
    FormWidgetIface::slotRefresh(form);

    Okular::FormFieldChoice *ffc = static_cast<Okular::FormFieldChoice *>(m_ff);
    const QList<int> selectedItems = ffc->currentChoices();
    disconnect(this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged);
    if (ffc->multiSelect()) {
        clearSelection();
        for (const int index : selectedItems) {
            if (index >= 0 && index < count()) {
                item(index)->setSelected(true);
            }
        }
    } else {
        if (selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count()) {
            setCurrentRow(selectedItems.at(0));
            scrollToItem(item(selectedItems.at(0)));
        }
    }
    connect(this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged);
}

ComboEdit::ComboEdit(Okular::FormFieldChoice *choice, PageView *pageView)
    : QComboBox(pageView->viewport())
    , FormWidgetIface(this, choice)
{
    addItems(choice->choices());
    setEditable(true);
    setInsertPolicy(NoInsert);
    lineEdit()->setReadOnly(!choice->isEditable());
    QList<int> selectedItems = choice->currentChoices();
    if (selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count()) {
        setCurrentIndex(selectedItems.at(0));
    }

    if (choice->isEditable() && !choice->editChoice().isEmpty()) {
        lineEdit()->setText(choice->editChoice());
    }

    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ComboEdit::slotValueChanged);
    connect(this, &QComboBox::editTextChanged, this, &ComboEdit::slotValueChanged);
    connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &ComboEdit::slotValueChanged);

    m_editing = false;
    setVisible(choice->isVisible());
    setCursor(Qt::ArrowCursor);
    m_prevCursorPos = lineEdit()->cursorPosition();
    m_prevAnchorPos = lineEdit()->cursorPosition();
}

void ComboEdit::setFormWidgetsController(FormWidgetsController *controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect(m_controller, &FormWidgetsController::formComboChangedByUndoRedo, this, &ComboEdit::slotHandleFormComboChangedByUndoRedo);
}

void ComboEdit::slotValueChanged()
{
    const QString text = lineEdit()->text();

    Okular::FormFieldChoice *form = static_cast<Okular::FormFieldChoice *>(m_ff);

    QString prevText;
    if (form->currentChoices().isEmpty()) {
        prevText = form->editChoice();
    } else {
        prevText = form->choices().at(form->currentChoices().constFirst());
    }

    int cursorPos;
    if (text != prevText) {
        if (form->additionalAction(Okular::FormField::FieldModified) && m_editing && !form->isReadOnly()) {
            m_controller->document()->processKeystrokeAction(form->additionalAction(Okular::FormField::FieldModified), form, lineEdit()->text(), m_prevCursorPos, m_prevAnchorPos);
        }
        cursorPos = lineEdit()->cursorPosition();
        if (text == lineEdit()->text()) {
            Q_EMIT m_controller->formComboChangedByWidget(pageItem()->pageNumber(), form, currentText(), cursorPos, m_prevCursorPos, m_prevAnchorPos);
        }
    } else {
        cursorPos = lineEdit()->cursorPosition();
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = cursorPos;
    if (lineEdit()->hasSelectedText()) {
        if (cursorPos == lineEdit()->selectionStart()) {
            m_prevAnchorPos = lineEdit()->selectionStart() + lineEdit()->selectedText().size();
        } else {
            m_prevAnchorPos = lineEdit()->selectionStart();
        }
    }
}

void ComboEdit::slotHandleFormComboChangedByUndoRedo(int pageNumber, Okular::FormFieldChoice *form, const QString &text, int cursorPos, int anchorPos)
{
    Q_UNUSED(pageNumber);

    if (m_ff != form) {
        return;
    }

    // Determine if text corrisponds to an index choices
    int index = -1;
    for (int i = 0; i < count(); i++) {
        if (itemText(i) == text) {
            index = i;
        }
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;

    disconnect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &ComboEdit::slotValueChanged);
    const bool isCustomValue = index == -1;
    if (isCustomValue) {
        setEditText(text);
    } else {
        setCurrentIndex(index);
    }
    lineEdit()->setCursorPosition(anchorPos);
    lineEdit()->cursorForward(true, cursorPos - anchorPos);
    connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &ComboEdit::slotValueChanged);
    setFocus();
}

void ComboEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = lineEdit()->createStandardContextMenu();

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, DeleteAct, SelectAllAct };

    QAction *kundo = KStandardAction::create(KStandardAction::Undo, m_controller, SIGNAL(requestUndo()), menu);
    QAction *kredo = KStandardAction::create(KStandardAction::Redo, m_controller, SIGNAL(requestRedo()), menu);
    connect(m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled);
    connect(m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(m_controller->canUndo());
    kredo->setEnabled(m_controller->canRedo());

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction(oldUndo, kundo);
    menu->insertAction(oldRedo, kredo);

    menu->removeAction(oldUndo);
    menu->removeAction(oldRedo);

    menu->exec(event->globalPos());
    delete menu;
}

bool ComboEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
            m_controller->document()->processKVCFActions(m_ff);
            return true;
        }
        if (keyEvent == QKeySequence::Undo) {
            Q_EMIT m_controller->requestUndo();
            return true;
        } else if (keyEvent == QKeySequence::Redo) {
            Q_EMIT m_controller->requestRedo();
            return true;
        }
    } else if (e->type() == QEvent::FocusIn) {
        const auto ffc = static_cast<Okular::FormFieldChoice *>(m_ff);

        QString prevText;
        if (ffc->currentChoices().isEmpty()) {
            prevText = ffc->editChoice();
        } else {
            prevText = ffc->choices().at(ffc->currentChoices().constFirst());
        }
        if (lineEdit()->text() != prevText) {
            lineEdit()->setText(prevText);
        }
        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(e);
        if (focusEvent->reason() != Qt::ActiveWindowFocusReason) {
            if (const Okular::Action *action = m_ff->additionalAction(Okular::Annotation::FocusIn)) {
                m_controller->document()->processFocusAction(action, m_ff);
            }
        }
        setFocus();
        m_editing = true;
    } else if (e->type() == QEvent::FocusOut) {
        m_editing = false;

        // Don't worry about focus events from other sources than the user FocusEvent to edit the field
        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(e);
        if (focusEvent->reason() == Qt::OtherFocusReason || focusEvent->reason() == Qt::ActiveWindowFocusReason) {
            return true;
        }

        m_controller->document()->processKVCFActions(m_ff);

        if (const Okular::Action *action = m_ff->additionalAction(Okular::Annotation::FocusOut)) {
            m_controller->document()->processFocusAction(action, m_ff);
        }
    }
    return QComboBox::event(e);
}

void ComboEdit::slotRefresh(Okular::FormField *form)
{
    if (form != m_ff) {
        return;
    }
    FormWidgetIface::slotRefresh(form);

    disconnect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ComboEdit::slotValueChanged);
    disconnect(this, &QComboBox::editTextChanged, this, &ComboEdit::slotValueChanged);
    Okular::FormFieldChoice *ffc = static_cast<Okular::FormFieldChoice *>(m_ff);
    const QList<int> selectedItems = ffc->currentChoices();
    if (selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count()) {
        setCurrentIndex(selectedItems.at(0));
    }

    if (ffc->isEditable() && !ffc->editChoice().isEmpty()) {
        lineEdit()->setText(ffc->editChoice());
    }
    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ComboEdit::slotValueChanged);
    connect(this, &QComboBox::editTextChanged, this, &ComboEdit::slotValueChanged);
}

SignatureEdit::SignatureEdit(Okular::FormFieldSignature *signature, PageView *pageView)
    : QAbstractButton(pageView->viewport())
    , FormWidgetIface(this, signature)
    , m_widgetPressed(false)
    , m_dummyMode(false)
    , m_wasVisible(false)
{
    setCursor(Qt::PointingHandCursor);
    if (signature->signatureType() == Okular::FormFieldSignature::UnsignedSignature) {
        setToolTip(i18nc("Digital signature", "Signature placeholder (Click to Sign)"));
        connect(this, &SignatureEdit::clicked, this, &SignatureEdit::signUnsignedSignature);
    } else {
        connect(this, &SignatureEdit::clicked, this, &SignatureEdit::slotViewProperties);
    }
}

void SignatureEdit::setDummyMode(bool set)
{
    m_dummyMode = set;
    if (m_dummyMode) {
        m_wasVisible = isVisible();
        // if widget was hidden then show it.
        // even if it wasn't hidden calling this will still update the background.
        setVisibility(true);
    } else {
        // forms were not visible before this call so hide this widget.
        if (!m_wasVisible) {
            setVisibility(false);
        } else {
            // forms were visible even before this call so only update the background color.
            update();
        }
    }
}

bool SignatureEdit::event(QEvent *e)
{
    if (m_dummyMode && e->type() != QEvent::Paint) {
        e->accept();
        return true;
    }

    switch (e->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        if (ev->button() == Qt::LeftButton) {
            m_widgetPressed = true;
            update();
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        if (ev->button() == Qt::LeftButton) {
            m_widgetPressed = false;
            update();
        }
        break;
    }
    case QEvent::Leave: {
        m_widgetPressed = false;
        update();
    }
    default:
        break;
    }

    return QAbstractButton::event(e);
}

void SignatureEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);
    Okular::FormFieldSignature *formSignature = static_cast<Okular::FormFieldSignature *>(formField());
    if (formSignature->signatureType() == Okular::FormFieldSignature::UnsignedSignature) {
        QAction *signAction = new QAction(i18n("&Sign…"), menu);
        connect(signAction, &QAction::triggered, this, &SignatureEdit::signUnsignedSignature);
        menu->addAction(signAction);
    } else {
        QAction *signatureProperties = new QAction(i18n("Signature Properties"), menu);
        connect(signatureProperties, &QAction::triggered, this, &SignatureEdit::slotViewProperties);
        menu->addAction(signatureProperties);
    }
    menu->exec(event->globalPos());
    delete menu;
}

void SignatureEdit::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // no borders when user hasn't allowed the forms to be shown
    if (m_dummyMode && !m_wasVisible) {
        painter.setPen(Qt::transparent);
    } else {
        painter.setPen(Qt::black);
    }

    if (m_widgetPressed || m_dummyMode) {
        QColor col = palette().color(QPalette::Active, QPalette::Highlight);
        col.setAlpha(50);
        painter.setBrush(col);
    } else {
        painter.setBrush(Qt::transparent);
    }
    painter.drawRect(0, 0, width() - 2, height() - 2);
}

void SignatureEdit::slotViewProperties()
{
    if (m_dummyMode) {
        return;
    }

    Okular::FormFieldSignature *formSignature = static_cast<Okular::FormFieldSignature *>(formField());
    SignaturePropertiesDialog propDlg(m_controller->m_doc, formSignature, this);
    propDlg.exec();
}

void SignatureEdit::signUnsignedSignature()
{
    if (m_dummyMode) {
        return;
    }

    const Okular::FormFieldSignature *formSignature = static_cast<Okular::FormFieldSignature *>(formField());
    PageView *pageView = static_cast<PageView *>(parent()->parent());
    SignaturePartUtils::signUnsignedSignature(formSignature, pageView, pageView->document());
}

// Code for additional action handling.
// Challenge: Change preprocessor magic to C++ magic!
//
// The mouseRelease event is special because the PDF spec
// says that the activation action takes precedence over this.
// So the mouse release action is only signaled if no activation
// action exists.
//
// For checkboxes the activation action is not triggered as
// they are still triggered from the clicked signal and additionally
// when the checked state changes.

#define DEFINE_ADDITIONAL_ACTIONS(FormClass, BaseClass)                                                                                                                                                                                        \
    void FormClass::mousePressEvent(QMouseEvent *event)                                                                                                                                                                                        \
    {                                                                                                                                                                                                                                          \
        Okular::Action *act = m_ff->additionalAction(Okular::Annotation::MousePressed);                                                                                                                                                        \
        if (act) {                                                                                                                                                                                                                             \
            m_controller->signalMouseAction(act, m_ff, Okular::Document::FieldMouseDown);                                                                                                                                                      \
        }                                                                                                                                                                                                                                      \
        BaseClass::mousePressEvent(event);                                                                                                                                                                                                     \
    }                                                                                                                                                                                                                                          \
    void FormClass::mouseReleaseEvent(QMouseEvent *event)                                                                                                                                                                                      \
    {                                                                                                                                                                                                                                          \
        if (!QWidget::rect().contains(event->position().toPoint())) {                                                                                                                                                                          \
            BaseClass::mouseReleaseEvent(event);                                                                                                                                                                                               \
            return;                                                                                                                                                                                                                            \
        }                                                                                                                                                                                                                                      \
        Okular::Action *act = m_ff->activationAction();                                                                                                                                                                                        \
        if (act && !qobject_cast<CheckBoxEdit *>(this)) {                                                                                                                                                                                      \
            m_controller->signalMouseAction(act, m_ff, Okular::Document::FieldMouseUp);                                                                                                                                                        \
        } else if ((act = m_ff->additionalAction(Okular::Annotation::MouseReleased))) {                                                                                                                                                        \
            m_controller->signalMouseAction(act, m_ff, Okular::Document::FieldMouseUp);                                                                                                                                                        \
        }                                                                                                                                                                                                                                      \
        BaseClass::mouseReleaseEvent(event);                                                                                                                                                                                                   \
    }                                                                                                                                                                                                                                          \
    void FormClass::focusInEvent(QFocusEvent *event)                                                                                                                                                                                           \
    {                                                                                                                                                                                                                                          \
        Okular::Action *act = m_ff->additionalAction(Okular::Annotation::FocusIn);                                                                                                                                                             \
        if (act && event->reason() != Qt::ActiveWindowFocusReason) {                                                                                                                                                                           \
            m_controller->processScriptAction(act, m_ff, Okular::Annotation::FocusIn);                                                                                                                                                         \
        }                                                                                                                                                                                                                                      \
        BaseClass::focusInEvent(event);                                                                                                                                                                                                        \
    }                                                                                                                                                                                                                                          \
    void FormClass::focusOutEvent(QFocusEvent *event)                                                                                                                                                                                          \
    {                                                                                                                                                                                                                                          \
        Okular::Action *act = m_ff->additionalAction(Okular::Annotation::FocusOut);                                                                                                                                                            \
        if (act) {                                                                                                                                                                                                                             \
            m_controller->processScriptAction(act, m_ff, Okular::Annotation::FocusOut);                                                                                                                                                        \
        }                                                                                                                                                                                                                                      \
        BaseClass::focusOutEvent(event);                                                                                                                                                                                                       \
    }                                                                                                                                                                                                                                          \
    void FormClass::leaveEvent(QEvent *event)                                                                                                                                                                                                  \
    {                                                                                                                                                                                                                                          \
        Okular::Action *act = m_ff->additionalAction(Okular::Annotation::CursorLeaving);                                                                                                                                                       \
        if (act) {                                                                                                                                                                                                                             \
            m_controller->signalMouseAction(act, m_ff, Okular::Document::FieldMouseExit);                                                                                                                                                      \
        }                                                                                                                                                                                                                                      \
        BaseClass::leaveEvent(event);                                                                                                                                                                                                          \
    }                                                                                                                                                                                                                                          \
    void FormClass::enterEvent(QEnterEvent *event)                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                          \
        Okular::Action *act = m_ff->additionalAction(Okular::Annotation::CursorEntering);                                                                                                                                                      \
        if (act) {                                                                                                                                                                                                                             \
            m_controller->signalMouseAction(act, m_ff, Okular::Document::FieldMouseEnter);                                                                                                                                                     \
        }                                                                                                                                                                                                                                      \
        BaseClass::enterEvent(event);                                                                                                                                                                                                          \
    }

DEFINE_ADDITIONAL_ACTIONS(PushButtonEdit, QPushButton)
DEFINE_ADDITIONAL_ACTIONS(CheckBoxEdit, QCheckBox)
DEFINE_ADDITIONAL_ACTIONS(RadioButtonEdit, QRadioButton)
DEFINE_ADDITIONAL_ACTIONS(FormLineEdit, QLineEdit)
DEFINE_ADDITIONAL_ACTIONS(TextAreaEdit, KTextEdit)
DEFINE_ADDITIONAL_ACTIONS(FileEdit, KUrlRequester)
DEFINE_ADDITIONAL_ACTIONS(ListEdit, QListWidget)
DEFINE_ADDITIONAL_ACTIONS(ComboEdit, QComboBox)
DEFINE_ADDITIONAL_ACTIONS(SignatureEdit, QAbstractButton)

#undef DEFINE_ADDITIONAL_ACTIONS
