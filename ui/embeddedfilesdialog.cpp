/***************************************************************************
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "embeddedfilesdialog.h"

#include <QDateTime>
#include <QTreeWidget>

#include <kfiledialog.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kglobal.h>

#include "core/document.h"

static QString dateToString( const QDateTime & date )
{
	return date.isValid()
		? KGlobal::locale()->formatDateTime( date, KLocale::LongDate, true )
		: i18nc( "Unknown date", "Unknown" );
}

EmbeddedFilesDialog::EmbeddedFilesDialog(QWidget *parent, const Okular::Document *document) : KDialog(parent)
{
	setCaption(i18n("Embedded Files"));
	setButtons(Close | User1);
	setDefaultButton(Close);
	setButtonGuiItem(User1, KStandardGuiItem::save());

	m_tw = new QTreeWidget(this);
	setMainWidget(m_tw);
	QStringList header;
	header.append(i18n("Name"));
	header.append(i18n("Description"));
	header.append(i18n("Created"));
	header.append(i18n("Modified"));
	m_tw->setHeaderLabels(header);
	m_tw->setRootIsDecorated(false);
	m_tw->setSelectionMode(QAbstractItemView::ExtendedSelection);

	foreach(Okular::EmbeddedFile* ef, *document->embeddedFiles())
	{
		QTreeWidgetItem *twi = new QTreeWidgetItem();
		twi->setText(0, ef->name());
		KMimeType::Ptr mime = KMimeType::findByPath( ef->name(), 0, true );
		if (mime)
		{
			twi->setIcon(0, KIcon(mime->iconName()));
		}
		twi->setText(1, ef->description());
		twi->setText(2, dateToString( ef->creationDate() ) );
		twi->setText(3, dateToString( ef->modificationDate() ) );
		m_tw->addTopLevelItem(twi);
		m_files.insert(twi, ef);
	}
        // Having filled the columns, it is nice to resize them to be able to read the contents
        for (int lv = 0; lv <  m_tw->columnCount(); ++lv) {
                m_tw->resizeColumnToContents(lv);
        }
        // This is a bit dubious, but I'm not seeing a nice way to say "expand to fit contents"
        m_tw->setMinimumWidth(640);
        m_tw->updateGeometry();

	connect(this, SIGNAL(user1Clicked()), this, SLOT(saveFile()));
}

void EmbeddedFilesDialog::saveFile()
{
	QList<QTreeWidgetItem *> selected = m_tw->selectedItems();
	foreach(QTreeWidgetItem *twi, selected)
	{
		Okular::EmbeddedFile* ef = m_files[twi];
		QString path = KFileDialog::getSaveFileName(ef->name(), QString(), this, i18n("Where do you want to save %1?", ef->name()));
		if (!path.isEmpty())
		{
			QFile f(path);
			if (!f.exists() || KMessageBox::warningContinueCancel( this, i18n("A file named \"%1\" already exists. Are you sure you want to overwrite it?", path), QString(), KGuiItem(i18n("Overwrite"))) == KMessageBox::Continue)
			{
				f.open(QIODevice::WriteOnly);
				f.write(ef->data());
				f.close();
			}
			
		}
	}
}

#include "embeddedfilesdialog.moc"
