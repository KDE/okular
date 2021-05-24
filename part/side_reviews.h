/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_SIDE_REVIEWS_H_
#define _OKULAR_SIDE_REVIEWS_H_

#include <QModelIndexList>
#include <QVector>
#include <QWidget>

#include "core/observer.h"

class QModelIndex;

namespace Okular
{
class Annotation;
class Document;
}

class AnnotationModel;
class AuthorGroupProxyModel;
class PageFilterProxyModel;
class PageGroupProxyModel;
class KTreeViewSearchLine;
class TreeView;

/**
 * @short ...
 */
class Reviews : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    Reviews(QWidget *parent, Okular::Document *document);
    ~Reviews() override;

    // [INHERITED] from DocumentObserver
    void notifyCurrentPageChanged(int previous, int current) override;

    void reparseConfig();

public Q_SLOTS:
    void slotPageEnabled(bool);
    void slotAuthorEnabled(bool);
    void slotCurrentPageOnly(bool);
    void slotExpandAll();
    void slotCollapseAll();

Q_SIGNALS:
    void openAnnotationWindow(Okular::Annotation *annotation, int pageNumber);

private Q_SLOTS:
    void activated(const QModelIndex &);
    void contextMenuRequested(const QPoint);
    void saveSearchOptions();

private:
    QModelIndexList retrieveAnnotations(const QModelIndex &idx) const;

    // data fields (GUI)
    KTreeViewSearchLine *m_searchLine;
    TreeView *m_view;
    // internal storage
    Okular::Document *m_document;
    AnnotationModel *m_model;
    AuthorGroupProxyModel *m_authorProxy;
    PageFilterProxyModel *m_filterProxy;
    PageGroupProxyModel *m_groupProxy;
};

#endif
