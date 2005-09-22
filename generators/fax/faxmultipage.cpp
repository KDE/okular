/***************************************************************************
 *   Copyright (C) 2005 by Stefan Kebekus                                  *
 *   kebekus@kde.org                                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include <kfiledialog.h>
#include <kparts/genericfactory.h>

#include "faxmultipage.h"



typedef KParts::GenericFactory<FaxMultiPage> FaxMultiPageFactory;
K_EXPORT_COMPONENT_FACTORY(kfaxviewpart, FaxMultiPageFactory)


FaxMultiPage::FaxMultiPage(QWidget *parentWidget, const char *widgetName, QObject *parent,
                             const char *name, const QStringList&)
  : KMultiPage(parentWidget, widgetName, parent, name), faxRenderer(parentWidget)
{
  /* This is kparts wizardry that cannot be understood by man. Simply
     change the names to match your implementation.  */
  setInstance(FaxMultiPageFactory::instance());
  faxRenderer.setName("Fax renderer");

  setXMLFile("kfaxview.rc");

  /* It is very important that this method is called in the
     constructor. Otherwise kmultipage does not know how to render
     files, and crashes may result. */
  setRenderer(&faxRenderer);
}


FaxMultiPage::~FaxMultiPage()
{
  ;
}


KAboutData* FaxMultiPage::createAboutData()
{
  /* You obviously want to change this to match your setup */
  KAboutData* about = new KAboutData("kfaxview", I18N_NOOP("KFaxView"), "0.1",
				     I18N_NOOP("KViewshell Fax Plugin."),
				     KAboutData::License_GPL,
				     "Stefan Kebekus",
				     I18N_NOOP("This program previews fax (g3) files."));

  about->addAuthor ("Stefan Kebekus",
                    I18N_NOOP("Current Maintainer."),
                    "kebekus@kde.org",
                    "http://www.mi.uni-koeln.de/~kebekus");
  return about;
}


QStringList FaxMultiPage::fileFormats()
{
  /* This list is used in the file selection dialog when the file is
     saved */
  QStringList r;
  r << i18n("*.g3|Fax (g3) file (*.g3)");
  return r;
}


#include "faxmultipage.moc"
