/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2004 Albert Astals Cid <aacid@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    With portions of code from kpdf/kpdf_pagewidget.h by:
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003 Kurt Pfeifle <kpfeifle@danka.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
// This file follows coding style described in kdebase/kicker/HACKING

#ifndef _OKULAR_PAGEVIEW_H_
#define _OKULAR_PAGEVIEW_H_

#include "config-okular.h"
#include "core/area.h"
#include "core/observer.h"
#include "core/view.h"
#include "pageviewutils.h"
#include <QAbstractScrollArea>
#include <QList>
#include <QVector>

class QMenu;
class QMimeData;
class KActionCollection;

namespace Okular
{
class Action;
class Document;
class DocumentViewport;
class FormFieldSignature;
class Annotation;
class MovieAction;
class RenditionAction;
}

class PageViewPrivate;

class QGestureEvent;

/**
 * @short The main view. Handles zoom and continuous mode.. oh, and page
 * @short display of course :-)
 * ...
 */
class PageView : public QAbstractScrollArea, public Okular::DocumentObserver, public Okular::View
{
    Q_OBJECT

public:
    PageView(QWidget *parent, Okular::Document *document);
    ~PageView() override;

    // Zoom mode ( last 4 are internally used only! )
    enum ZoomMode { ZoomFixed = 0, ZoomFitWidth = 1, ZoomFitPage = 2, ZoomFitAuto = 3, ZoomIn, ZoomOut, ZoomRefreshCurrent, ZoomActual };

    enum ClearMode { ClearAllSelection, ClearOnlyDividers };

    // create actions that interact with this widget
    void setupBaseActions(KActionCollection *ac);
    void setupViewerActions(KActionCollection *ac);
    void setupActions(KActionCollection *ac);
    void setupActionsPostGUIActivated();
    void updateActionState(bool docHasPages, bool docHasFormWidgets);

    // misc methods (from RMB menu/children)
    bool canFitPageWidth() const;
    void fitPageWidth(int page);
    // keep in sync with pageviewutils
    void displayMessage(const QString &message, const QString &details = QString(), PageViewMessage::Icon icon = PageViewMessage::Info, int duration = -1);

    // inherited from DocumentObserver
    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;
    void notifyViewportChanged(bool smoothMove) override;
    void notifyPageChanged(int pageNumber, int changedFlags) override;
    void notifyContentsCleared(int changedFlags) override;
    void notifyZoom(int factor) override;
    bool canUnloadPixmap(int pageNum) const override;
    void notifyCurrentPageChanged(int previous, int current) override;

    // inherited from View
    bool supportsCapability(ViewCapability capability) const override;
    CapabilityFlags capabilityFlags(ViewCapability capability) const override;
    QVariant capability(ViewCapability capability) const override;
    void setCapability(ViewCapability capability, const QVariant &option) override;

    QList<Okular::RegularAreaRect *> textSelections(const QPoint start, const QPoint end, int &firstpage);
    Okular::RegularAreaRect *textSelectionForItem(const PageViewItem *item, const QPoint startPoint = QPoint(), const QPoint endPoint = QPoint());

    void reparseConfig();

    KActionCollection *actionCollection() const;
    QAction *toggleFormsAction() const;

    int contentAreaWidth() const;
    int contentAreaHeight() const;
    QPoint contentAreaPosition() const;
    QPoint contentAreaPoint(const QPoint pos) const;
    QPointF contentAreaPoint(const QPointF pos) const;

    bool areSourceLocationsShownGraphically() const;
    void setShowSourceLocationsGraphically(bool show);

    void setLastSourceLocationViewport(const Okular::DocumentViewport &vp);
    void clearLastSourceLocationViewport();

    void updateCursor();

    void highlightSignatureFormWidget(const Okular::FormFieldSignature *form);

    void showNoSigningCertificatesDialog(bool nonDateValidCerts);

    Okular::Document *document() const;

public Q_SLOTS:
    void copyTextSelection() const;
    void selectAll();

    void openAnnotationWindow(Okular::Annotation *annotation, int pageNumber);
    void reloadForms();

    void slotSelectPage();

    void slotAction(Okular::Action *action);
    void slotFormChanged(int pageNumber);

    void externalKeyPressEvent(QKeyEvent *e);

Q_SIGNALS:
    void rightClick(const Okular::Page *, const QPoint);
    void mouseBackButtonClick();
    void mouseForwardButtonClick();
    void escPressed();
    void fitWindowToPage(const QSize pageViewPortSize, const QSize pageSize);
    void triggerSearch(const QString &text);
    void requestOpenFile(const QString &filePath, int pageNumber);

protected:
    bool event(QEvent *event) override;

    void resizeEvent(QResizeEvent *) override;
    bool gestureEvent(QGestureEvent *e);

    // mouse / keyboard events
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void inputMethodEvent(QInputMethodEvent *) override;
    void wheelEvent(QWheelEvent *) override;

    void paintEvent(QPaintEvent *e) override;
    void tabletEvent(QTabletEvent *e) override;
    void continuousZoom(double delta);
    void continuousZoomEnd();
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

    bool viewportEvent(QEvent *e) override;

    void scrollContentsBy(int dx, int dy) override;

private:
    // draw background and items on the opened qpainter
    void drawDocumentOnPainter(const QRect contentsRect, QPainter *p);
    // update item width and height using current zoom parameters
    void updateItemSize(PageViewItem *item, int colWidth, int rowHeight);
    // return the widget placed on a certain point or 0 if clicking on empty space
    PageViewItem *pickItemOnPoint(int x, int y);
    // start / modify / clear selection rectangle
    void selectionStart(const QPoint pos, const QColor &color, bool aboveAll = false);
    void selectionClear(const ClearMode mode = ClearAllSelection);
    QMimeData *getTableContents() const;
    void drawTableDividers(QPainter *screenPainter);
    void guessTableDividers();
    // update either text or rectangle selection
    void updateSelection(const QPoint pos);
    // compute the zoom factor value for FitWidth and FitPage mode
    double zoomFactorFitMode(ZoomMode mode);
    // update internal zoom values and end in a slotRelayoutPages();
    void updateZoom(ZoomMode newZoomMode);
    // update the text on the label using global zoom value or current page's one
    void updateZoomText();
    // update the text enabled status of the zoom actions
    void updateZoomActionsEnabledStatus();
    // update view mode (single, facing...)
    void updateViewMode(const int nr);
    void textSelectionClear();
    // updates cursor
    void updateCursor(const QPoint p);

    void moveMagnifier(const QPoint p);
    void updateMagnifier(const QPoint p);

    int viewColumns() const;

    void center(int cx, int cy, bool smoothMove = false);
    void scrollTo(int x, int y, bool smoothMove = false);

    void toggleFormWidgets(bool on);

    void resizeContentArea(const QSize newSize);
    void updatePageStep();

    void addSearchWithinDocumentAction(QMenu *menu, const QString &searchText);
    void addWebShortcutsMenu(QMenu *menu, const QString &text);
    QMenu *createProcessLinkMenu(PageViewItem *item, const QPoint eventPos);
    // used when selecting stuff, makes the view scroll as necessary to keep the mouse inside the view
    void scrollPosIntoView(const QPoint pos);
    QPoint viewportToContentArea(const Okular::DocumentViewport &vp) const;

    // called from slots to turn off trim modes mutually exclusive to id
    void updateTrimMode(int except_id);

    // handle link clicked
    bool mouseReleaseOverLink(const Okular::ObjectRect *rect) const;

    void createAnnotationsVideoWidgets(PageViewItem *item, const QList<Okular::Annotation *> &annotations);

    // Update speed of animated smooth scroll transitions
    void updateSmoothScrollAnimationSpeed();

    /*
     * returns the continuous mode value of the current document, by either:
     * - if the continuous mode action is initialized, then we return its associated value
     * - if not, then we will fallback to the default settings
     */
    bool getContinuousMode() const;

    // don't want to expose classes in here
    class PageViewPrivate *d;

private Q_SLOTS:
    // used to decouple the notifyViewportChanged calle
    void slotRealNotifyViewportChanged(bool smoothMove);
    // activated either directly or via queued connection on notifySetup
    void slotRelayoutPages();
    // activated by the resize event delay timer
    void delayedResizeEvent();
    // activated either directly or via the contentsMoving(int,int) signal
    void slotRequestVisiblePixmaps(int newValue = -1);
    // activated by the autoscroll timer (Shift+Up/Down keys)
    void slotAutoScroll();
    // activated by the dragScroll timer
    void slotDragScroll();
    // show the welcome message
    void slotShowWelcome();
    // activated by left click timer
    void slotShowSizeAllCursor();

    void slotHandleWebShortcutAction();
    void slotConfigureWebShortcuts();

    // connected to local actions (toolbar, menu, ..)
    void slotZoom();
    void slotZoomIn();
    void slotZoomOut();
    void slotZoomActual();
    void slotFitToWidthToggled(bool);
    void slotFitToPageToggled(bool);
    void slotAutoFitToggled(bool);
    void slotViewMode(QAction *action);
    void slotContinuousToggled();
    void slotReadingDirectionToggled(bool leftToRight);
    void slotUpdateReadingDirectionAction();
    void slotSetMouseNormal();
    void slotSetMouseZoom();
    void slotSetMouseMagnifier();
    void slotSetMouseSelect();
    void slotSetMouseTextSelect();
    void slotSetMouseTableSelect();
    void slotSignature();
    void slotAutoScrollUp();
    void slotAutoScrollDown();
    void slotScrollUp(int nSteps = 0);
    void slotScrollDown(int nSteps = 0);
    void slotRotateClockwise();
    void slotRotateCounterClockwise();
    void slotRotateOriginal();
    void slotTrimMarginsToggled(bool);
    void slotTrimToSelectionToggled(bool);
    void slotToggleForms();
    void slotRefreshPage();
#if HAVE_SPEECH
    void slotSpeakDocument();
    void slotSpeakCurrentPage();
    void slotStopSpeaks();
    void slotPauseResumeSpeech();
#endif
    void slotAnnotationWindowDestroyed(QObject *window);
    void slotProcessMovieAction(const Okular::MovieAction *action);
    void slotProcessRenditionAction(const Okular::RenditionAction *action);
    void slotFitWindowToPage();
};

#endif

/* kate: replace-tabs on; indent-width 4; */
