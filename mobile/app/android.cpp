/*
    SPDX-FileCopyrightText: 2018 Aleix Pol <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "android.h"

#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QStringList>

URIHandler URIHandler::handler;

void URIHandler::handleViewIntent()
{
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("handleViewIntent", "()V");
}

void Java_org_kde_something_FileClass_openUri(JNIEnv *env, jobject /*obj*/, jstring uri)
{
    jboolean isCopy = false;
    const char *utf = env->GetStringUTFChars(uri, &isCopy);
    const QString uriString = QString::fromUtf8(utf);
    URIHandler::handler.openUri(uriString);
    env->ReleaseStringUTFChars(uri, utf);
}
