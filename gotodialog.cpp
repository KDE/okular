/*
 *   kdvi: KDE dvi viewer
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Based on ealier dialog code 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// Add header files alphabetically

#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qvalidator.h>

#include <kapp.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "gotodialog.h"

GotoDialog::GotoDialog( QWidget *parent, const char *name, bool modal)
  :KDialogBase( parent, name, modal, i18n("Go to page"), Ok|Apply|Cancel, Ok, 
		true )
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayount = new QVBoxLayout( page, 0, spacingHint() );

  QLabel *label = new QLabel( i18n("Enter page number:"), page );
  topLayount->addWidget( label );

  mLineEdit = new QLineEdit( page );
  topLayount->addWidget( mLineEdit );
  mLineEdit->setMinimumWidth( fontMetrics().maxWidth()*15 );
  
  QIntValidator *validator = new QIntValidator(mLineEdit);
  validator->setBottom(0);
  mLineEdit->setValidator( validator );
}


void GotoDialog::slotOk()
{
  if( startGoto() )
  {
    accept();
  }
}


void GotoDialog::slotApply()
{
  startGoto();
}


bool GotoDialog::startGoto()
{
  QString text = mLineEdit->text().stripWhiteSpace(); 
  if( text.isEmpty() == true )
  {
    QString msg = i18n( "You must enter a page number first." );
    KMessageBox::sorry( this, msg );
    return false;
  }
 
  bool valid;
  text.toUInt( &valid );
  if( valid == false )
  {
    QString msg = i18n( "You must enter a valid number" );
    KMessageBox::sorry( this, msg );
    return false;
  }

  emit gotoPage( text );
  return true;
}

