/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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

// local includes
#include "core/document.h"
#include "core/page.h"
#include "core/annotations.h"
#include "annotationpropertiesdialog.h"


AnnotsPropertiesDialog::AnnotsPropertiesDialog(QWidget *parent,Annotation* ann)
    : KPageDialog( parent ), modified( false )
{
    setFaceType( Tabbed );
    m_annot=ann;
    setCaptionTextbyAnnotType();
    setButtons( Ok | Apply | Cancel );
    connect( this, SIGNAL( applyClicked() ), this, SLOT( slotapply() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotapply() ) );


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
    colorBn->setColor( ann->style.color );
    tmplabel->setBuddy( colorBn );
    hlay->addWidget( colorBn );

    hlay = new QHBoxLayout();
    lay->addLayout( hlay );
    tmplabel = new QLabel( i18n( "&Opacity:" ), page );
    hlay->addWidget( tmplabel );
    m_opacity = new KIntNumInput( page );
    m_opacity->setRange( 0, 100, 1, true );
    m_opacity->setValue( (int)( ann->style.opacity * 100 ) );
    tmplabel->setBuddy( m_opacity );
    hlay->addWidget( m_opacity );

    lay->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::Expanding ) );
    //END tab1
    
    //BEGIN tab 2
    page = new QFrame();
    addPage( page, i18n( "&General" ) );
//    m_tabitem[1]->setIcon( KIcon( "fonts" ) );
    QGridLayout * gridlayout = new QGridLayout( page );
    tmplabel = new QLabel( i18n( "&Author:" ), page );
    AuthorEdit = new QLineEdit( ann->author, page );
    tmplabel->setBuddy( AuthorEdit );
    gridlayout->addWidget( tmplabel, 0, 0 );
    gridlayout->addWidget( AuthorEdit, 0, 1 );
    
    tmplabel = new QLabel( i18n( "Created:" ), page );
    gridlayout->addWidget( tmplabel, 1, 0 );
    tmplabel = new QLabel( KGlobal::locale()->formatDateTime( ann->creationDate, false, true ), page );//time
    gridlayout->addWidget( tmplabel, 1, 1 );
    
    tmplabel = new QLabel( i18n( "Modified:" ), page );
    gridlayout->addWidget( tmplabel, 2, 0 );
    tmplabel = new QLabel( KGlobal::locale()->formatDateTime( ann->modifyDate, false, true ), page );//time
    gridlayout->addWidget( tmplabel, 2, 1 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::Expanding ), 3, 0 );
    //END tab 2
    //BEGIN advance properties:
    page = new QFrame();
    addPage( page, i18n( "&Advanced" ) );
    gridlayout = new QGridLayout( page );
    
    tmplabel = new QLabel( i18n( "uniqueName:" ), page );
    gridlayout->addWidget( tmplabel, 0, 0 );
    uniqueNameEdit = new QLineEdit( ann->uniqueName, page );
    gridlayout->addWidget( uniqueNameEdit, 0, 1 );
    
    tmplabel = new QLabel( i18n( "contents:" ), page );
    gridlayout->addWidget( tmplabel, 1, 0 );
    contentsEdit = new QLineEdit( ann->contents, page );
    gridlayout->addWidget( contentsEdit, 1, 1 );

    tmplabel = new QLabel( i18n( "flags:" ), page );
    gridlayout->addWidget( tmplabel, 2, 0 );
    flagsEdit = new QLineEdit( QString::number( m_annot->flags ), page );
    gridlayout->addWidget( flagsEdit, 2, 1 );

    QString tmpstr = QString( "%1,%2,%3,%4" ).arg( m_annot->boundary.left ).arg( m_annot->boundary.top ).arg( m_annot->boundary.right ).arg( m_annot->boundary.bottom );
    tmplabel = new QLabel( i18n( "boundary:" ), page );
    gridlayout->addWidget( tmplabel, 3, 0 );
    boundaryEdit = new QLineEdit( tmpstr, page );
    gridlayout->addWidget( boundaryEdit, 3, 1 );

    gridlayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::Expanding ), 4, 0 );
    //END advance

    resize( sizeHint() );
}
AnnotsPropertiesDialog::~AnnotsPropertiesDialog()
{
}


void AnnotsPropertiesDialog::setCaptionTextbyAnnotType()
{
    Annotation::SubType type=m_annot->subType();
    QString captiontext;
    switch(type)
    {
        case Annotation::AText:
            if(((TextAnnotation*)m_annot)->textType==TextAnnotation::Linked)
                captiontext = i18n( "Note Properties" );
            else
                captiontext = i18n( "FreeText Properties" );
            break;
        case Annotation::ALine:
            captiontext = i18n( "Line Properties" );
            break;
        case Annotation::AGeom:
            captiontext = i18n( "Geom Properties" );
            break;
        case Annotation::AHighlight:
            captiontext = i18n( "Highlight Properties" );
            break;
        case Annotation::AStamp:
            captiontext = i18n( "Stamp Properties" );
            break;
        case Annotation::AInk:
            captiontext = i18n( "Ink Properties" );
            break;
        default:
            captiontext = i18n( "Annotation Properties" );
            break;
    }
        setCaption( captiontext );
}

void AnnotsPropertiesDialog::slotapply()
{
    m_annot->author=AuthorEdit->text();
    m_annot->contents=contentsEdit->text();
    m_annot->style.color = colorBn->color();
    m_annot->modifyDate=QDateTime::currentDateTime();
    m_annot->flags=flagsEdit->text().toInt();
    m_annot->style.opacity = (double)m_opacity->value() / 100.0;
}
    
#include "annotationpropertiesdialog.moc"
    
