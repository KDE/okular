/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2007, 2009-2010 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_SEARCHLINEEDIT_H_
#define _OKULAR_SEARCHLINEEDIT_H_

#include "core/document.h"

#include <KLineEdit>

#include <kwidgetsaddons_version.h>
class KBusyIndicatorWidget;
class QTimer;

/**
 * @short A line edit for find-as-you-type search. Outputs to the Document.
 */
class SearchLineEdit : public KLineEdit
{
    Q_OBJECT
public:
    SearchLineEdit(QWidget *parent, Okular::Document *document);

    void clearText();

    void setSearchCaseSensitivity(Qt::CaseSensitivity cs);
    void setSearchMinimumLength(int length);
    void setSearchType(Okular::Document::SearchType type);
    void setSearchId(int id);
    void setSearchColor(const QColor &color);
    void setSearchMoveViewport(bool move);
    void setSearchFromStart(bool fromStart);
    void setFindAsYouType(bool findAsYouType);
    void resetSearch();

    bool isSearchRunning() const;

Q_SIGNALS:
    void searchStarted();
    void searchStopped();

public Q_SLOTS:
    void restartSearch();
    void stopSearch();
    void findNext();
    void findPrev();

private:
    void prepareLineEditForSearch();

    Okular::Document *m_document;
    QTimer *m_inputDelayTimer;
    int m_minLength;
    Qt::CaseSensitivity m_caseSensitivity;
    Okular::Document::SearchType m_searchType;
    int m_id;
    QColor m_color;
    bool m_moveViewport;
    bool m_changed;
    bool m_fromStart;
    bool m_findAsYouType;
    bool m_searchRunning;

private Q_SLOTS:
    void slotTextChanged(const QString &text);
    void slotReturnPressed(const QString &text);
    void startSearch();
    void searchFinished(int id, Okular::Document::SearchStatus endStatus);
};

class SearchLineWidget : public QWidget
{
    Q_OBJECT
public:
    SearchLineWidget(QWidget *parent, Okular::Document *document);

    SearchLineEdit *lineEdit() const;

private Q_SLOTS:
    void slotSearchStarted();
    void slotSearchStopped();
    void slotTimedout();

private:
    SearchLineEdit *m_edit;
    KBusyIndicatorWidget *m_anim;
    QTimer *m_timer;
};

#endif
