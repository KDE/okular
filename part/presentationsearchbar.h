/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_PRESENTATIONSEARCHBAR_H_
#define _OKULAR_PRESENTATIONSEARCHBAR_H_

#include <qwidget.h>

class SearchLineEdit;

namespace Okular
{
class Document;
}

class PresentationSearchBar : public QWidget
{
    Q_OBJECT

public:
    PresentationSearchBar(Okular::Document *document, QWidget *anchor, QWidget *parent = nullptr);
    ~PresentationSearchBar() override;

    void forceSnap();
    void focusOnSearchEdit();

protected:
    void resizeEvent(QResizeEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;

private:
    QWidget *m_handle;
    QWidget *m_anchor;
    QPoint m_point;
    bool m_snapped;
    QPoint m_drag;

    SearchLineEdit *m_search;
};

#endif
