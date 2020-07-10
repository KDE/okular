/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
