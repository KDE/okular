#include "part.h"

#include <qlayout.h>
#include <qtable.h>

#include "kpdf_pagewidget.h"
#include "thumbnaillist.h"

/*
 *  Constructs a PDFPartView as a child of 'parent', with the
 *  name 'name' 
 */
PDFPartView::PDFPartView(QWidget* parent, const char* name) : QWidget(parent, name)
{
    PDFPartViewLayout = new QHBoxLayout( this, 11, 6, "PDFPartViewLayout"); 

    pagesList = new ThumbnailList(this);
    pagesList->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)7, 0, 0, pagesList->sizePolicy().hasHeightForWidth() ) );
    pagesList->setMaximumSize( QSize( 75, 32767 ) );
    pagesList->setColumnWidth(0, 75);
    PDFPartViewLayout->addWidget( pagesList );

    outputdev = new KPDF::PageWidget( this, "outputdev" );
    PDFPartViewLayout->addWidget( outputdev );
    resize( QSize(623, 381).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
    
    connect(pagesList, SIGNAL(clicked(int)), this, SIGNAL(clicked(int)));
}

/*
 *  Destroys the object and frees any allocated resources
 */
PDFPartView::~PDFPartView()
{
    // no need to delete child widgets, Qt does it all for us
}

void PDFPartView::setCurrentItem(int i)
{
    pagesList->setCurrentItem(i);
}

int PDFPartView::getCurrentItem() const
{
    return pagesList->getCurrentItem();
}

void PDFPartView::setPages(int i, double ar)
{
    pagesList->setPages(i, ar);
}

void PDFPartView::setThumbnail(int i, const QPixmap *thumbnail)
{
    pagesList->setThumbnail(i, thumbnail);
}

void PDFPartView::showPageList(bool show)
{
    if (show) pagesList->show();
    else pagesList->hide();
}

#include "part.moc"
