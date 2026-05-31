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
#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizeGrip>
#include <QStyle>
#include <QTimer>
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
    explicit CloseButton(QWidget *parent = Q_NULLPTR)
        : QPushButton(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QSize size = QSize(14, 14);
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
    explicit MovableTitle(AnnotWindow *parent, int numberPreviousAnnotation = 0)
        : QWidget(parent)
    {
        countPreviousAnnotation = numberPreviousAnnotation;

        // First zone: titleLabel + buttonClose
        QVBoxLayout *mainlay = new QVBoxLayout(this);
        mainlay->setContentsMargins(0, 0, 0, 0);
        mainlay->setSpacing(0);
        // close button row
        QHBoxLayout *buttonlay = new QHBoxLayout();
        titleLabel = new QLabel(this);
        QFont f = titleLabel->font();
        f.setBold(true);
        titleLabel->setFont(f);
        titleLabel->setCursor(Qt::SizeAllCursor);
        buttonlay->addWidget(titleLabel);
        dateLabel = new QLabel(this);
        buttonlay->addWidget(dateLabel);
        CloseButton *close = new CloseButton(this);
        connect(close, &QAbstractButton::clicked, parent, &QWidget::close);
        buttonlay->addWidget(close);

        QScrollArea *scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameStyle(QFrame::NoFrame);
        scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        QWidget *scrollContainer = new QWidget(scrollArea);
        QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContainer);
        scrollLayout->setContentsMargins(0, 0, 0, 0);

        for (qsizetype i = 0; i < numberPreviousAnnotation; i++) {
            // For authorLabel
            QLabel *newAuthorLabel = new QLabel(this);
            newAuthorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            listPreviousAuthorAndDateLabel.append(newAuthorLabel);
            scrollLayout->addWidget(newAuthorLabel);

            // For contents
            QLabel *newContentsLabel = new QLabel(this);
            newContentsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            listPreviousContentLabel.append(newContentsLabel);
            scrollLayout->addWidget(newContentsLabel);

            // Add a horizontal black line after each comment
            QFrame *line = new QFrame(scrollContainer);
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setStyleSheet(QStringLiteral("color: rgba(0,0,0,0.1);"));
            scrollLayout->addWidget(line);
        }

        scrollArea->setWidget(scrollContainer);

        // Third zone: authorAndDateLabel (author name with date)
        authorAndDateLabel = new QLabel(this);
        authorAndDateLabel->setContentsMargins(0, 10, 0, 0);
        authorAndDateLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: black"));
        authorAndDateLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        authorAndDateLabel->setCursor(Qt::SizeAllCursor);

        // Last zone: Useful properties for editable text zone
        QFrame *bottomFrame = new QFrame(this);
        QVBoxLayout *bottomLayout = new QVBoxLayout(bottomFrame);
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        bottomLayout->setSpacing(0);

        QHBoxLayout *optionlay = new QHBoxLayout();
        optionlay->addWidget(authorAndDateLabel);
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
        connect(latexButton, &QToolButton::clicked, parent, &AnnotWindow::renderLatex);
        connect(parent, &AnnotWindow::containsLatex, latexButton, &QWidget::setVisible);

        bottomLayout->addLayout(optionlay);

        // Adds all widgets and layouts with mainlay
        // First zone
        mainlay->addLayout(buttonlay);
        // Second zone
        if (numberPreviousAnnotation > 0) {
            mainlay->addWidget(scrollArea);
        }
        // Third zone
        mainlay->addWidget(authorAndDateLabel);
        // Last zone
        mainlay->addWidget(bottomFrame);
        mainlay->addLayout(latexlay);

        setScrollArea(scrollArea);

        // Event filters
        titleLabel->installEventFilter(this);
        authorAndDateLabel->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched != titleLabel && watched != authorAndDateLabel) {
            return false;
        }

        QMouseEvent *me = nullptr;
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            me = static_cast<QMouseEvent *>(event);
            mousePressPos = me->pos();
            parentWidget()->raise();
            break;
        case QEvent::MouseButtonRelease:
            mousePressPos = QPoint();
            break;
        case QEvent::MouseMove: {
            me = static_cast<QMouseEvent *>(event);
            const QPoint mouseMovePos = me->pos();
            const QPoint mouseDelta = mouseMovePos - mousePressPos;

            const auto annotWidget = parentWidget();
            QPoint newPositionPoint = annotWidget->pos() + mouseDelta;
            annotWidget->move(newPositionPoint);
            break;
        }
        default:
            return false;
        }
        return true;
    }

    int numberPreviousAnnotation()
    {
        return countPreviousAnnotation;
    }

    void setTitle(const QString &title)
    {
        titleLabel->setText(QStringLiteral(" ") + title);
    }

    void setAuthorAndDate(const QString &author, const QDateTime &dt)
    {
        QString auth;
        (author.isNull() || author.isEmpty()) ? auth = i18n("Unknown author") : auth = author;

        QString dateTime;
        dt.isValid() ? dateTime = QLocale().toString(dt.toTimeSpec(Qt::LocalTime), QLocale::ShortFormat) : dateTime = i18n("No date recognized");

        authorAndDateLabel->setText(QStringLiteral(" %1 (%2)").arg(auth, dateTime));
    }

    void connectOptionButton(QObject *recv, const char *method)
    {
        connect(optionButton, SIGNAL(clicked()), recv, method);
    }

    void uncheckLatexButton()
    {
        latexButton->setChecked(false);
    }

    void setAuthorAndDateAtList(int index, const QString &author, const QDateTime &dt)
    {
        if (index < listPreviousAuthorAndDateLabel.size()) {
            QString auth;
            (author.isNull() || author.isEmpty()) ? auth = i18n("Unknown author") : auth = author;

            QString dateTime;
            dt.isValid() ? dateTime = QLocale().toString(dt.toTimeSpec(Qt::LocalTime), QLocale::ShortFormat) : dateTime = i18n("No date recognized");

            listPreviousAuthorAndDateLabel[index]->setText(QStringLiteral(" > %1 (%2)").arg(auth, dateTime));
        }
    }

    void setContentAtList(int index, const QString &content)
    {
        if (index < listPreviousContentLabel.size()) {
            listPreviousContentLabel[index]->setText(content);
        }
    }

    void setScrollArea(QScrollArea *scrollArea)
    {
        scrollAr = scrollArea;
    }

    QScrollArea *scrollArea()
    {
        return scrollAr;
    }

private:
    QLabel *titleLabel;
    int countPreviousAnnotation = 0;
    QList<QLabel *> listPreviousAuthorAndDateLabel;
    QList<QLabel *> listPreviousContentLabel;
    QLabel *dateLabel;
    QLabel *authorAndDateLabel;
    QPoint mousePressPos;
    QToolButton *optionButton;
    QToolButton *latexButton;
    QScrollArea *scrollAr = nullptr;
};

// Qt::SubWindow is needed to make QSizeGrip work
AnnotWindow::AnnotWindow(QWidget *parent, QRect initialViewportBounds, Okular::Annotation *annot, Okular::Document *document, int page)
    : QFrame(parent, Qt::SubWindow)
    , m_viewportBounds(initialViewportBounds)
    , m_annot(annot)
    , m_document(document)
    , m_page(page)
{
    setAutoFillBackground(true);
    setFrameStyle(Panel | Raised);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("AnnotWindow"));

    const bool canEditAnnotation = m_document->canModifyPageAnnotation(annot);

    int countNumberPreviousAnnotation = 0;

    Okular::Annotation *currentAnnot = annot;
    while (currentAnnot) {
        Okular::Annotation *child = currentAnnot->previousAnnotation();
        if (child) {
            countNumberPreviousAnnotation++;
            currentAnnot = child;
        } else {
            break;
        }
    }

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

    if (!canEditAnnotation) {
        textEdit->setReadOnly(true);
    }

    QVBoxLayout *mainlay = new QVBoxLayout(this);
    mainlay->setContentsMargins(2, 2, 2, 2);
    mainlay->setSpacing(0);
    m_title = new MovableTitle(this, countNumberPreviousAnnotation);
    mainlay->addWidget(m_title);
    mainlay->addWidget(textEdit);
    QHBoxLayout *lowerlay = new QHBoxLayout();
    mainlay->addLayout(lowerlay);
    lowerlay->addItem(new QSpacerItem(5, 5, QSizePolicy::Expanding, QSizePolicy::Fixed));
    QSizeGrip *sb = new QSizeGrip(this);
    lowerlay->addWidget(sb);

    m_latexRenderer = new GuiUtils::LatexRenderer();
    // The Q_EMIT below is not wrong even if emitting signals from the constructor it's usually wrong
    // in this case the signal it's connected to inside MovableTitle constructor a few lines above
    Q_EMIT containsLatex(GuiUtils::LatexRenderer::mightContainLatex(m_annot->contents())); // clazy:exclude=incorrect-emit

    m_title->setTitle(m_annot->window().summary());
    m_title->connectOptionButton(this, SLOT(slotOptionBtn()));

    const auto initialPosition = qApp->isRightToLeft() //
        ? initialViewportBounds.topRight() + QPoint(-defaultSize.width() - defaultPosition.x(), defaultPosition.y())
        : initialViewportBounds.topLeft() + defaultPosition;
    setGeometry(QRect(initialPosition, defaultSize));

    reloadInfo();

    QScrollArea *scrollArea = m_title->scrollArea();
    if (scrollArea) {
        // Adapt the size of the scrollArea between 5 and 125px that depends of the content height size.
        if (scrollArea->widget() && scrollArea->widget()->layout()) {
            int contentHeight = scrollArea->widget()->layout()->sizeHint().height();
            int finalHeight = qBound(5, contentHeight + 5, 135);
            scrollArea->setFixedHeight(finalHeight);
        }

        // Put the scrollbar at the lowest position (to show firstly the most recent comments)
        if (scrollArea->verticalScrollBar()) {
            QTimer::singleShot(20, this, [scrollArea]() { scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum()); });
        }
    }
}

AnnotWindow::~AnnotWindow()
{
    delete m_latexRenderer;
    delete textEdit;
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
        if (textAnn->textType() == Okular::TextAnnotation::InPlace && textAnn->inplaceIntent() == Okular::TextAnnotation::TypeWriter) {
            newcolor = QColor(0xfd, 0xfd, 0x96);
        }
    }
    if (!newcolor.isValid()) {
        newcolor = m_annot->style().color().isValid() ? QColor(m_annot->style().color().red(), m_annot->style().color().green(), m_annot->style().color().blue(), 255) : Qt::yellow;
    }
    if (newcolor != m_color) {
        m_color = newcolor;
        setPalette(QPalette(m_color));
        QPalette pl = textEdit->palette();
        pl.setColor(QPalette::Base, m_color);
        textEdit->setPalette(pl);
    }

    Okular::Annotation *parent = m_annot;
    Okular::Annotation *child = nullptr;
    int count = m_title->numberPreviousAnnotation() - 1;

    while (parent) {
        child = parent->previousAnnotation();
        if (child) {
            m_title->setAuthorAndDateAtList(count, child->author(), child->modificationDate());
            m_title->setContentAtList(count, child->contents());
            parent = child;
        } else {
            break;
        }

        count--;
    }

    m_title->setAuthorAndDate(m_annot->author(), m_annot->modificationDate());
}

int AnnotWindow::pageNumber() const
{
    return m_page;
}

void AnnotWindow::updateViewportBounds(QRect bounds)
{
    m_viewportBounds = bounds;
    fixupGeometry();
}

void AnnotWindow::fixupGeometry()
{
    // Try to maintain the default size, but squeeze if not does not fit.
    // Use viewport bounds to exclude scrollbars.
    const auto bounds = m_viewportBounds;

    const QSize size( //
        std::min(bounds.width(), defaultSize.width()),
        std::min(bounds.height(), defaultSize.height()));

    const QPoint position( //
        bounds.x() + std::max(0, std::min(bounds.width() - size.width(), x() - bounds.x())),
        bounds.y() + std::max(0, std::min(bounds.height() - size.height(), y() - bounds.y())));

    // hopefully no infinite event recursion, because we only need to fix up once, after which it should be idempotent
    setGeometry(QRect(position, size));
}

void AnnotWindow::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);

    // focus the content area by default
    textEdit->setFocus();
}

void AnnotWindow::moveEvent(QMoveEvent *event)
{
    QFrame::moveEvent(event);
    fixupGeometry();
}

void AnnotWindow::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    fixupGeometry();
}

bool AnnotWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == textEdit) {
        if (event->type() == QEvent::ShortcutOverride) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                event->accept();
                return true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
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
        } else if (event->type() == QEvent::FocusIn) {
            raise();
        }
    }
    return QFrame::eventFilter(watched, event);
}

void AnnotWindow::slotUpdateUndoAndRedoInContextMenu(QMenu *menu)
{
    if (!menu) {
        return;
    }

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, ClearAct, SelectAllAct, NCountActs };

    QAction *kundo = KStandardAction::create(
        KStandardAction::Undo,
        m_document,
        [doc = m_document] {
            // We need a QueuedConnection because undoing may end up destroying the menu this action is on
            // because it will undo the addition of the annotation. If it's not queued things gets unhappy
            // because the menu is destroyed in the middle of processing the click on the menu itself
            QMetaObject::invokeMethod(doc, &Okular::Document::undo, Qt::QueuedConnection);
        },
        menu);
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
    // Q_EMIT sig...
}

void AnnotWindow::slotsaveWindowText()
{
    const QString contents = textEdit->toPlainText();
    const int cursorPos = textEdit->textCursor().position();
    if (contents != m_annot->contents()) {
        m_document->editPageAnnotationContents(m_page, m_annot, contents, cursorPos, m_prevCursorPos, m_prevAnchorPos);
        Q_EMIT containsLatex(GuiUtils::LatexRenderer::mightContainLatex(textEdit->toPlainText()));
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
            KMessageBox::error(this, i18n("Cannot find latex executable."), i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::DvipngNotFound:
            KMessageBox::error(this, i18n("Cannot find dvipng executable."), i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::LatexFailed:
            KMessageBox::detailedError(this, i18n("A problem occurred during the execution of the 'latex' command."), latexOutput, i18n("LaTeX rendering failed"));
            m_title->uncheckLatexButton();
            renderLatex(false);
            break;
        case GuiUtils::LatexRenderer::DvipngFailed:
            KMessageBox::error(this, i18n("A problem occurred during the execution of the 'dvipng' command."), i18n("LaTeX rendering failed"));
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
    Q_EMIT containsLatex(GuiUtils::LatexRenderer::mightContainLatex(m_annot->contents()));
}

#include "annotwindow.moc"
#include "moc_annotwindow.cpp"
