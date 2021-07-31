/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_MINIBAR_H_
#define _OKULAR_MINIBAR_H_

#include "core/observer.h"
#include <KLineEdit>
#include <QSet>
#include <qwidget.h>

namespace Okular
{
class Document;
}

class MiniBar;
class HoverButton;
class QIntValidator;
class QLabel;
class QToolBar;

// [private widget] lineEdit for entering/validating page numbers
class PagesEdit : public KLineEdit
{
    Q_OBJECT

public:
    explicit PagesEdit(MiniBar *parent);
    void setText(const QString &) override;

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private Q_SLOTS:
    void updatePalette();

private:
    MiniBar *m_miniBar;
    bool m_eatClick;
};

class PageNumberEdit : public PagesEdit
{
    Q_OBJECT

public:
    explicit PageNumberEdit(MiniBar *miniBar);
    void setPagesNumber(int pages);

private:
    QIntValidator *m_validator;
};

class PageLabelEdit : public PagesEdit
{
    Q_OBJECT
public:
    explicit PageLabelEdit(MiniBar *parent);
    void setText(const QString &newText) override;
    void setPageLabels(const QVector<Okular::Page *> &pageVector);

Q_SIGNALS:
    void pageNumberChosen(int page);

private Q_SLOTS:
    void pageChosen();

private:
    QString m_lastLabel;
    QMap<QString, int> m_labelPageMap;
};

/**
 * @short The object that observes the document and feeds the minibars
 */
class MiniBarLogic : public QObject, public Okular::DocumentObserver
{
    Q_OBJECT

public:
    MiniBarLogic(QObject *parent, Okular::Document *m_document);
    ~MiniBarLogic() override;

    void addMiniBar(MiniBar *miniBar);
    void removeMiniBar(MiniBar *miniBar);

    Okular::Document *document() const;
    int currentPage() const;

    // [INHERITED] from DocumentObserver
    void notifySetup(const QVector<Okular::Page *> &pageVector, int setupFlags) override;
    void notifyCurrentPageChanged(int previous, int current) override;

private:
    QSet<MiniBar *> m_miniBars;
    Okular::Document *m_document;
};

/**
 * @short A widget to display page number and change current page.
 */
class MiniBar : public QWidget
{
    Q_OBJECT
    friend class MiniBarLogic;

public:
    MiniBar(QWidget *parent, MiniBarLogic *miniBarLogic);
    ~MiniBar() override;

    void changeEvent(QEvent *event) override;

Q_SIGNALS:
    void gotoPage();
    void prevPage();
    void nextPage();
    void forwardKeyPressEvent(QKeyEvent *e);

public Q_SLOTS:
    void slotChangePageFromReturn();
    void slotChangePage(int page);
    void slotEmitNextPage();
    void slotEmitPrevPage();
    void slotToolBarIconSizeChanged();

private:
    void resizeForPage(int pages, const QString &pagesOrLabelString);
    bool eventFilter(QObject *target, QEvent *event) override;

    MiniBarLogic *m_miniBarLogic;
    PageNumberEdit *m_pageNumberEdit;
    PageLabelEdit *m_pageLabelEdit;
    QLabel *m_pageNumberLabel;
    HoverButton *m_prevButton;
    HoverButton *m_pagesButton;
    HoverButton *m_nextButton;
    QToolBar *m_oldToobarParent;
};

/**
 * @short A small progress bar.
 */
class ProgressWidget : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    ProgressWidget(QWidget *parent, Okular::Document *document);
    ~ProgressWidget() override;

    // [INHERITED] from DocumentObserver
    void notifyCurrentPageChanged(int previous, int current) override;

    void slotGotoNormalizedPage(float index);

Q_SIGNALS:
    void prevPage();
    void nextPage();

protected:
    void setProgress(float percentage);

    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

private:
    Okular::Document *m_document;
    float m_progressPercentage;
};

#endif
