
/***************************************************************************
 *   Copyright (C) 2005 by Georgy Yunaev                                   *
 *   tim@krasnogorsk.ru                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <qstringlist.h>
#include <qstyle.h>
#include <qstyleoption.h>

#include "kchmtreeviewitem.h"
#include "xchmfile.h"
#include "iconstorage.h"


KCHMMainTreeViewItem::KCHMMainTreeViewItem( K3ListViewItem * parent, K3ListViewItem * after, QString name, QString aurl, int image) :
    K3ListViewItem (parent, after, name), url(aurl), image_number(image)
{
}

KCHMMainTreeViewItem::KCHMMainTreeViewItem( K3ListView * parent, K3ListViewItem * after, QString name, QString aurl, int image) :
    K3ListViewItem (parent, after, name), url(aurl), image_number(image)
{
}


const QPixmap * KCHMMainTreeViewItem::pixmap( int i ) const
{
	int imagenum;

    if ( i || image_number == KCHMImageType::IMAGE_NONE || image_number == KCHMImageType::IMAGE_INDEX )
        return 0;

	if ( firstChild () )
	{
		if ( isOpen() )
			imagenum = (image_number == KCHMImageType::IMAGE_AUTO) ? 1 : image_number + 1;
		else
			imagenum = (image_number == KCHMImageType::IMAGE_AUTO) ? 0 : image_number;
	}
	else
		imagenum = (image_number == KCHMImageType::IMAGE_AUTO) ? 10 : image_number;

	return gIconStorage.getBookIconPixmap(imagenum);
}

QString KCHMMainTreeViewItem::getUrl( ) const
{
	if ( url.find ('|') == -1 )
		return url;
/*
	// Create a dialog with URLs, and show it, so user can select an URL he/she wants.
	QStringList urls = QStringList::split ('|', url);
	QStringList titles;
	CHMFile * xchm = (CHMFile *) ::mainWindow->getChmFile();

	for ( unsigned int i = 0; i < urls.size(); i++ )
	{
		QString title = xchm->getTopicByUrl (urls[i]);
		
		if ( !title )
		{
			qWarning ("Could not get item name for url '%s'", urls[i].ascii());
			titles.push_back(QString::null);
		}
		else
			titles.push_back(title);
	}

 	KCHMDialogChooseUrlFromList dlg (urls, titles, ::mainWindow);

	if ( dlg.exec() == QDialog::Accepted )
		return dlg.getSelectedItemUrl();
*/
	return QString::null;
}

void KCHMMainTreeViewItem::paintBranches( QPainter * p, const QColorGroup & cg, int w, int y, int h )
{
	if ( image_number != KCHMImageType::IMAGE_INDEX )
		K3ListViewItem::paintBranches(p, cg, w, y, h);
	else
	{
		// Too bad that listView()->paintEmptyArea( p, QRect( 0, 0, w, h ) ) is protected. 
		// Taken from qt-x11-free-3.0.4/src/widgets/qlistview.cpp
    	QStyleOptionQ3ListView opt;
    	listView()->style()->drawComplexControl( QStyle::CC_Q3ListView, &opt, p, listView());
	}
}


void KCHMMainTreeViewItem::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    QColorGroup newcg ( cg );
    QColor c = newcg.text();

	if ( url.find ('|') != -1 )
        newcg.setColor( QColorGroup::Text, Qt::red );
	else if ( url[0] == ':' )
        newcg.setColor( QColorGroup::Text, Qt::lightGray );
	else
	{
		K3ListViewItem::paintCell( p, cg, column, width, align );
		return;
	}

    K3ListViewItem::paintCell( p, newcg, column, width, align );
	newcg.setColor( QColorGroup::Text, c );
}

void KCHMMainTreeViewItem::setOpen( bool open )
{
	if ( image_number != KCHMImageType::IMAGE_INDEX || open )
		K3ListViewItem::setOpen (open);
}

/*
int KCHMMainTreeViewItem::width( const QFontMetrics & fm, const Q3ListView * lv, int c ) const
{
        return (QListViewItem::width
            (QFontMetrics (myFont), lv, c));
}
*/
