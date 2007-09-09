/*
   Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>
   Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
   Copyright (c) 2006 Hamish Rodda <rodda@kde.org>
   Copyright 2007 Pino Toscano <pino@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KTREEVIEWSEARCHLINE_H
#define KTREEVIEWSEARCHLINE_H

#include <klineedit.h>

class QModelIndex;
class QTreeView;

/**
 * This class makes it easy to add a search line for filtering the items in
 * listviews based on a simple text search.
 *
 * No changes to the application other than instantiating this class with
 * appropriate QTreeViews should be needed.
 */

class KTreeViewSearchLine : public KLineEdit
{
    Q_OBJECT

    Q_PROPERTY( Qt::CaseSensitivity caseSensitity READ caseSensitivity WRITE setCaseSensitivity )
    Q_PROPERTY( bool keepParentsVisible READ keepParentsVisible WRITE setKeepParentsVisible )


  public:
    /**
     * Constructs a KTreeViewSearchLine with \a treeView being the QTreeView to
     * be filtered.
     *
     * If \a treeView is null then the widget will be disabled until listviews
     * are set with setTreeView(), setTreeViews() or added with addTreeView().
     */
    explicit KTreeViewSearchLine( QWidget *parent = 0, QTreeView *treeView = 0 );

    /**
     * Constructs a KTreeViewSearchLine with \a treeViews being the list of
     * pointers to QTreeViews to be filtered.
     *
     * If \a treeViews is empty then the widget will be disabled until listviews
     * are set with setTreeView(), setTreeViews() or added with addTreeView().
     */
    KTreeViewSearchLine( QWidget *parent, const QList<QTreeView *> &treeViews );


    /**
     * Destroys the KTreeViewSearchLine.
     */
    virtual ~KTreeViewSearchLine();

    /**
     * Returns true if the search is case sensitive.  This defaults to false.
     *
     * @see setCaseSensitive()
     */
    Qt::CaseSensitivity caseSensitivity() const;

    /**
     * Returns the current list of columns that will be searched.  If the
     * returned list is empty all visible columns will be searched.
     *
     * @see setSearchColumns
     */
    QList<int> searchColumns() const;

    /**
     * If this is true (the default) then the parents of matched items will also
     * be shown.
     *
     * @see setKeepParentsVisible()
     */
    bool keepParentsVisible() const;

    /**
     * Returns the listview that is currently filtered by the search.
     * If there are multiple listviews filtered, it returns 0.
     *
     * @see setTreeView(), treeView()
     */
    QTreeView *treeView() const;

    /**
     * Returns the list of pointers to listviews that are currently filtered by
     * the search.
     *
     * @see setTreeViews(), addTreeView(), treeView()
     */
    QList<QTreeView *> treeViews() const;

  public Q_SLOTS:
    /**
     * Adds a QTreeView to the list of listviews filtered by this search line.
     * If \a treeView is null then the widget will be disabled.
     *
     * @see treeView(), setTreeViews(), removeTreeView()
     */
    void addTreeView( QTreeView *treeView );

    /**
     * Removes a QTreeView from the list of listviews filtered by this search
     * line. Does nothing if \a treeView is 0 or is not filtered by the quick search
     * line.
     *
     * @see listVew(), setTreeView(), addTreeView()
     */
    void removeTreeView( QTreeView *treeView );

    /**
     * Updates search to only make visible the items that match \a pattern.  If
     * \a s is null then the line edit's text will be used.
     */
    virtual void updateSearch( const QString &pattern = QString() );

    /**
     * Make the search case sensitive or case insensitive.
     *
     * @see caseSenstivity()
     */
    void setCaseSensitivity( Qt::CaseSensitivity caseSensitivity );

    /**
     * When a search is active on a list that's organized into a tree view if
     * a parent or ancesestor of an item is does not match the search then it
     * will be hidden and as such so too will any children that match.
     *
     * If this is set to true (the default) then the parents of matching items
     * will be shown.
     *
     * @see keepParentsVisible
     */
    void setKeepParentsVisible( bool value );

    /**
     * Sets the list of columns to be searched.  The default is to search all,
     * visible columns which can be restored by passing \a columns as an empty
     * list.
     * If listviews to be filtered have different numbers or labels of columns
     * this method has no effect.
     *
     * @see searchColumns
     */
    void setSearchColumns( const QList<int> &columns );

    /**
     * Sets the QTreeView that is filtered by this search line, replacing any
     * previously filtered listviews.  If \a treeView is null then the widget will be
     * disabled.
     *
     * @see treeView(), setTreeViews()
     */
    void setTreeView( QTreeView *treeView );

    /**
     * Sets QTreeViews that are filtered by this search line, replacing any
     * previously filtered listviews.  If \a treeViews is empty then the widget will
     * be disabled.
     *
     * @see treeViews(), addTreeView(), setTreeView()
     */
    void setTreeViews( const QList<QTreeView *> &treeViews );


  protected:
    /**
     * Returns true if \a item matches the search \a pattern.  This will be evaluated
     * based on the value of caseSensitive().  This can be overridden in
     * subclasses to implement more complicated matching schemes.
     */
    virtual bool itemMatches( const QModelIndex &item, int row, const QString &pattern ) const;

    /**
    * Re-implemented for internal reasons.  API not affected.
    */
    virtual void contextMenuEvent( QContextMenuEvent* );

    /**
     * Updates search to only make visible appropriate items in \a treeView.  If
     * \a treeView is null then nothing is done.
     */
    virtual void updateSearch( QTreeView *treeView );

    /**
     * Connects signals of this listview to the appropriate slots of the search
     * line.
     */
    virtual void connectTreeView( QTreeView* );

    /**
     * Disconnects signals of a listviews from the search line.
     */
    virtual void disconnectTreeView( QTreeView* );

    /**
     * Checks columns in all listviews and decides whether choosing columns to
     * filter on makes any sense.
     *
     * Returns false if either of the following is true:
     * * there are no listviews connected,
     * * the listviews have different numbers of columns,
     * * the listviews have only one column,
     * * the listviews differ in column labels.
     *
     * Otherwise it returns true.
     *
     * @see setSearchColumns()
     */
    virtual bool canChooseColumnsCheck();

  protected Q_SLOTS:
    /**
     * When keys are pressed a new search string is created and a timer is
     * activated.  The most recent search is activated when this timer runs out
     * if another key has not yet been pressed.
     *
     * This method makes @param search the most recent search and starts the
     * timer.
     *
     * Together with activateSearch() this makes it such that searches are not
     * started until there is a short break in the users typing.
     *
     * @see activateSearch()
     */
    void queueSearch( const QString &search );

    /**
     * When the timer started with queueSearch() expires this slot is called.
     * If there has been another timer started then this slot does nothing.
     * However if there are no other pending searches this starts the list view
     * search.
     *
     * @see queueSearch()
     */
    void activateSearch();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void rowsInserted( const QModelIndex&, int, int ) const )
    Q_PRIVATE_SLOT( d, void treeViewDeleted( QObject* ) )
    Q_PRIVATE_SLOT( d, void slotColumnActivated( QAction* ) )
    Q_PRIVATE_SLOT( d, void slotAllVisibleColumns() )
};

/**
 * Creates a widget featuring a KTreeViewSearchLine, a label with the text
 * "Search" and a button to clear the search.
 */
class KTreeViewSearchLineWidget : public QWidget
{
    Q_OBJECT

  public:
    /**
     * Creates a KTreeViewSearchLineWidget for \a treeView with \a parent as the
     * parent.
     */
    explicit KTreeViewSearchLineWidget( QWidget *parent = 0, QTreeView *treeView = 0 );

    /**
     * Destroys the KTreeViewSearchLineWidget
     */
    ~KTreeViewSearchLineWidget();

    /**
     * Returns a pointer to the search line.
     */
    KTreeViewSearchLine *searchLine() const;

  protected Q_SLOTS:
    /**
     * Creates the widgets inside of the widget.  This is called from the
     * constructor via a single shot timer so that it it guaranteed to run
     * after construction is complete.  This makes it suitable for overriding in
     * subclasses.
     */
    virtual void createWidgets();

  protected:
    /**
     * Creates the search line.  This can be useful to reimplement in cases where
     * a KTreeViewSearchLine subclass is used.
     *
     * It is const because it is be called from searchLine(), which to the user
     * doesn't conceptually alter the widget.
     */
    virtual KTreeViewSearchLine *createSearchLine( QTreeView *treeView ) const;

  private:
    class Private;
    Private* const d;
};

#endif
