/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_DOCUMENT_P_H
#define OKULAR_SCRIPT_JS_DOCUMENT_P_H

#include <QJSValue>
#include <QObject>

namespace Okular
{
class DocumentPrivate;

class JSDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int numPages READ numPages CONSTANT)
    Q_PROPERTY(int pageNum READ pageNum WRITE setPageNum) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QString documentFileName READ documentFileName CONSTANT)
    Q_PROPERTY(int filesize READ filesize CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QString URL READ URL CONSTANT)
    Q_PROPERTY(bool permStatusReady READ permStatusReady CONSTANT)
    Q_PROPERTY(QJSValue dataObjects READ dataObjects CONSTANT)
    Q_PROPERTY(bool external READ external CONSTANT)
    Q_PROPERTY(int numFields READ numFields CONSTANT)

    // info properties
    Q_PROPERTY(QJSValue info READ info CONSTANT)
    Q_PROPERTY(QString author READ author CONSTANT)
    Q_PROPERTY(QString creator READ creator CONSTANT)
    Q_PROPERTY(QString keywords READ keywords CONSTANT)
    Q_PROPERTY(QString producer READ producer CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString subject READ subject CONSTANT)

public:
    explicit JSDocument(DocumentPrivate *doc, QObject *parent = nullptr);
    ~JSDocument() override;

    int numPages() const;
    int pageNum() const;
    void setPageNum(int pageNum);
    QString documentFileName() const;
    int filesize() const;
    QString path() const;
    QString URL() const;
    bool permStatusReady() const;
    QJSValue dataObjects() const;
    bool external() const;
    int numFields() const;

    QJSValue info() const;
    QString author() const;
    QString creator() const;
    QString keywords() const;
    QString producer() const;
    QString title() const;
    QString subject() const;

    Q_INVOKABLE QJSValue getField(const QString &cName) const;
    Q_INVOKABLE QString getPageLabel(int nPage) const;
    Q_INVOKABLE int getPageRotation(int nPage) const;
    Q_INVOKABLE void gotoNamedDest(const QString &cName) const;
    Q_INVOKABLE void syncAnnotScan() const;
    Q_INVOKABLE QJSValue getNthFieldName(int nIndex) const;
    Q_INVOKABLE QJSValue getOCGs(int nPage = -1) const;

private:
    DocumentPrivate *m_doc = nullptr;
};

}

#endif
