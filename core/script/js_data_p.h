/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_DATA_P_H
#define OKULAR_SCRIPT_JS_DATA_P_H

#include <QObject>

namespace Okular
{
class EmbeddedFile;

class JSData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime creationDate READ creationDate CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString MIMEType READ MIMEType CONSTANT)
    Q_PROPERTY(QDateTime modDate READ modDate CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(int size READ size CONSTANT)

public:
    explicit JSData(EmbeddedFile *f, QObject *parent = nullptr);
    ~JSData() override;

    QDateTime creationDate() const;
    QString description() const;
    QString MIMEType() const;
    QDateTime modDate() const;
    QString name() const;
    QString path() const;
    int size() const;

private:
    EmbeddedFile *m_file = nullptr;
};

}

#endif
