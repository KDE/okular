/*
    SPDX-FileCopyrightText: 2004-2005 Enrico Ros <eros.kde@email.it>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _PAGEVIEW_UTILS_H_
#define _PAGEVIEW_UTILS_H_

#include <QHash>
#include <QPixmap>
#include <QRect>
#include <qwidget.h>

#include "core/area.h"

class QTimer;
class FormWidgetIface;
class PageView;
class VideoWidget;

namespace Okular
{
class Movie;
class Page;
}

/**
 * @short PageViewItem represents graphically a page into the PageView.
 *
 * It has methods for settings Item's geometry and other visual properties such
 * as the individual zoom factor.
 */
class PageViewItem
{
public:
    explicit PageViewItem(const Okular::Page *page);
    ~PageViewItem();

    PageViewItem(const PageViewItem &) = delete;
    PageViewItem &operator=(const PageViewItem &) = delete;

    const Okular::Page *page() const;
    int pageNumber() const;
    double zoomFactor() const;
    bool isVisible() const;
    QSet<FormWidgetIface *> &formWidgets();
    QHash<Okular::Movie *, VideoWidget *> &videoWidgets();

    /* The page is cropped as follows: */
    const Okular::NormalizedRect &crop() const;

    /* Real geometry into which the cropped page is rendered: */
    const QRect &croppedGeometry() const;
    int croppedWidth() const;
    int croppedHeight() const;

    /* "Uncropped" geometry:
     * If the whole page was rendered into the uncropped geometry then the
     * cropped page would be rendered into the real geometry.
     * (Hence, uncropped always contains cropped, and they are equal only if
     * the page is uncropped.) This is just for convenience in calculations.
     */
    const QRect &uncroppedGeometry() const;
    int uncroppedWidth() const;
    int uncroppedHeight() const;

    /* Convert absolute geometry coordinates to normalized [0,1] page coordinates: */
    double absToPageX(double absX) const;
    double absToPageY(double absY) const;

    void setWHZC(int w, int h, double zoom, const Okular::NormalizedRect &c);
    void moveTo(int x, int y);
    void setVisible(bool visible);
    void invalidate();
    bool setFormWidgetsVisible(bool visible);
    void reloadFormWidgetsState();

private:
    const Okular::Page *m_page;
    double m_zoomFactor;
    bool m_visible;
    bool m_formsVisible;
    QRect m_croppedGeometry;
    QRect m_uncroppedGeometry;
    Okular::NormalizedRect m_crop;
    QSet<FormWidgetIface *> m_formWidgets;
    QHash<Okular::Movie *, VideoWidget *> m_videoWidgets;
};

/**
 * @short A widget that displays messages in the top-left corner.
 *
 * This is a widget with thin border and rounded corners that displays a given
 * text along as an icon. It's meant to be used for displaying messages to the
 * user by placing this above other widgets.
 */
class PageViewMessage : public QWidget
{
    Q_OBJECT

public:
    explicit PageViewMessage(QWidget *parent);

    enum Icon { None, Info, Warning, Error, Find, Annotation };
    void display(const QString &message, const QString &details = QString(), Icon icon = Info, int durationMs = 4000);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QRect computeTextRect(const QString &message, int extra_width) const;
    void computeSizeAndResize();
    QString m_message;
    QString m_details;
    QPixmap m_symbol;
    QTimer *m_timer;
    int m_lineSpacing;
};

#endif
