/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_SEARCHWIDGET_H_
#define _OKULAR_SEARCHWIDGET_H_

#include <qwidget.h>

namespace Okular
{
class Document;
}

class QAction;
class QMenu;

class SearchLineEdit;

/**
 * @short A widget for find-as-you-type search. Outputs to the Document.
 *
 * This widget accepts keyboard input and performs a call to findTextAll(..)
 * in the Okular::Document class when there are 3 or more chars to search for.
 * It supports case sensitive/unsensitive(default) and provides a button
 * for switching between the 2 modes.
 */
class SearchWidget : public QWidget
{
    Q_OBJECT
public:
    SearchWidget(QWidget *parent, Okular::Document *document);
    void clearText();

private:
    QMenu *m_menu;
    QAction *m_matchPhraseAction, *m_caseSensitiveAction, *m_marchAllWordsAction, *m_marchAnyWordsAction;
    SearchLineEdit *m_lineEdit;

private Q_SLOTS:
    void slotMenuChaged(QAction *);
};

#endif
