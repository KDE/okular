/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "side_reviews.h"

// qt/kde includes
#include <QtCore/QStringList>
#include <QtGui/QHeaderView>
#include <QtGui/QLayout>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QSizePolicy>
#include <QtGui/QTextDocument>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>

#include <kaction.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kicon.h>

// local includes
#include "core/annotations.h"
#include "core/document.h"
#include "core/page.h"
#include "settings.h"
#include "annotationpopup.h"
#include "annotationproxymodels.h"
#include "annotationmodel.h"
#include "ktreeviewsearchline.h"

class TreeView : public QTreeView
{
  public:
    TreeView( Okular::Document *document, QWidget *parent = 0 )
      : QTreeView( parent ), m_document( document )
    {
    }

  protected:
    virtual void paintEvent( QPaintEvent *event )
    {
      bool hasAnnotations = false;
      for ( uint i = 0; i < m_document->pages(); ++i )
        if ( m_document->page( i )->hasAnnotations() ) {
          hasAnnotations = true;
          break;
        }
      if ( !hasAnnotations ) {
        QPainter p( viewport() );
        p.setRenderHint( QPainter::Antialiasing, true );
        p.setClipRect( event->rect() );

        QTextDocument document;
        document.setHtml( i18n( "<div align=center><h3>No annotations</h3>"
                                "To create new annotations press F6 or select <i>Tools -&gt; Review</i>"
                                " from the menu.</div>" ) );
        document.setTextWidth( width() - 50 );

        const uint w = document.size().width() + 20;
        const uint h = document.size().height() + 20;

        p.setBrush( palette().background() );
        p.translate( 0.5, 0.5 );
        p.drawRoundRect( 15, 15, w, h, (8*200)/w, (8*200)/h );
        p.translate( 20, 20 );
        document.drawContents( &p );

      } else {
        QTreeView::paintEvent( event );
      }
    }

  private:
    Okular::Document *m_document;
};


Reviews::Reviews( QWidget * parent, Okular::Document * document )
    : QWidget( parent ), m_document( document )
{
    // create widgets and layout them vertically
    QVBoxLayout * vLayout = new QVBoxLayout( this );
    vLayout->setMargin( 0 );
    vLayout->setSpacing( 6 );

    m_view = new TreeView( m_document, this );
    m_view->setAlternatingRowColors( true );
    m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_view->header()->hide();

    QToolBar *toolBar = new QToolBar( this );
    toolBar->setObjectName( QLatin1String( "reviewOptsBar" ) );
    QSizePolicy sp = toolBar->sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    toolBar->setSizePolicy( sp );

    m_model = new AnnotationModel( m_document, m_view );

    m_filterProxy = new PageFilterProxyModel( m_view );
    m_groupProxy = new PageGroupProxyModel( m_view );
    m_authorProxy  = new AuthorGroupProxyModel( m_view );

    m_filterProxy->setSourceModel( m_model );
    m_groupProxy->setSourceModel( m_filterProxy );
    m_authorProxy->setSourceModel( m_groupProxy );


    m_view->setModel( m_authorProxy );

    m_searchLine = new KTreeViewSearchLine( this, m_view );
    m_searchLine->setCaseSensitivity( Okular::Settings::self()->reviewsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::self()->reviewsSearchRegularExpression() );
    connect( m_searchLine, SIGNAL(searchOptionsChanged()), this, SLOT(saveSearchOptions()) );
    vLayout->addWidget( m_searchLine );
    vLayout->addWidget( m_view );
    vLayout->addWidget( toolBar );

    toolBar->setIconSize( QSize( 16, 16 ) );
    toolBar->setMovable( false );
    // - add Page button
    QAction * groupByPageAction = toolBar->addAction( KIcon( "text-x-generic" ), i18n( "Group by Page" ) );
    groupByPageAction->setCheckable( true );
    connect( groupByPageAction, SIGNAL(toggled(bool)), this, SLOT(slotPageEnabled(bool)) );
    groupByPageAction->setChecked( Okular::Settings::groupByPage() );
    // - add Author button
    QAction * groupByAuthorAction = toolBar->addAction( KIcon( "user-identity" ), i18n( "Group by Author" ) );
    groupByAuthorAction->setCheckable( true );
    connect( groupByAuthorAction, SIGNAL(toggled(bool)), this, SLOT(slotAuthorEnabled(bool)) );
    groupByAuthorAction->setChecked( Okular::Settings::groupByAuthor() );

    // - add separator
    toolBar->addSeparator();
    // - add Current Page Only button
    QAction * curPageOnlyAction = toolBar->addAction( KIcon( "arrow-down" ), i18n( "Show reviews for current page only" ) );
    curPageOnlyAction->setCheckable( true );
    connect( curPageOnlyAction, SIGNAL(toggled(bool)), this, SLOT(slotCurrentPageOnly(bool)) );
    curPageOnlyAction->setChecked( Okular::Settings::currentPageOnly() );

    connect( m_view, SIGNAL(activated(QModelIndex)),
             this, SLOT(activated(QModelIndex)) );

    m_view->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( m_view, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(contextMenuRequested(QPoint)) );

}

Reviews::~Reviews()
{
    m_document->removeObserver( this );
}

//BEGIN DocumentObserver Notifies 
void Reviews::notifyCurrentPageChanged( int previousPage, int currentPage )
{
    Q_UNUSED( previousPage )

    m_filterProxy->setCurrentPage( currentPage );
}
//END DocumentObserver Notifies 

void Reviews::reparseConfig()
{
    m_searchLine->setCaseSensitivity( Okular::Settings::reviewsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::reviewsSearchRegularExpression() );
    m_view->update();
}

//BEGIN GUI Slots -> requestListViewUpdate
void Reviews::slotPageEnabled( bool on )
{
    // store toggle state in Settings and update the listview
    Okular::Settings::setGroupByPage( on );
    m_groupProxy->groupByPage( on );

    m_view->expandAll();
}

void Reviews::slotAuthorEnabled( bool on )
{
    // store toggle state in Settings and update the listview
    Okular::Settings::setGroupByAuthor( on );
    m_authorProxy->groupByAuthor( on );

    m_view->expandAll();
}

void Reviews::slotCurrentPageOnly( bool on )
{
    // store toggle state in Settings and update the listview
    Okular::Settings::setCurrentPageOnly( on );
    m_filterProxy->groupByCurrentPage( on );

    m_view->expandAll();
}
//END GUI Slots


void Reviews::activated( const QModelIndex &index )
{
    const QModelIndex authorIndex = m_authorProxy->mapToSource( index );
    const QModelIndex filterIndex = m_groupProxy->mapToSource( authorIndex );
    const QModelIndex annotIndex = m_filterProxy->mapToSource( filterIndex );

    Okular::Annotation *annotation = m_model->annotationForIndex( annotIndex );
    if ( !annotation )
      return;

    int pageNumber = m_model->data( annotIndex, AnnotationModel::PageRole ).toInt();
    const Okular::Page * page = m_document->page( pageNumber );

    // calculating the right coordinates to center the view on the annotation
    QRect rect = Okular::AnnotationUtils::annotationGeometry( annotation, page->width(), page->height() );
    Okular::NormalizedRect nr( rect, (int)page->width(), (int)page->height() );
    // set the viewport parameters
    Okular::DocumentViewport vp;
    vp.pageNumber = pageNumber;
    vp.rePos.enabled = true;
    vp.rePos.pos = Okular::DocumentViewport::Center;
    vp.rePos.normalizedX = ( nr.right + nr.left ) / 2.0;
    vp.rePos.normalizedY = ( nr.bottom + nr.top ) / 2.0;
    // setting the viewport
    m_document->setViewport( vp, 0, true );
}

QModelIndexList Reviews::retrieveAnnotations(const QModelIndex& idx) const
{
    QModelIndexList ret;
    if ( idx.isValid() )
    {
        if ( idx.model()->hasChildren( idx ) )
        {
            int rowCount = idx.model()->rowCount( idx );
            for ( int i = 0; i < rowCount; i++ )
            {
                ret += retrieveAnnotations( idx.child( i, idx.column() ) );
            }
        }
        else
        {
            ret += idx;
        }
    }
    
    return ret;
}

void Reviews::contextMenuRequested( const QPoint &pos )
{
    AnnotationPopup popup( m_document, AnnotationPopup::SingleAnnotationMode, this );
    connect( &popup, SIGNAL(openAnnotationWindow(Okular::Annotation*,int)),
             this, SIGNAL(openAnnotationWindow(Okular::Annotation*,int)) );

    QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();
    Q_FOREACH ( const QModelIndex &index, indexes )
    {
        QModelIndexList annotations = retrieveAnnotations(index);
        Q_FOREACH ( const QModelIndex &idx, annotations )
        {
            const QModelIndex authorIndex = m_authorProxy->mapToSource( idx );
            const QModelIndex filterIndex = m_groupProxy->mapToSource( authorIndex );
            const QModelIndex annotIndex = m_filterProxy->mapToSource( filterIndex );
            Okular::Annotation *annotation = m_model->annotationForIndex( annotIndex );
            if ( annotation )
            {
                const int pageNumber = m_model->data( annotIndex, AnnotationModel::PageRole ).toInt();
                popup.addAnnotation( annotation, pageNumber );
            }
        }
    }

    popup.exec( m_view->viewport()->mapToGlobal( pos ) );
}

void Reviews::saveSearchOptions()
{
    Okular::Settings::setReviewsSearchRegularExpression( m_searchLine->regularExpression() );
    Okular::Settings::setReviewsSearchCaseSensitive( m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false );
    Okular::Settings::self()->writeConfig();
}

#include "side_reviews.moc"
