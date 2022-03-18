/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_PRESENTATIONWIDGET_H_
#define _OKULAR_PRESENTATIONWIDGET_H_

#include "core/area.h"
#include "core/observer.h"
#include "core/pagetransition.h"
#include <QDomElement>
#include <QList>
#include <QPixmap>
#include <QStringList>
#include <qwidget.h>

class QLineEdit;
class QToolBar;
class QTimer;
class QGestureEvent;
class KActionCollection;
class KSelectAction;
class SmoothPathEngine;
struct PresentationFrame;
class PresentationSearchBar;
class DrawingToolActions;

namespace Okular
{
class Action;
class Annotation;
class Document;
class MovieAction;
class Page;
class RenditionAction;
}

/**
 * @short A widget that shows pages as fullscreen slides (with transitions fx).
 *
 * This is a fullscreen widget that displays
 */
class PresentationWidget : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    PresentationWidget(QWidget *parent, Okular::Document *doc, DrawingToolActions *drawingToolActions, KActionCollection *collection);
    ~PresentationWidget() override;

    // inherited from DocumentObserver
    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;
    void notifyViewportChanged(bool smoothMove) override;
    void notifyPageChanged(int pageNumber, int changedFlags) override;
    bool canUnloadPixmap(int pageNumber) const override;
    void notifyCurrentPageChanged(int previous, int current) override;

public Q_SLOTS:
    void slotFind();

protected:
    // widget events
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    bool gestureEvent(QGestureEvent *e);

    // Catch TabletEnterProximity and TabletLeaveProximity events from the QApplication
    bool eventFilter(QObject *o, QEvent *ev) override;

private:
    const void *getObjectRect(Okular::ObjectRect::ObjectType type, int x, int y, QRect *geometry = nullptr) const;
    const Okular::Action *getLink(int x, int y, QRect *geometry = nullptr) const;
    const Okular::Annotation *getAnnotation(int x, int y, QRect *geometry = nullptr) const;
    void testCursorOnLink(int x, int y);
    void overlayClick(const QPoint position);
    void changePage(int newPage);
    void generatePage(bool disableTransition = false);
    void generateIntroPage(QPainter &p);
    void generateContentsPage(int page, QPainter &p);
    void generateOverlay();
    void initTransition(const Okular::PageTransition *transition);
    const Okular::PageTransition defaultTransition() const;
    const Okular::PageTransition defaultTransition(int) const;
    QRect routeMouseDrawingEvent(QMouseEvent *);
    void startAutoChangeTimer();
    /** @returns Configure -> Presentation -> Preferred screen */
    QScreen *defaultScreen() const;
    void requestPixmaps();
    /** @param newScreen must be valid. */
    void setScreen(const QScreen *newScreen);
    void inhibitPowerManagement();
    void allowPowerManagement();
    void showTopBar(bool);
    // create actions that interact with this widget
    void setupActions();
    void setPlayPauseIcon();

    // cache stuff
    int m_width;
    int m_height;
    QPixmap m_lastRenderedPixmap;
    QPixmap m_lastRenderedOverlay;
    QRect m_overlayGeometry;
    const Okular::Action *m_pressedLink;
    bool m_handCursor;
    SmoothPathEngine *m_drawingEngine;
    QRect m_drawingRect;
    uint m_screenInhibitCookie;
    int m_sleepInhibitFd;

    // transition related
    QTimer *m_transitionTimer;
    QTimer *m_overlayHideTimer;
    QTimer *m_nextPageTimer;
    int m_transitionDelay;
    int m_transitionMul;
    int m_transitionSteps;
    QList<QRect> m_transitionRects;
    Okular::PageTransition m_currentTransition;
    QPixmap m_currentPagePixmap;
    QPixmap m_previousPagePixmap;
    double m_currentPixmapOpacity;

    // misc stuff
    QWidget *m_parentWidget;
    Okular::Document *m_document;
    QVector<PresentationFrame *> m_frames;
    int m_frameIndex;
    QStringList m_metaStrings;
    QToolBar *m_topBar;
    QLineEdit *m_pagesEdit;
    PresentationSearchBar *m_searchBar;
    KActionCollection *m_ac;
    KSelectAction *m_screenSelect;
    QDomElement m_currentDrawingToolElement;
    bool m_isSetup;
    bool m_blockNotifications;
    bool m_inBlackScreenMode;
    bool m_showSummaryView;
    bool m_advanceSlides;
    bool m_goToPreviousPageOnRelease;
    bool m_goToNextPageOnRelease;

    /** TODO Qt6: Just use QWidget::screen() instead of this. */
    static inline QScreen *oldQt_screenOf(const QWidget *widget)
    {
        return widget->screen();
    }

private Q_SLOTS:
    void slotNextPage();
    void slotPrevPage();
    void slotFirstPage();
    void slotLastPage();
    void slotHideOverlay();
    void slotTransitionStep();
    void slotDelayedEvents();
    void slotPageChanged();
    void clearDrawings();
    void chooseScreen(QAction *);
    void toggleBlackScreenMode(bool);
    void slotProcessMovieAction(const Okular::MovieAction *action);
    void slotProcessRenditionAction(const Okular::RenditionAction *action);
    void slotTogglePlayPause();
    void slotChangeDrawingToolEngine(const QDomElement &element);
    void slotAddDrawingToolActions();
};

#endif
