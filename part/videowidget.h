/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_VIDEOWIDGET_H_
#define _OKULAR_VIDEOWIDGET_H_

#include <qwidget.h>

namespace Okular
{
class Annotation;
class Document;
class Movie;
class NormalizedRect;
}

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    VideoWidget(const Okular::Annotation *annot, Okular::Movie *movie, Okular::Document *document, QWidget *parent = nullptr);
    ~VideoWidget() override;

    void setNormGeometry(const Okular::NormalizedRect &rect);
    Okular::NormalizedRect normGeometry() const;

    bool isPlaying() const;

    /**
     * This method is called when the page the video widget is located on has been initialized.
     */
    void pageInitialized();

    /**
     * This method is called when the page the video widget is located on has been entered.
     */
    void pageEntered();

    /**
     * This method is called when the page the video widget is located on has been left.
     */
    void pageLeft();

public Q_SLOTS:
    void play();
    void pause();
    void stop();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // private storage
    class Private;
    Private *d;
};

#endif
