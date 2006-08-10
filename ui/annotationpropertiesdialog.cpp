
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


AnnotsPropertiesDialog::AnnotsPropertiesDialog(QWidget *parent, KPDFDocument *doc, Annotation* ann)
    : KPageDialog( parent ), modified( false )
{
    setFaceType( Tabbed );
    m_annot=ann;
    setCaptionTextbyAnnotType();
    setButtons( Ok | Apply | Cancel );

    QLabel* tmplabel;
  //1. Appearance
    //BEGIN tab1
    m_page[0] = new QFrame();
    m_tabitem[0] = addPage( m_page[0], i18n( "&Appearance" ) );
    m_layout[0] = new QGridLayout( m_page[0] );
    m_layout[0]->setMargin( marginHint() );
    m_layout[0]->setSpacing( spacingHint() );

    colorBn = new QPushButton(m_page[0]);
    QPalette pal = colorBn->palette();
    pal.setColor( QPalette::Active, QPalette::Button, ann->style.color );
    pal.setColor( QPalette::Inactive, QPalette::Button, ann->style.color );
    pal.setColor( QPalette::Disabled, QPalette::Button, ann->style.color );
    colorBn->setPalette( pal );
    //colorBn->setText(i18n( "&Color" ));
 //   QString colorsz;
 //   QTextStream(&colorsz) << "(" << ann->style.color.red()<<", "
 //         << ann->style.color.green()<<", "<< ann->style.color.blue()<<")";
    tmplabel = new QLabel( ann->style.color.name(), m_page[0] );
    m_layout[0]->addWidget( colorBn, 0, 0, Qt::AlignRight );
    m_layout[0]->addWidget( tmplabel, 0, 1 );
    
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
    
    //END advance
    
    
    QObject::connect(colorBn, SIGNAL(clicked()), this, SLOT(slotChooseColor()));
    
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
    
    QColor col = QColorDialog::getColor(m_annot->style.color, this);
    if (!col.isValid())
        return;
    m_annot->style.color=col;
}
    
#include "annotationpropertiesdialog.moc"
    
