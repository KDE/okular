#include "layers.h"
#include "settings.h"

// qt/kde includes
#include <QVBoxLayout>
#include <QTreeView>
#include <qheaderview.h>

// local includes
#include "core/document.h"
#include "ktreeviewsearchline.h"

Layers::Layers(QWidget *parent, Okular::Document *document) : QWidget(parent), m_document(document)
{
    QVBoxLayout * const mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 6 );

    m_document->addObserver( this );

    m_searchLine = new KTreeViewSearchLine( this );
    mainlay->addWidget( m_searchLine );
    m_searchLine->setCaseSensitivity( Okular::Settings::self()->contentsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::self()->contentsSearchRegularExpression() );
    connect( m_searchLine, SIGNAL(searchOptionsChanged()), this, SLOT(saveSearchOptions()) );

    m_treeView = new QTreeView( this );
    mainlay->addWidget( m_treeView );

    QAbstractItemModel * layersModel = m_document->layersModel();

    if( layersModel )
    {
	m_treeView->setModel( layersModel );
	m_searchLine->addTreeView( m_treeView );
	emit hasLayers( true );
	connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_document, SLOT(reloadDocument()) );
    }
    else
    {
	emit hasLayers( false );
    }
    m_treeView->setSortingEnabled( false );
    m_treeView->setRootIsDecorated( true );
    m_treeView->setAlternatingRowColors( true );
    m_treeView->header()->hide();
}

Layers::~Layers()
{
    m_document->removeObserver( this );
}

void Layers::notifySetup( const QVector< Okular::Page * > & /*pages*/, int /*setupFlags*/ )
{
    QAbstractItemModel * layersModel = m_document->layersModel();
    if( layersModel )
    {
	m_treeView->setModel( layersModel );
	m_searchLine->addTreeView( m_treeView );
	emit hasLayers( true );
	connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_document, SLOT(reloadDocument()) );
    }
    else
    {
	emit hasLayers( false );
    }
}

void Layers::saveSearchOptions()
{
    Okular::Settings::setContentsSearchRegularExpression( m_searchLine->regularExpression() );
    Okular::Settings::setContentsSearchCaseSensitive( m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false );
    Okular::Settings::self()->writeConfig();
}

 #include "layers.moc"
