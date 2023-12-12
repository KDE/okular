/*
    SPDX-FileCopyrightText: 2018 Aleix Pol <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "android.h"
#include <QCoreApplication>
#include <QDebug>
#include <QJniEnvironment>
#include <QJniObject>
#include <QStringList>

URIHandler URIHandler::handler;
static AndroidInstance *s_instance = nullptr;

void AndroidInstance::openFile(const QString &title, const QStringList &mimes)
{
    s_instance = this;
    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;"); // activity is valid
    Q_ASSERT(activity.isValid());

    QJniEnvironment _env;
    QJniObject::callStaticMethod<void>("org/kde/something/OpenFileActivity",
                                       "openFile",
                                       "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)V",
                                       activity.object<jobject>(),
                                       QJniObject::fromString(title).object<jstring>(),
                                       QJniObject::fromString(mimes.join(QLatin1Char(';'))).object<jstring>());
    if (_env.checkAndClearExceptions()) {
        qWarning() << "couldn't launch intent";
    }
}

void AndroidInstance::handleViewIntent()
{
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("handleViewIntent", "()V");
}

void Java_org_kde_something_FileClass_openUri(JNIEnv *env, jobject /*obj*/, jstring uri)
{
    jboolean isCopy = false;
    const char *utf = env->GetStringUTFChars(uri, &isCopy);
    const QString uriString = QString::fromUtf8(utf);
    if (s_instance)
        s_instance->openUri(QUrl(uriString));
    else
        URIHandler::handler.openUri(uriString);
    env->ReleaseStringUTFChars(uri, utf);
}
