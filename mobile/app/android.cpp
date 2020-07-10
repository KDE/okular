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
