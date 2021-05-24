/*
    SPDX-FileCopyrightText: 2018 Aleix Pol <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
