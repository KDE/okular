/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationpropertiesdialog.h"

// qt/kde includes
#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qheaderview.h>
#include <kcolorbutton.h>
#include <kicon.h>
#include <klocale.h>
#include <knuminput.h>
#include <kglobal.h>

// local includes
#include "core/document.h"
#include "core/page.h"
#include "core/annotations.h"
#include "annotationwidgets.h"


AnnotsPropertiesDialog::AnnotsPropertiesDialog( QWidget *parent, Okular::Document *document, int docpage, Okular::Annotation *ann )
    : KPageDialog( parent ), m_document( document ), m_page( docpage ), modified( false )
{
    setFaceType( Tabbed );
    m_annot=ann;
    setCaptionTextbyAnnotType();
    setButtons( Ok | Apply | Cancel );
    enableButton( Apply, false );
    connect( this, SIGNAL( applyClicked() ), this, SLOT( slotapply() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotapply() ) );

    m_annotWidget = AnnotationWidgetFactory::widgetFor( ann );

    QLabel* tmplabel;
  //1. Appearance
    //BEGIN tab1
    QFrame *page = new QFrame();
    addPage( page, i18n( "&Appearance" ) );
    QVBoxLayout * lay = new QVBoxLayout( page );

    QHBoxLayout * hlay = new QHBoxLayout();
    lay->addLayout( hlay );
    tmplabel = new QLabel( i18n( "&Color:" ), page );
    hlay->addWidget( tmplabel );
    colorBn = new KColorButton( page );
    colorBn->setColor( ann->style().color() );
    tmplabel->setBuddy( colorBn );
    hlay->addWidget( colorBn );

    hlay = new QHBoxLayout();
    lay->addLayout( hlay );
    tmplabel = new QLabel( i18n( "&Opacity:" ), page );
    hlay->addWidget( tmplabel );
    m_opacity = new KIntNumInput( page );
    m_opacity->setRange( 0, 100, 1, true );
    m_opacity->setValue( (int)( ann->style().opacity() * 100 ) );
    tmplabel->setBuddy( m_opacity );
    hlay->addWidget( m_opacity );

    if ( m_annotWidget )
        lay->addWidget( m_annotWidget->widget() );

    lay->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::Expanding ) );
    //END tab1
    
    //BEGIN tab 2
    page = new QFrame();
    addPage( page, i18n( "&General" ) );
//    m_tabitem[1]->setIcon( KIcon( "fonts" ) );
    QGridLayout * gridlayout = new QGridLayout( page );
    tmplabel = new QLabel( i18n( "&Author:" ), page );
    AuthorEdit = new QLineEdit( ann->author(), page );
    tmplabel->setBuddy( AuthorEdit );
    gridlayout->addWidget( tmplabel, 0, 0 );
    gridlayout->addWidget( AuthorEdit, 0, 1 );
    
    tmplabel = new QLabel( i18n( "Created:" ), page );
    gridlayout->addWidget( tmplabel, 1, 0 );
    tmplabel = new QLabel( KGlobal::locale()->formatDateTime( ann->creationDate(), KLocale::LongDate, true ), page );//time
    gridlayout->addWidget( tmplabel, 1, 1 );
    
    tmplabel = new QLabel( i18n( "Modified:" ), page );
    gridlayout->addWidget( tmplabel, 2, 0 );
    m_modifyDateLabel = new QLabel( KGlobal::locale()->formatDateTime( ann->modificationDate(), KLocale::LongDate, true ), page );//time
    gridlayout->addWidget( m_modifyDateLabel, 2, 1 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::Expanding ), 3, 0 );
    //END tab 2
    //BEGIN advance properties:
    page = new QFrame();
    addPage( page, i18n( "&Advanced" ) );
    gridlayout = new QGridLayout( page );
    
    tmplabel = new QLabel( i18n( "contents:" ), page );
    gridlayout->addWidget( tmplabel, 1, 0 );
    contentsEdit = new QLineEdit( ann->contents(), page );
    gridlayout->addWidget( contentsEdit, 1, 1 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::Expanding ), 4, 0 );
    //END advance

    //BEGIN connections
    connect( colorBn, SIGNAL( changed( const QColor& ) ), this, SLOT( setModified() ) );
    connect( m_opacity, SIGNAL( valueChanged( int ) ), this, SLOT( setModified() ) );
    connect( AuthorEdit, SIGNAL( textChanged ( const QString& ) ), this, SLOT( setModified() ) );
    connect( contentsEdit, SIGNAL( textChanged ( const QString& ) ), this, SLOT( setModified() ) );
    if ( m_annotWidget )
    {
        connect( m_annotWidget, SIGNAL( dataChanged() ), this, SLOT( setModified() ) );
    }
    //END

#if 0
    kDebug() << "Annotation details:" << endl;
    kDebug() << " => unique name: '" << ann->uniqueName() << endl;
    kDebug() << " => flags: '" << QString::number( m_annot->flags(), 2 ) << endl;
#endif

    resize( sizeHint() );
}
AnnotsPropertiesDialog::~AnnotsPropertiesDialog()
{
    delete m_annotWidget;
}


void AnnotsPropertiesDialog::setCaptionTextbyAnnotType()
{
    Okular::Annotation::SubType type=m_annot->subType();
    QString captiontext;
    switch(type)
    {
        case Okular::Annotation::AText:
            if(((Okular::TextAnnotation*)m_annot)->textType()==Okular::TextAnnotation::Linked)
                captiontext = i18n( "Note Properties" );
            else
                captiontext = i18n( "FreeText Properties" );
            break;
        case Okular::Annotation::ALine:
            captiontext = i18n( "Line Properties" );
            break;
        case Okular::Annotation::AGeom:
            captiontext = i18n( "Geom Properties" );
            break;
        case Okular::Annotation::AHighlight:
            captiontext = i18n( "Highlight Properties" );
            break;
        case Okular::Annotation::AStamp:
            captiontext = i18n( "Stamp Properties" );
            break;
        case Okular::Annotation::AInk:
            captiontext = i18n( "Ink Properties" );
            break;
        default:
            captiontext = i18n( "Annotation Properties" );
            break;
    }
        setCaption( captiontext );
}

void AnnotsPropertiesDialog::setModified()
{
    modified = true;
    enableButton( Apply, true );
}

void AnnotsPropertiesDialog::slotapply()
{
    if ( !modified )
        return;

    m_annot->setAuthor( AuthorEdit->text() );
    m_annot->setContents( contentsEdit->text() );
    m_annot->setModificationDate( QDateTime::currentDateTime() );
    m_annot->style().setColor( colorBn->color() );
    m_annot->style().setOpacity( (double)m_opacity->value() / 100.0 );

    if ( m_annotWidget )
        m_annotWidget->applyChanges();

    m_document->modifyPageAnnotation( m_page, m_annot );

    m_modifyDateLabel->setText( KGlobal::locale()->formatDateTime( m_annot->modificationDate(), KLocale::LongDate, true ) );

    modified = false;
    enableButton( Apply, false );
}

#include "annotationpropertiesdialog.moc"
    
