/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_SEARCHWIDGET_H_
#define _KPDF_SEARCHWIDGET_H_

#include <qhbox.h>

class KActionCollection;
class KPopupMenu;
class KLineEdit;

class KPDFDocument;

/**
 * @short A search widget for find-as-you-type search.
 *
 * ...
 */
class SearchWidget : public QHBox
{
Q_OBJECT

public:
	SearchWidget( QWidget *parent, KPDFDocument *document );

	// create actions that interact with this widget
	void setupActions( KActionCollection * collection );
	void saveSettings();

private slots:
	void slotTextChanged( const QString & text );
	void slotChangeCase( int index );
	void slotToggleSearchBar( bool visible );

private:
	KPDFDocument * m_document;
	KLineEdit * m_lineEdit;
	KPopupMenu * m_caseMenu;
	bool m_caseSensitive;
};

#endif
