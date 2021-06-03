/*
    SPDX-FileCopyrightText: 2006 Chu Xiaodong <xiaodongchu@gmail.com>
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _ANNOTWINDOW_H_
#define _ANNOTWINDOW_H_

#include <QColor>
#include <QFrame>

namespace Okular
{
class Annotation;
class Document;
}

namespace GuiUtils
{
class LatexRenderer;
}

class KTextEdit;
class MovableTitle;
class QMenu;

class AnnotWindow : public QFrame
{
    Q_OBJECT
public:
    AnnotWindow(QWidget *parent, Okular::Annotation *annot, Okular::Document *document, int page);
    ~AnnotWindow() override;

    void reloadInfo();

    Okular::Annotation *annotation() const;
    int pageNumber() const;

    void updateAnnotation(Okular::Annotation *a);

private:
    MovableTitle *m_title;
    KTextEdit *textEdit;
    QColor m_color;
    GuiUtils::LatexRenderer *m_latexRenderer;
    Okular::Annotation *m_annot;
    Okular::Document *m_document;
    int m_page;
    int m_prevCursorPos;
    int m_prevAnchorPos;

public Q_SLOTS:
    void renderLatex(bool render);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void slotUpdateUndoAndRedoInContextMenu(QMenu *menu);
    void slotOptionBtn();
    void slotsaveWindowText();
    void slotHandleContentsChangedByUndoRedo(Okular::Annotation *annot, const QString &contents, int cursorPos, int anchorPos);

Q_SIGNALS:
    void containsLatex(bool);
};

#endif
