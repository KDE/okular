/*
    SPDX-FileCopyrightText: 2008 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MOBI_DOCUMENT_H
#define MOBI_DOCUMENT_H

#include <QFile>
#include <QTextDocument>
#include <QUrl>
#include <QVariant>
#include <qmobipocket_version.h>

#if QMOBIPOCKET_VERSION_MAJOR < 3
#include <qmobipocket/qfilestream.h>
#endif

class QFile;
namespace Mobipocket
{
class Document;
}

namespace Mobi
{
class MobiDocument : public QTextDocument
{
    Q_OBJECT

public:
    explicit MobiDocument(const QString &fileName);
    ~MobiDocument() override;

    Mobipocket::Document *mobi() const
    {
        return doc.get();
    }

protected:
    QVariant loadResource(int type, const QUrl &name) override;

private:
    QString fixMobiMarkup(const QString &data);
    std::unique_ptr<Mobipocket::Document> doc;
#if QMOBIPOCKET_VERSION_MAJOR < 3
    Mobipocket::QFileStream m_file;
#else
    QFile m_file;
#endif
};

}
#endif
