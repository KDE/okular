/***********************************************************************
 *
 *  prefs.cpp
 *
 **********************************************************************/

#include <qapp.h>
#include <qtabdlg.h>
#include <qmlined.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qlabel.h>
#include <qcombo.h>
#include <qlayout.h>
#include <stdio.h>
#include <qstring.h>
#include <qfont.h>
#include <qtooltip.h>
#include <qlined.h>
#include <qchkbox.h>
#include <qpushbt.h>
#include <qfiledlg.h> 
#include <qdir.h>
#include <qregexp.h> 
#include <qdatetm.h> 
#include <qmsgbox.h> 
#include <qlistbox.h> 
#include <qstrlist.h>

#include <stdlib.h>
#include <string.h>

#include <klocale.h>

#include "prefs.h"

kdviprefs::kdviprefs( QWidget *, const char * )
	: QTabDialog() 
{
	resize(400,330);
	insertPages();

	setApplyButton(i18n("Apply"));
	setCancelButton(i18n("Cancel"));
	setCaption(i18n("Preferences"));

	this->setMinimumSize(400,330);
	this->setMaximumSize(400,330);
 
	connect(this,SIGNAL(applyButtonPressed()),SLOT(applyChanges()));
 
}

static const char *paper_names[] = { 
	"US",
	"US landscape",
	"Legal",
	"foolscap",

	/* ISO `A' formats, Portrait */
	"A1",
	"A2",
	"A3",
	"A4",
	"A5",
	"A6",
	"A7",

	/* ISO `A' formats, Landscape */
	"A1 landscape",
	"A2 landscape",
	"A3 landscape",
	"A4 landscape",
	"A5 landscape",
	"A6 landscape",
	"A7 landscape",

	/* ISO `B' formats, Portrait */
	"B1",
	"B2",
	"B3",
	"B4",
	"B5",
	"B6",
	"B7",

	/* ISO `B' formats, Landscape */
	"B1 landscape",
	"B2 landscape",
	"B3 landscape",
	"B4 landscape",
	"B5 landscape",
	"B6 landscape",
	"B7 landscape",

	/* ISO `C' formats, Portrait */
	"C1",
	"C2",
	"C3",
	"C4",
	"C5",
	"C6",
	"C7",

	/* ISO `C' formats, Landscape */
	"C1 landscape",
	"C2 landscape",
	"C3 landscape",
	"C4 landscape",
	"C5 landscape",
	"C6 landscape",
	"C7 landscape",
	0
};
static const char *papers[] = { 
	"us",		"8.5x11",
	"usr",		"11x8.5",
	"legal",	"8.5x14",
	"foolscap",	"13.5x17.0",	/* ??? */

	/* ISO `A' formats, Portrait */
	"a1",		"59.4x84.0cm",
	"a2",		"42.0x59.4cm",
	"a3",		"29.7x42.0cm",
	"a4",		"21.0x29.7cm",
	"a5",		"14.85x21.0cm",
	"a6",		"10.5x14.85cm",
	"a7",		"7.42x10.5cm",

	/* ISO `A' formats, Landscape */
	"a1r",		"84.0x59.4cm",
	"a2r",		"59.4x42.0cm",
	"a3r",		"42.0x29.7cm",
	"a4r",		"29.7x21.0cm",
	"a5r",		"21.0x14.85cm",
	"a6r",		"14.85x10.5cm",
	"a7r",		"10.5x7.42cm",

	/* ISO `B' formats, Portrait */
	"b1",		"70.6x100.0cm",
	"b2",		"50.0x70.6cm",
	"b3",		"35.3x50.0cm",
	"b4",		"25.0x35.3cm",
	"b5",		"17.6x25.0cm",
	"b6",		"13.5x17.6cm",
	"b7",		"8.8x13.5cm",

	/* ISO `B' formats, Landscape */
	"b1r",		"100.0x70.6cm",
	"b2r",		"70.6x50.0cm",
	"b3r",		"50.0x35.3cm",
	"b4r",		"35.3x25.0cm",
	"b5r",		"25.0x17.6cm",
	"b6r",		"17.6x13.5cm",
	"b7r",		"13.5x8.8cm",

	/* ISO `C' formats, Portrait */
	"c1",		"64.8x91.6cm",
	"c2",		"45.8x64.8cm",
	"c3",		"32.4x45.8cm",
	"c4",		"22.9x32.4cm",
	"c5",		"16.2x22.9cm",
	"c6",		"11.46x16.2cm",
	"c7",		"8.1x11.46cm",

	/* ISO `C' formats, Landscape */
	"c1r",		"91.6x64.8cm",
	"c2r",		"64.8x45.8cm",
	"c3r",		"45.8x32.4cm",
	"c4r",		"32.4x22.9cm",
	"c5r",		"22.9x16.2cm",
	"c6r",		"16.2x11.46cm",
	"c7r",		"11.46x8.1cm",
	0
};

bool kdviprefs::paperSizes( const char *p, float &w, float &h )
{
	QString s(p);
	s = s.simplifyWhiteSpace();
	
	for ( unsigned i = 0; i< sizeof(paper_names)/sizeof(*paper_names)-1; i++ )
		if ( s == paper_names[i] )
		{
			s = papers[ 2*i + 1 ];
			int cm = s.findRev( "cm" );
			w = s.toFloat();
			int ind = s.find( 'x' );
			if ( ind<0 )
				return 0;
			s = s.mid( ind + 1, 9999 );
			h = s.toFloat();
			if ( cm >= 0 )
				w /= 2.54, h /= 2.54;
			return 1;
		}
	return 0;
}

void kdviprefs::insertPages()
{
	QSize sz( 60, fontMetrics().lineSpacing() + 10 );
	int row;
	QLabel *l;

	setFocusPolicy(QWidget::StrongFocus);	

    // First page of tab preferences dialog

	pages[0]= new QWidget( this, "page1" );

	QButtonGroup *bg	= new QButtonGroup( "", pages[0] );
	menu	= new QCheckBox( i18n("Menu Bar"), bg );
	button	= new QCheckBox( i18n("Button Bar"), bg );
	status	= new QCheckBox( i18n("Status Bar"), bg );
	pagelist= new QCheckBox( i18n("Page List"), bg );
	scrollb	= new QCheckBox( i18n("Scroll Bars"), bg );
	recent	= new QLineEdit( bg );

	KConfig *config = kapp->getConfig();
	config->setGroup("kdvi");

	menu->setChecked( !config->readNumEntry( "HideMenubar" ) );
	button->setChecked( !config->readNumEntry( "HideButtons" ) );
	pagelist->setChecked( config->readNumEntry( "VertToolbar" ) );
	status->setChecked( !config->readNumEntry( "HideStatusbar" ) );
	scrollb->setChecked( !config->readNumEntry( "HideScrollbars" ) );
	config->setGroup("RecentFiles");
	recent->setText( config->readEntry( "MaxCount" ) );
	config->setGroup("kdvi");

	l10 = new QGridLayout( bg, 9, 2, 25, 5 );
	row = 0;
	menu->setMinimumSize( menu->sizeHint() );	l10->addWidget( menu, row++, 0 );
	button->setMinimumSize( button->sizeHint() );	l10->addWidget( button, row++, 0 );
	status->setMinimumSize( status->sizeHint() );	l10->addWidget( status, row++, 0 );
	pagelist->setMinimumSize( pagelist->sizeHint() );l10->addWidget( pagelist, row++, 0 );
	scrollb->setMinimumSize( scrollb->sizeHint() );	l10->addWidget( scrollb, row++, 0 );
	l = new QLabel( i18n("Number of recent files"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );	l10->addWidget( l, row, 0 );
	recent->setMinimumSize( sz );	l10->addWidget( recent, row++, 1 );
	l10->setColStretch ( 0, 3 );
	l10->setRowStretch ( row, 6 );
	l10->activate();

	l11 = new QBoxLayout( pages[0], QBoxLayout::Down, 5 );
	l11->addWidget( bg );
	l11->activate();

	addTab(pages[0],i18n("Appearance"));

    // Second page of tab preferences dialog

	pages[1]= new QWidget( this, "page2" );

 	bg	= new QButtonGroup( "", pages[1] );


	pk	= new QCheckBox( i18n("Generate missing fonts"), bg );
	dpi	= new QLineEdit( bg );
	mode	= new QLineEdit( bg );
	fontdir	= new QLineEdit( bg );
	small	= new QLineEdit( bg );
	large	= new QLineEdit( bg );

	dpi->setText( config->readEntry( "BaseResolution" ) );
	mode->setText( config->readEntry( "MetafontMode" ) );
	pk->setChecked( config->readNumEntry( "MakePK" ) );
	fontdir->setText( config->readEntry( "FontPath" ) );
	small->setText( config->readEntry( "SmallShrink" ) );
	large->setText( config->readEntry( "LargeShrink" ) );
	
	l20 = new QGridLayout( bg, 9, 2, 25, 5 );
	row = 0;

	l = new QLabel( i18n("Resolution"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );		l20->addWidget( l, row, 0 );
	dpi->setMinimumSize( sz );		l20->addWidget( dpi, row++, 1 );

	l = new QLabel( i18n("Metafont mode"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );		l20->addWidget( l, row, 0 );
	mode->setMinimumSize( sz );		l20->addWidget( mode, row++, 1 );

	pk->setMinimumSize( pk->sizeHint() );
	l20->setRowStretch ( row, 6 );
	l20->addWidget( pk, row, 0 ); row++;

	l = new QLabel( i18n("PK Font Path"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );		l20->addWidget( l, row, 0 );
	fontdir->setMinimumSize( sz );		l20->addWidget( fontdir, row++, 1 );

	QFrame *f = new QFrame( bg );
	f->setMinimumSize( 10, 4 );
	l20->addMultiCellWidget( f, row, row, 0, 1 ); row++;

	l = new QLabel( i18n("Shrink factor for small text"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );		l20->addWidget( l, row, 0 );
	small->setMinimumSize( sz );		l20->addWidget( small, row++, 1 );

	l = new QLabel( i18n("Shrink factor for large text"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );		l20->addWidget( l, row, 0 );
	large->setMinimumSize( sz );		l20->addWidget( large, row++, 1 );

//	l20->setColStretch ( 0, 3 );
	l20->setRowStretch ( row, 6 );
	l20->activate();

	l21 = new QBoxLayout( pages[1], QBoxLayout::Down, 5  );
	l21->addWidget( bg );
	l21->activate();

	addTab( pages[1], i18n("Fonts") );

    // Third page of tab preferences dialog

	pages[2]= new QWidget( this, "page3");

 	bg	= new QButtonGroup( "", pages[2] );
	ps	= new QCheckBox( i18n("Show postscript specials"), bg );
	psaa	= new QCheckBox( i18n("Antialiased postcript"), bg );
	gamma	= new QLineEdit( bg );

	ps->setChecked( config->readNumEntry( "ShowPS" ) );
	ps->setMinimumSize( ps->sizeHint() );

	psaa->setChecked( config->readNumEntry( "PS Anti Alias", 1 ) );
	psaa->setMinimumSize( psaa->sizeHint() );

	gamma->setText( config->readEntry( "Gamma" ) );
	gamma->setMinimumSize( QSize( 60, fontMetrics().lineSpacing() + 10 ) );

	l30	= new QGridLayout( bg, 4, 2, 25, 15 );
	row	= 0;

	l30->addWidget( ps, row++, 0 );
	l30->addWidget( psaa, row++, 0 );
	
	l = new QLabel( i18n("Gamma"), bg );
	l->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	l->setMinimumSize( sz );		l30->addWidget( l, row, 0 );

	l30->addWidget( gamma, row++, 1 );
	l30->setColStretch ( 0, 3 );
	l30->setRowStretch ( row, 6 );
	l30->activate();

	l31	= new QBoxLayout( pages[2], QBoxLayout::Down, 5 );
	l31->addWidget( bg );
	l31->activate();

	addTab( pages[2], i18n("Rendering") );

    // Fourth page of tab preferences dialog

	pages[3] = new QWidget( this, "page4" );

	bg	= new QButtonGroup( "", pages[3] );
	paper	= new QComboBox( TRUE, bg );

	QString s = config->readEntry( "Paper" );
	for ( unsigned i = 0; i< sizeof(paper_names)/sizeof(*paper_names)-1; i++ )
		if ( s == papers[i*2] )
		{
			s = paper_names[ i ];
			break;
		}
	paper->insertItem( s );
	paper->insertStrList( paper_names );
	paper->setMinimumSize( paper->sizeHint() );

	l40	= new QBoxLayout( bg, QBoxLayout::Down, 50  );
	l40->addSpacing( fontMetrics().lineSpacing() );
	l40->addWidget( paper );
	l40->addStretch( 1 );
	l40->activate();

	l41	= new QBoxLayout( pages[3], QBoxLayout::Down, 5  );
	l41->addWidget( bg );
	l41->activate();

	addTab( pages[3], i18n("Paper") );

}

void kdviprefs::applyChanges()
{
	KConfig *config = kapp->getConfig();

	config->setGroup("kdvi");

	config->writeEntry( "HideMenubar", ! menu->isChecked() );
	config->writeEntry( "HideButtons", ! button->isChecked() );
	config->writeEntry( "HideStatusbar", ! status->isChecked() );
	config->writeEntry( "HideScrollbars", ! scrollb->isChecked() );
	config->writeEntry( "VertToolbar", pagelist->isChecked() );
	config->writeEntry( "MakePK", pk->isChecked() );
	config->writeEntry( "ShowPS", ps->isChecked() );
	config->writeEntry( "PS Anti Alias", psaa->isChecked() );
	config->writeEntry( "BaseResolution", dpi->text() );
	config->writeEntry( "MetafontMode", mode->text() );
	config->writeEntry( "FontPath", fontdir->text() );
	config->writeEntry( "SmallShrink", small->text() );
	config->writeEntry( "LargeShrink", large->text() );
	config->writeEntry( "Gamma", gamma->text() );
	QString s = paper->currentText();
	config->writeEntry( "Paper", s.simplifyWhiteSpace() );
	emit preferencesChanged();
}
