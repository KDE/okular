
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
#include <qtreeview.h>
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
    : KPageDialog( parent ), m_annot( ann )
{
    setFaceType( Tabbed );
    QString captiontext;
    if(ann->subType()==Annotation::AText)
    {
        if(((TextAnnotation*)ann)->textType==TextAnnotation::Linked)
            captiontext=i18n( "Note Properties" );
        else
            captiontext=i18n( "FreeText Properties" );
    }
    else if(ann->subType()==Annotation::AHighlight)
    {
        captiontext=i18n( "Highlight Properties" );
    }
    else if(ann->subType()==Annotation::AInk)
    {
        captiontext=i18n( "Ink Properties" );
    }
    else if(ann->subType()==Annotation::ALine)
    {
        captiontext=i18n( "Line Properties" );
    }
    else if(ann->subType()==Annotation::AStamp)
    {
        captiontext=i18n( "Properties" );
    }
    else
        captiontext=i18n( "Properties" );
    setCaption( captiontext );
    setButtons( Ok | Apply | Cancel );

  //1. Appearance
    QFrame *page = new QFrame();
    KPageWidgetItem *item = addPage( page, i18n( "&Appearance" ) );
    QGridLayout *layout = new QGridLayout( page );
    layout->setMargin( marginHint() );
    layout->setSpacing( spacingHint() );

    QLabel *key=0,*value =0;
        // create labels and layout them
   // key = new QLabel( i18n( "&Color" ), page );
    QPushButton* colorBn = new QPushButton(page);
    colorBn->setObjectName("ColorButton");
    colorBn->setText(i18n( "&Color" ));
    QString colorsz;
    QTextStream(&colorsz) << "(" << ann->style.color.red()<<", "
            << ann->style.color.green()<<", "<< ann->style.color.blue()<<")";
    value = new KSqueezedTextLabel( colorsz, page );
    layout->addWidget( colorBn, 0, 0, Qt::AlignRight );
    layout->addWidget( value, 0, 1 );
    
    key = new QLabel( i18n( "Opacity" ), page );
    QString szopacity;
    szopacity.setNum( int(ann->style.opacity*100),10);
    opacityEdit = new QLineEdit(szopacity,page);
    opacityEdit->setObjectName(QString::fromUtf8("opacityEdit"));
    //opacityEdit->setGeometry(QRect(150, 90, 113, 28));
    //value = new KSqueezedTextLabel( "KSqueezedTextLabel", page );
    layout->addWidget( key, 1, 0, Qt::AlignRight );
    layout->addWidget( opacityEdit, 1, 1 );
    
    opacitySlider=new QSlider(page);
    opacitySlider->setObjectName(QString::fromUtf8("opacitySlider"));
   // opacitySlider->setGeometry(QRect(110, 130, 160, 16));
    opacitySlider->setMaximum(100);
    opacitySlider->setValue(100);
    opacitySlider->setSliderPosition(100);
    opacitySlider->setOrientation(Qt::Horizontal);
    layout->addWidget( opacitySlider, 2, 0 );


    
    QLineEdit *AuthorEdit;
    Annotation* m_annot;
    
    QGridLayout *page2Layout = 0;
    QFrame *page2 = new QFrame();
    KPageWidgetItem *item2 = addPage(page2, i18n("&General"));
    item2->setIcon( KIcon( "fonts" ) );
    page2Layout = new QGridLayout(page2);
    page2Layout->setMargin(marginHint());
    page2Layout->setSpacing(spacingHint());
    // create labels and layout them
    key = new QLabel( i18n( "Author" ), page2 );
    AuthorEdit= new QLineEdit(page2);
    AuthorEdit->setObjectName(QString::fromUtf8("AuthorEdit"));
    AuthorEdit->setText(ann->author);
    page2Layout->addWidget( key, 0, 0, Qt::AlignRight );
    page2Layout->addWidget( AuthorEdit, 0, 1 );
    
    key = new QLabel( i18n( "Created" ), page2 );
    value = new KSqueezedTextLabel(ann->creationDate.toString("hh:mm:ss, dd.MM.yyyy"), page2 );//time
    page2Layout->addWidget( key, 1, 0, Qt::AlignRight );
    page2Layout->addWidget( value, 1, 1 );
    
    key = new QLabel( i18n( "Modified" ), page2 );
    value = new KSqueezedTextLabel(ann->modifyDate.toString("hh:mm:ss, dd.MM.yyyy"), page2 );//time
    page2Layout->addWidget( key, 2, 0, Qt::AlignRight );
    page2Layout->addWidget( value, 2, 1 );
    
    
    QObject::connect(colorBn, SIGNAL(clicked()), this, SLOT(slotChooseColor()));
    
}
void AnnotsPropertiesDialog::slotChooseColor()
{
    ;
}
    
#include "annotationpropertiesdialog.moc"
    
