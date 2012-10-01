/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _FINDBAR_H_
#define _FINDBAR_H_

#include <qwidget.h>

class QAction;
class SearchLineWidget;

namespace Okular {
class Document;
}

class FindBar
    : public QWidget
{
    Q_OBJECT

    public:
        explicit FindBar( Okular::Document * document, QWidget * parent = 0 );
        virtual ~FindBar();

        QString text() const;
        Qt::CaseSensitivity caseSensitivity() const;

        void focusAndSetCursor();
        bool maybeHide();

    signals:
        void forwardKeyPressEvent( QKeyEvent* );

    public slots:
        void findNext();
        void findPrev();
        void resetSearch();

    private slots:
        void caseSensitivityChanged();
        void fromCurrentPageChanged();
        void closeAndStopSearch();

    private:
        SearchLineWidget * m_search;
        QAction * m_caseSensitiveAct;
        QAction * m_fromCurrentPageAct;
        bool eventFilter( QObject *target, QEvent *event );
        bool m_active;
};


#endif
