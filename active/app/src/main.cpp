/***************************************************************************
 *                                                                         *
 *   Copyright 2011 Sebastian Kügler <sebas@kde.org>                       *
 *   Copyright 2012 Marco Martin <mart@kde.org>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

// KDE
#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>
#include <KDE/KLocale>
#include <KConfigGroup>

#include "kdeclarativemainwindow.h"
#include "kdeclarativeview.h"

static const char description[] = I18N_NOOP("Document viewer for Plasma Active using Okular");

static const char version[] = "0.1";

int main(int argc, char **argv)
{
    KAboutData about("active-documentviewer", 0, ki18n("Reader"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("Copyright 2012 Marco Martin"), KLocalizedString(), 0, "mart@kde.org");
                     about.addAuthor( ki18n("Marco Martin"), KLocalizedString(), "mart@kde.org" );
    about.setProgramIconName("okular");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[url]", ki18n( "URL of the file to open" ));

    KCmdLineArgs::addCmdLineOptions(options);
    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KGlobal::locale()->insertCatalog("org.kde.okular");
    //kDebug() << "ARGS:" << args << args->count();

    KDeclarativeMainWindow *mainWindow = new KDeclarativeMainWindow();
    mainWindow->declarativeView()->setPackageName("org.kde.active.documentviewer");
    mainWindow->show();
    args->clear();
    return app.exec();
}
