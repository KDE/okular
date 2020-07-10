/*************************************************************************************
 *  Copyright (C) 2018 by Aleix Pol <aleixpol@kde.org>                               *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#ifndef ANDROID_H
#define ANDROID_H

#include <QObject>
#include <QUrl>
#include <jni.h>

class URIHandler
{
public:
    void openUri(const QString &uri)
    {
        m_lastUrl = uri;
    }

    QString m_lastUrl;
    static URIHandler handler;
};

class AndroidInstance : public QObject
{
    Q_OBJECT
public:
    Q_SCRIPTABLE void openFile(const QString &title, const QStringList &mimes);

    static void handleViewIntent();

Q_SIGNALS:
    void openUri(const QUrl &uri);
};

extern "C" {

JNIEXPORT void JNICALL Java_org_kde_something_FileClass_openUri(JNIEnv *env, jobject /*obj*/, jstring uri);
}

#endif
