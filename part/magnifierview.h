/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef MAGNIFIERVIEW_H
#define MAGNIFIERVIEW_H

#include "core/observer.h"
#include "core/page.h"
#include <QWidget>

class MagnifierView : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT

public:
    explicit MagnifierView(Okular::Document *document, QWidget *parent = nullptr);
    ~MagnifierView() override;

    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;
    void notifyPageChanged(int page, int flags) override;
    void notifyCurrentPageChanged(int previous, int current) override;
    bool canUnloadPixmap(int page) const override;

    void updateView(const Okular::NormalizedPoint &p, const Okular::Page *page);
    void move(int x, int y);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    Okular::NormalizedRect normalizedView() const;
    void requestPixmap();
    void drawTicks(QPainter *p);

private:
    Okular::Document *m_document;
    Okular::NormalizedPoint m_viewpoint;
    const Okular::Page *m_page;
    int m_current;
    QVector<Okular::Page *> m_pages;
};

#endif // MAGNIFIERVIEW_H
