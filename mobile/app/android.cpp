/*
    SPDX-FileCopyrightText: 2018 Aleix Pol <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "android.h"
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QDebug>
#include <QStringList>
#include <QtAndroid>

URIHandler URIHandler::handler;
static AndroidInstance *s_instance = nullptr;

void AndroidInstance::openFile(const QString &title, const QStringList &mimes)
{
    s_instance = this;
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;"); // activity is valid
    Q_ASSERT(activity.isValid());

    QAndroidJniEnvironment _env;
    QAndroidJniObject::callStaticMethod<void>("org/kde/something/OpenFileActivity",
                                              "openFile",
                                              "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)V",
                                              activity.object<jobject>(),
                                              QAndroidJniObject::fromString(title).object<jstring>(),
                                              QAndroidJniObject::fromString(mimes.join(';')).object<jstring>());
    if (_env->ExceptionCheck()) {
        _env->ExceptionClear();
        qWarning() << "couldn't launch intent";
    }
}

void AndroidInstance::handleViewIntent()
{
    QtAndroid::androidActivity().callMethod<void>("handleViewIntent", "()V");
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
