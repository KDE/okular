/*
    SPDX-FileCopyrightText: 2006 Chu Xiaodong <xiaodongchu@gmail.com>
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "annotwindow.h"

// qt/kde includes
#include <KLocalizedString>
#include <KStandardAction>
#include <KTextEdit>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPushButton>
#include <QSizeGrip>
#include <QStyle>
#include <QToolButton>

// local includes
#include "core/annotations.h"
#include "core/document.h"
#include "latexrenderer.h"
#include <KMessageBox>
#include <core/utils.h>

class CloseButton : public QPushButton
{
    Q_OBJECT

public:
    CloseButton(QWidget *parent = Q_NULLPTR)
        : QPushButton(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QSize size = QSize(14, 14).expandedTo(QApplication::globalStrut());
        setFixedSize(size);
        setIcon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
        setIconSize(size);
        setToolTip(i18n("Close this note"));
        setCursor(Qt::ArrowCursor);
    }
};

class MovableTitle : public QWidget
{
    Q_OBJECT

public:
    MovableTitle(AnnotWindow *parent)
        : QWidget(parent)
    {
        QVBoxLayout *mainlay = new QVBoxLayout(this);
        mainlay->setContentsMargins(0, 0, 0, 0);
        mainlay->setSpacing(0);
        // close button row
        QHBoxLayout *buttonlay = new QHBoxLayout();
        mainlay->addLayout(buttonlay);
        titleLabel = new QLabel(this);
        QFont f = titleLabel->font();
        f.setBold(true);
        titleLabel->setFont(f);
        titleLabel->setCursor(Qt::SizeAllCursor);
        buttonlay->addWidget(titleLabel);
        dateLabel = new QLabel(this);
        dateLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);
        f = dateLabel->font();
        f.setPointSize(QFontInfo(f).pointSize() - 2);
        dateLabel->setFont(f);
        dateLabel->setCursor(Qt::SizeAllCursor);
        buttonlay->addWidget(dateLabel);
        CloseButton *close = new CloseButton(this);
        connect(close, &QAbstractButton::clicked, parent, &QWidget::close);
        buttonlay->addWidget(close);
        // option button row
        QHBoxLayout *optionlay = new QHBoxLayout();
        mainlay->addLayout(optionlay);
        authorLabel = new QLabel(this);
        authorLabel->setCursor(Qt::SizeAllCursor);
        authorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        optionlay->addWidget(authorLabel);
        optionButton = new QToolButton(this);
        QString opttext = i18n("Options");
        optionButton->setText(opttext);
        optionButton->setAutoRaise(true);
        QSize s = QFontMetrics(optionButton->font()).boundingRect(opttext).size() + QSize(8, 8);
        optionButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        optionButton->setFixedSize(s);
        optionlay->addWidget(optionButton);
        // ### disabled for now
        optionButton->hide();
        latexButton = new QToolButton(this);
        QHBoxLayout *latexlay = new QHBoxLayout();
        QString latextext = i18n("This annotation may contain LaTeX code.\nClick here to render.");
        latexButton->setText(latextext);
        latexButton->setAutoRaise(true);
        s = QFontMetrics(latexButton->font()).boundingRect(0, 0, this->width(), this->height(), 0, latextext).size() + QSize(8, 8);
        latexButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        latexButton->setFixedSize(s);
        latexButton->setCheckable(true);
        latexButton->setVisible(false);
        latexlay->addSpacing(1);
        latexlay->addWidget(latexButton);
        latexlay->addSpacing(1);
        mainlay->addLayout(latexlay);
        connect(latexButton, &QToolButton::clicked, parent, &AnnotWindow::renderLatex);
        connect(parent, &AnnotWindow::containsLatex, latexButton, &QWidget::setVisible);

        titleLabel->installEventFilter(this);
        dateLabel->installEventFilter(this);
        authorLabel->installEventFilter(this);
    }

    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (obj != titleLabel && obj != authorLabel && obj != dateLabel)
            return false;

        QMouseEvent *me = nullptr;
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            me = (QMouseEvent *)e;
            mousePressPos = me->pos();
            parentWidget()->raise();
            break;
        case QEvent::MouseButtonRelease:
            mousePressPos = QPoint();
            break;
        case QEvent::MouseMove:
            me = (QMouseEvent *)e;
            parentWidget()->move(me->pos() - mousePressPos + parentWidget()->pos());
            break;
        default:
            return false;
        }
        return true;
    }

    void setTitle(const QString &title)
    {
        titleLabel->setText(QStringLiteral(" ") + title);
    }

    void setDate(const QDateTime &dt)
    {
        dateLabel->setText(QLocale().toString(dt.toTimeSpec(Qt::LocalTime), QLocale::ShortFormat) + QLatin1Char(' '));
    }

    void setAuthor(const QString &author)
    {
        authorLabel->setText(QStringLiteral(" ") + author);
    }

    void connectOptionButton(QObject *recv, const char *method)
    {
        connect(optionButton, SIGNAL(clicked()), recv, method);
    }

    void uncheckLatexButton()
    {
        latexButton->setChecked(false);
    }

private:
    QLabel *titleLabel;
    QLabel *dateLabel;
    QLabel *authorLabel;
    QPoint mousePressPos;
    QToolButton *optionButton;
    QToolButton *latexButton;
};

// Qt::SubWindow is needed to make QSizeGrip work
AnnotWindow::AnnotWindow(QWidget *parent, Okular::Annotation *annot, Okular::Document *document, int page)
    : QFrame(parent, Qt::SubWindow)
    , m_annot(annot)
    , m_document(document)
    , m_page(page)
{
    setAutoFillBackground(true);
    setFrameStyle(Panel | Raised);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("AnnotWindow"));

    const bool canEditAnnotation = m_document->canModifyPageAnnotation(annot);

    textEdit = new KTextEdit(this);
    textEdit->setAcceptRichText(false);
    textEdit->setPlainText(m_annot->contents());
    textEdit->installEventFilter(this);
    textEdit->setUndoRedoEnabled(false);

    m_prevCursorPos = textEdit->textCursor().position();
    m_prevAnchorPos = textEdit->textCursor().anchor();

    connect(textEdit, &KTextEdit::textChanged, this, &AnnotWindow::slotsaveWindowText);
    connect(textEdit, &KTextEdit::cursorPositionChanged, this, &AnnotWindow::slotsaveWindowText);
    connect(textEdit, &KTextEdit::aboutToShowContextMenu, this, &AnnotWindow::slotUpdateUndoAndRedoInContextMenu);
    connect(m_document, &Okular::Document::annotationContentsChangedByUndoRedo, this, &AnnotWindow::slotHandleContentsChangedByUndoRedo);

    if (!canEditAnnotation)
        textEdit->setReadOnly(true);

    QVBoxLayout *mainlay = new QVBoxLayout(this);
    mainlay->setContentsMargins(2, 2, 2, 2);
    mainlay->setSpacing(0);
    m_title = new MovableTitle(this);
    mainlay->addWidget(m_title);
    mainlay->addWidget(textEdit);
    QHBoxLayout *lowerlay = new QHBoxLayout();
    mainlay->addLayout(lowerlay);
    lowerlay->addItem(new QSpacerItem(5, 5, QSizePolicy::Expanding, QSizePolicy::Fixed));
    QSizeGrip *sb = new QSizeGrip(this);
    lowerlay->addWidget(sb);

    m_latexRenderer = new GuiUtils::LatexRenderer();
    // The emit below is not wrong even if emitting signals from the constructor it's usually wrong
    // in this case the signal it's connected to inside MovableTitle constructor a few lines above
    emit containsLatex(GuiUtils::LatexRenderer::mightContainLatex(m_annot->contents())); // clazy:exclude=incorrect-emit

    m_title->setTitle(m_annot->window().summary());
    m_title->connectOptionButton(this, SLOT(slotOptionBtn()));

    setGeometry(10, 10, 300, 300);

    reloadInfo();
}

AnnotWindow::~AnnotWindow()
{
    delete m_latexRenderer;
}

Okular::Annotation *AnnotWindow::annotation() const
{
    return m_annot;
}

void AnnotWindow::updateAnnotation(Okular::Annotation *a)
{
    m_annot = a;
}

void AnnotWindow::reloadInfo()
{
    QColor newcolor;
    if (m_annot->subType() == Okular::Annotation::AText) {
        Okular::TextAnnotation *textAnn = static_cast<Okular::TextAnnotation *>(m_annot);
        if (textAnn->textType() == Okular::TextAnnotation::InPlace && textAnn->inplaceIntent() == Okular::TextAnnotation::TypeWriter)
            newcolor = QColor(0xfd, 0xfd, 0x96);
    }
    if (!newcolor.isValid())
        newcolor = m_annot->style().color().isValid() ? QColor(m_annot->style().color().red(), m_annot->style().color().green(), m_annot->style().color().blue(), 255) : Qt::yellow;
    if (newcolor != m_color) {
        m_color = newcolor;
        setPalette(QPalette(m_color));
        QPalette pl = textEdit->palette();
        pl.setColor(QPalette::Base, m_color);
        textEdit->setPalette(pl);
    }
    m_title->setAuthor(m_annot->author());
    m_title->setDate(m_annot->modificationDate());
}

int AnnotWindow::pageNumber() const
{
    return m_page;
}

void AnnotWindow::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);

    // focus the content area by default
    textEdit->setFocus();
}

bool AnnotWindow::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_Escape) {
            e->accept();
            return true;
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        if (keyEvent == QKeySequence::Undo) {
            m_document->undo();
            return true;
        } else if (keyEvent == QKeySequence::Redo) {
            m_document->redo();
            return true;
        } else if (keyEvent->key() == Qt::Key_Escape) {
            close();
            return true;
        }
    } else if (e->type() == QEvent::FocusIn) {
        raise();
    }
    return QFrame::eventFilter(o, e);
}

void AnnotWindow::slotUpdateUndoAndRedoInContextMenu(QMenu *menu)
{
    if (!menu)
        return;

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, ClearAct, SelectAllAct, NCountActs };

    QAction *kundo = KStandardAction::create(KStandardAction::Undo, m_document, SLOT(undo()), menu);
    QAction *kredo = KStandardAction::create(KStandardAction::Redo, m_document, SLOT(redo()), menu);
    connect(m_document, &Okular::Document::canUndoChanged, kundo, &QAction::setEnabled);
    connect(m_document, &Okular::Document::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(m_document->canUndo());
    kredo->setEnabled(m_document->canRedo());

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction(oldUndo, kundo);
    menu->insertAction(oldRedo, kredo);

    menu->removeAction(oldUndo);
    menu->removeAction(oldRedo);
}

void AnnotWindow::slotOptionBtn()
{
    // TODO: call context menu in pageview
    // emit sig...
}

void AnnotWindow::slotsaveWindowText()
{
    const QString contents = textEdit->toPlainText();
    const int cursorPos = textEdit->textCursor().position();
    if (contents != m_annot->contents()) {
        m_document->editPageAnnotationContents(m_page, m_annot, contents, cursorPos, m_prevCursorPos, m_prevAnchorPos);
        emit containsLatex(GuiUtils::LatexRenderer::mightContainLatex(textEdit->toPlainText()));
    }
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = textEdit->textCursor().anchor();
}

void AnnotWindow::renderLatex(bool render)
{
    if (render) {
        textEdit->setReadOnly(true);
        disconnect(textEdit, &KTextEdit::textChanged, this, &AnnotWindow::slotsaveWindowText);
        disconnect(textEdit, &KTextEdit::cursorPositionChanged, this, &AnnotWindow::slotsaveWindowText);
        textEdit->setAcceptRichText(true);
        QString contents = m_annot->contents();
        contents = Qt::convertFromPlainText(contents);
        QColor fontColor = textEdit->textColor();
        int fontSize = textEdit->fontPointSize();
        QString latexOutput;
        GuiUtils::LatexRenderer::Error errorCode = m_latexRenderer->renderLatexInHtml(contents, fontColor, fontSize, Okular::Utils::realDpi(nullptr).width(), latexOutput);
        switch (errorCode) {
        case GuiUtils::LatexRenderer::LatexNotFound:
            KMessageBox::sorry(this, i18n("Cannot find latex executable."), i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::DvipngNotFound:
            KMessageBox::sorry(this, i18n("Cannot find dvipng executable."), i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::LatexFailed:
            KMessageBox::detailedSorry(this, i18n("A problem occurred during the execution of the 'latex' command."), latexOutput, i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::DvipngFailed:
            KMessageBox::sorry(this, i18n("A problem occurred during the execution of the 'dvipng' command."), i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::NoError:
        default:
            textEdit->setHtml(contents);
            break;
        }
    } else {
        textEdit->setAcceptRichText(false);
        textEdit->setPlainText(m_annot->contents());
        connect(textEdit, &KTextEdit::textChanged, this, &AnnotWindow::slotsaveWindowText);
        connect(textEdit, &KTextEdit::cursorPositionChanged, this, &AnnotWindow::slotsaveWindowText);
        textEdit->setReadOnly(false);
    }
}

void AnnotWindow::slotHandleContentsChangedByUndoRedo(Okular::Annotation *annot, const QString &contents, int cursorPos, int anchorPos)
{
    if (annot != m_annot) {
        return;
    }

    textEdit->setPlainText(contents);
    QTextCursor c = textEdit->textCursor();
    c.setPosition(anchorPos);
    c.setPosition(cursorPos, QTextCursor::KeepAnchor);
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    textEdit->setTextCursor(c);
    textEdit->setFocus();
    emit containsLatex(GuiUtils::LatexRenderer::mightContainLatex(m_annot->contents()));
}

#include "annotwindow.moc"
