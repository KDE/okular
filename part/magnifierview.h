/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
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
