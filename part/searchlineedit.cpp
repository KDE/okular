/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2007, 2009-2010 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "searchlineedit.h"

// local includes

// qt/kde includes
#include <KBusyIndicatorWidget>
#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageBox>
#include <QApplication>
#include <QLayout>
#include <QTimer>

SearchLineEdit::SearchLineEdit(QWidget *parent, Okular::Document *document)
    : KLineEdit(parent)
    , m_document(document)
    , m_minLength(0)
    , m_caseSensitivity(Qt::CaseInsensitive)
    , m_searchType(Okular::Document::AllDocument)
    , m_id(-1)
    , m_moveViewport(false)
    , m_changed(false)
    , m_fromStart(true)
    , m_findAsYouType(true)
    , m_searchRunning(false)
{
    setObjectName(QStringLiteral("SearchLineEdit"));
    setClearButtonEnabled(true);

    // a timer to ensure that we don't flood the document with requests to search
    m_inputDelayTimer = new QTimer(this);
    m_inputDelayTimer->setSingleShot(true);
    connect(m_inputDelayTimer, &QTimer::timeout, this, &SearchLineEdit::startSearch);

    connect(this, &SearchLineEdit::textChanged, this, &SearchLineEdit::slotTextChanged);
    connect(document, &Okular::Document::searchFinished, this, &SearchLineEdit::searchFinished);
}

void SearchLineEdit::clearText()
{
    clear();
}

void SearchLineEdit::setSearchCaseSensitivity(Qt::CaseSensitivity cs)
{
    m_caseSensitivity = cs;
    m_changed = true;
}

void SearchLineEdit::setSearchMinimumLength(int length)
{
    m_minLength = length;
    m_changed = true;
}

void SearchLineEdit::setSearchType(Okular::Document::SearchType type)
{
    if (type == m_searchType) {
        return;
    }

    disconnect(this, &SearchLineEdit::returnKeyPressed, this, &SearchLineEdit::slotReturnPressed);

    m_searchType = type;

    // Only connect Enter for next/prev searches, the rest of searches are document global so
    // next/prev search does not make sense for them
    if (m_searchType == Okular::Document::NextMatch || m_searchType == Okular::Document::PreviousMatch) {
        connect(this, &SearchLineEdit::returnKeyPressed, this, &SearchLineEdit::slotReturnPressed);
    }

    if (!m_changed) {
        m_changed = (m_searchType != Okular::Document::NextMatch && m_searchType != Okular::Document::PreviousMatch);
    }
}

void SearchLineEdit::setSearchId(int id)
{
    m_id = id;
    m_changed = true;
}

void SearchLineEdit::setSearchColor(const QColor &color)
{
    m_color = color;
    m_changed = true;
}

void SearchLineEdit::setSearchMoveViewport(bool move)
{
    m_moveViewport = move;
}

void SearchLineEdit::setSearchFromStart(bool fromStart)
{
    m_fromStart = fromStart;
}

void SearchLineEdit::setFindAsYouType(bool findAsYouType)
{
    m_findAsYouType = findAsYouType;
}

void SearchLineEdit::resetSearch()
{
    // Stop the currently running search, if any
    stopSearch();

    // Clear highlights
    if (m_id != -1) {
        m_document->resetSearch(m_id);
    }

    // Make sure that the search will be reset at the next one
    m_changed = true;

    // Reset input box color
    prepareLineEditForSearch();
}

bool SearchLineEdit::isSearchRunning() const
{
    return m_searchRunning;
}

void SearchLineEdit::restartSearch()
{
    m_inputDelayTimer->stop();
    m_inputDelayTimer->start(700);
    m_changed = true;
}

void SearchLineEdit::stopSearch()
{
    if (m_id == -1 || !m_searchRunning) {
        return;
    }

    m_inputDelayTimer->stop();
    // ### this should just cancel the search with id m_id, not all of them
    m_document->cancelSearch();
    // flagging as "changed" so the search will be reset at the next one
    m_changed = true;
}

void SearchLineEdit::findNext()
{
    if (m_id == -1 || m_searchType != Okular::Document::NextMatch) {
        return;
    }

    if (!m_changed) {
        Q_EMIT searchStarted();
        m_searchRunning = true;
        m_document->continueSearch(m_id, m_searchType);
    } else {
        startSearch();
    }
}

void SearchLineEdit::findPrev()
{
    if (m_id == -1 || m_searchType != Okular::Document::PreviousMatch) {
        return;
    }

    if (!m_changed) {
        Q_EMIT searchStarted();
        m_searchRunning = true;
        m_document->continueSearch(m_id, m_searchType);
    } else {
        startSearch();
    }
}

void SearchLineEdit::slotTextChanged(const QString &text)
{
    Q_UNUSED(text);

    prepareLineEditForSearch();

    if (m_findAsYouType) {
        restartSearch();
    } else {
        m_changed = true;
    }
}

void SearchLineEdit::prepareLineEditForSearch()
{
    QPalette pal = palette();
    const int textLength = text().length();
    if (textLength > 0 && textLength < m_minLength) {
        const KColorScheme scheme(QPalette::Active, KColorScheme::View);
        pal.setBrush(QPalette::Base, scheme.background(KColorScheme::NegativeBackground));
        pal.setBrush(QPalette::Text, scheme.foreground(KColorScheme::NegativeText));
    } else {
        const QPalette qAppPalette = QApplication::palette();
        pal.setColor(QPalette::Base, qAppPalette.color(QPalette::Base));
        pal.setColor(QPalette::Text, qAppPalette.color(QPalette::Text));
    }
    setPalette(pal);
}

void SearchLineEdit::slotReturnPressed(const QString &text)
{
    Q_UNUSED(text);

    m_inputDelayTimer->stop();
    prepareLineEditForSearch();
    if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {
        m_searchType = Okular::Document::PreviousMatch;
        findPrev();
    } else {
        m_searchType = Okular::Document::NextMatch;
        findNext();
    }
}

void SearchLineEdit::startSearch()
{
    if (m_id == -1 || !m_color.isValid()) {
        return;
    }

    if (m_changed && (m_searchType == Okular::Document::NextMatch || m_searchType == Okular::Document::PreviousMatch)) {
        m_document->resetSearch(m_id);
    }
    m_changed = false;
    // search text if have more than 3 chars or else clear search
    QString thistext = text();
    if (thistext.length() >= qMax(m_minLength, 1)) {
        Q_EMIT searchStarted();
        m_searchRunning = true;
        m_document->searchText(m_id, thistext, m_fromStart, m_caseSensitivity, m_searchType, m_moveViewport, m_color);
    } else {
        m_document->resetSearch(m_id);
    }
}

void SearchLineEdit::searchFinished(int id, Okular::Document::SearchStatus endStatus)
{
    // ignore the searches not started by this search edit
    if (id != m_id) {
        return;
    }

    // if not found, use warning colors
    if (endStatus == Okular::Document::NoMatchFound) {
        QPalette pal = palette();
        const KColorScheme scheme(QPalette::Active, KColorScheme::View);
        pal.setBrush(QPalette::Base, scheme.background(KColorScheme::NegativeBackground));
        pal.setBrush(QPalette::Text, scheme.foreground(KColorScheme::NegativeText));
        setPalette(pal);
    } else {
        QPalette pal = palette();
        const QPalette qAppPalette = QApplication::palette();
        pal.setColor(QPalette::Base, qAppPalette.color(QPalette::Base));
        pal.setColor(QPalette::Text, qAppPalette.color(QPalette::Text));
        setPalette(pal);
    }

    m_searchRunning = false;
    Q_EMIT searchStopped();
}

SearchLineWidget::SearchLineWidget(QWidget *parent, Okular::Document *document)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_edit = new SearchLineEdit(this, document);
    layout->addWidget(m_edit);

    m_anim = new KBusyIndicatorWidget(this);
    m_anim->setFixedSize(22, 22);
    layout->addWidget(m_anim);
    m_anim->hide();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &SearchLineWidget::slotTimedout);

    connect(m_edit, &SearchLineEdit::searchStarted, this, &SearchLineWidget::slotSearchStarted);
    connect(m_edit, &SearchLineEdit::searchStopped, this, &SearchLineWidget::slotSearchStopped);
}

SearchLineEdit *SearchLineWidget::lineEdit() const
{
    return m_edit;
}

void SearchLineWidget::slotSearchStarted()
{
    m_timer->start(100);
}

void SearchLineWidget::slotSearchStopped()
{
    m_timer->stop();
    m_anim->hide();
}

void SearchLineWidget::slotTimedout()
{
    m_anim->show();
}

#include "moc_searchlineedit.cpp"
