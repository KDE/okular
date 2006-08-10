
/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qlayout.h>
#include <qlabel.h>
#include <qheaderview.h>
#include <qsortfilterproxymodel.h>
#include <QColorDialog>
#include <kicon.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kglobalsettings.h>

// local includes
#include "core/document.h"
#include "core/page.h"
#include "core/annotations.h"
#include "annotationpropertiesdialog.h"


AnnotsPropertiesDialog::AnnotsPropertiesDialog(QWidget *parent,Annotation* ann)
    : KPageDialog( parent ), modified( false )
{
    setFaceType( Tabbed );
    resize(400,300);
    m_annot=ann;
    setCaptionTextbyAnnotType();
    setButtons( Ok | Apply | Cancel );
    connect( this, SIGNAL( applyClicked() ), this, SLOT( slotapply() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotapply() ) );


    QLabel* tmplabel;
  //1. Appearance
    //BEGIN tab1
    m_page[0] = new QFrame();
    m_tabitem[0] = addPage( m_page[0], i18n( "&Appearance" ) );
    m_layout[0] = new QGridLayout( m_page[0] );
    m_layout[0]->setMargin( marginHint() );
    m_layout[0]->setSpacing( spacingHint() );

    colorBn = new QPushButton(m_page[0]);
    
    m_selcol=ann->style.color;
    QPalette pal = colorBn->palette();
    pal.setColor( QPalette::Active, QPalette::Button, m_selcol );
    pal.setColor( QPalette::Inactive, QPalette::Button, m_selcol );
    pal.setColor( QPalette::Disabled, QPalette::Button, m_selcol );
    colorBn->setPalette( pal );
    colorBn->setText(i18n( "&Color" ));
    m_layout[0]->addWidget( colorBn, 0, 0, Qt::AlignRight );
    
    QObject::connect(colorBn, SIGNAL(clicked()), this, SLOT(slotChooseColor()));
    
    tmplabel = new QLabel( i18n( "Opacity" ), m_page[0] );
    QString szopacity;
    szopacity.setNum( int(ann->style.opacity*100),10);
    opacityEdit = new QLineEdit(szopacity,m_page[0]);
    m_layout[0]->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    m_layout[0]->addWidget( opacityEdit, 1, 1 );
    
    opacitySlider=new QSlider(m_page[0]);
    opacitySlider->setMaximum(100);
    opacitySlider->setValue(100);
    opacitySlider->setSliderPosition(100);
    opacitySlider->setOrientation(Qt::Horizontal);
    m_layout[0]->addWidget( opacitySlider, 2, 1 );
    //END tab1
    
    //BEGIN tab 2
    m_page[1] = new QFrame();
    m_tabitem[1] = addPage(m_page[1], i18n("&General"));
//    m_tabitem[1]->setIcon( KIcon( "fonts" ) );
    m_layout[1] = new QGridLayout(m_page[1]);
    m_layout[1]->setMargin(marginHint());
    m_layout[1]->setSpacing(spacingHint());
    tmplabel = new QLabel( i18n( "Author" ), m_page[1] );
    AuthorEdit= new QLineEdit(ann->author,m_page[1]);
    m_layout[1]->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    m_layout[1]->addWidget( AuthorEdit, 0, 1 );
    
    tmplabel = new QLabel( i18n( "Created:" ), m_page[1] );
    m_layout[1]->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    tmplabel = new QLabel(ann->creationDate.toString("hh:mm:ss, dd.MM.yyyy"), m_page[1] );//time
    m_layout[1]->addWidget( tmplabel, 1, 1 );
    
    tmplabel = new QLabel( i18n( "Modified:" ), m_page[1] );
        m_layout[1]->addWidget( tmplabel, 2, 0, Qt::AlignRight );
    tmplabel = new QLabel(ann->modifyDate.toString("hh:mm:ss, dd.MM.yyyy"), m_page[1] );//time
    m_layout[1]->addWidget( tmplabel, 2, 1 );
    //END tab 2
    //BEGIN advance properties:
    m_page[2] = new QFrame();
    m_tabitem[2] = addPage(m_page[2], i18n("&Advance"));
    m_layout[2] = new QGridLayout(m_page[2]);
    m_layout[2]->setMargin(marginHint());
    m_layout[2]->setSpacing(spacingHint());
    
    tmplabel = new QLabel( i18n( "uniqueName:" ), m_page[2] );
    m_layout[2]->addWidget( tmplabel, 0, 0 );
    uniqueNameEdit = new QLineEdit( ann->uniqueName, m_page[2] );
    m_layout[2]->addWidget( uniqueNameEdit, 0, 1 );
    
    tmplabel = new QLabel( i18n( "contents:" ), m_page[2] );
    m_layout[2]->addWidget( tmplabel, 1, 0 );
    contentsEdit = new QLineEdit( ann->contents, m_page[2] );
    m_layout[2]->addWidget( contentsEdit, 1, 1 );

    QString tmpstr;
    tmpstr.setNum(m_annot->flags);
    tmplabel = new QLabel( i18n( "flags:" ), m_page[2] );
    m_layout[2]->addWidget( tmplabel, 2, 0 );
    flagsEdit = new QLineEdit( tmpstr, m_page[2] );
    m_layout[2]->addWidget( flagsEdit, 2, 1 );

    QTextStream(&tmpstr)<<m_annot->boundary.left<<","<<m_annot->boundary.top
            <<","<<m_annot->boundary.right<<","<<m_annot->boundary.bottom;
    tmplabel = new QLabel( i18n( "boundary:" ), m_page[2] );
    m_layout[2]->addWidget( tmplabel, 3, 0 );
    boundaryEdit = new QLineEdit( tmpstr, m_page[2] );
    m_layout[2]->addWidget( boundaryEdit, 3, 1 );
    //END advance
    
    
    
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
                captiontext="Note Properties";
            else
                captiontext="FreeText Properties";
            break;
        case Annotation::ALine:
            captiontext="Line Properties";
            break;
        case Annotation::AGeom:
            captiontext="Geom Properties";
            break;
        case Annotation::AHighlight:
            captiontext="Highlight Properties";
            break;
        case Annotation::AStamp:
            captiontext="Stamp Properties";
            break;
        case Annotation::AInk:
            captiontext="Ink Properties";
            break;
        default:
            captiontext="Base Properties";
            break;
    }
        setCaption( captiontext );
}
void AnnotsPropertiesDialog::slotChooseColor()
{
    
    QColor col = QColorDialog::getColor(m_selcol, this);
    if (!col.isValid())
        return;
    m_selcol=col;
    QPalette pal = colorBn->palette();
    pal.setColor( QPalette::Active, QPalette::Button, m_selcol );
    pal.setColor( QPalette::Inactive, QPalette::Button, m_selcol );
    pal.setColor( QPalette::Disabled, QPalette::Button, m_selcol );
    colorBn->setPalette( pal );
}
void AnnotsPropertiesDialog::slotapply()
{
    m_annot->author=AuthorEdit->text();
    m_annot->contents=contentsEdit->text();
    m_annot->style.color=m_selcol;
    m_annot->modifyDate=QDateTime::currentDateTime();
    m_annot->flags=flagsEdit->text().toInt();
}
    
#include "annotationpropertiesdialog.moc"
    
