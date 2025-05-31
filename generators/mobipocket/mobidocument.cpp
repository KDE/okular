/*
    SPDX-FileCopyrightText: 2008 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "mobidocument.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <qmobipocket/mobipocket.h>

using namespace Mobi;

MobiDocument::MobiDocument(const QString &fileName)
    : QTextDocument()
#if QMOBIPOCKET_VERSION_MAJOR < 3
    , m_file(Mobipocket::QFileStream(fileName))
{
#else
    , m_file(QFile(fileName))
{
    if (!m_file.open(QIODeviceBase::ReadOnly)) {
        return;
    }
#endif
    doc = std::make_unique<Mobipocket::Document>(&m_file);
    if (doc->isValid()) {
        QString text = doc->text();
        QString header = text.left(1024);
        if (header.contains(QStringLiteral("<html")) || header.contains(QStringLiteral("<HTML"))) {
            setDefaultStyleSheet(QStringLiteral("a { color: %1 }").arg(QColor(Qt::blue).name()));
            setHtml(fixMobiMarkup(text));
        } else {
            setPlainText(text);
        }
    }
}

MobiDocument::~MobiDocument() = default;

QVariant MobiDocument::loadResource(int type, const QUrl &name)
{
    if (type != QTextDocument::ImageResource || name.scheme() != QString(QStringLiteral("pdbrec"))) {
        return QVariant();
    }
    bool ok;
    quint16 recnum = QStringView {name.path()}.mid(1).toUShort(&ok);
    if (!ok || recnum >= doc->imageCount()) {
        return QVariant();
    }

    QVariant resource;
    resource.setValue(doc->getImage(recnum - 1));
    addResource(type, name, resource);

    return resource;
}

// starting from 'pos', find position in the string that is not inside a tag
int outsideTag(const QString &data, int pos)
{
    for (int i = pos - 1; i >= 0; i--) {
        if (data[i] == QLatin1Char('>')) {
            return pos;
        }
        if (data[i] == QLatin1Char('<')) {
            return i;
        }
    }
    return pos;
}

QString MobiDocument::fixMobiMarkup(const QString &data)
{
    QString ret = data;
    QMap<int, QString> anchorPositions;
    static QRegularExpression anchors(QStringLiteral("<a(?: href=\"[^\"]*\"){0,1}[\\s]+filepos=['\"]{0,1}([\\d]+)[\"']{0,1}"), QRegularExpression::CaseInsensitiveOption);

    // find all link destinations
    auto matcher = anchors.globalMatch(data);
    while (matcher.hasNext()) {
        auto match = matcher.next();
        int filepos = match.captured(1).toUInt();
        if (filepos) {
            anchorPositions[filepos] = match.captured(1);
        }
    }

    // put HTML anchors in all link destinations
    int offset = 0;
    QMapIterator<int, QString> it(anchorPositions);
    while (it.hasNext()) {
        it.next();
        // link pointing outside the document, ignore
        if ((it.key() + offset) >= ret.size()) {
            continue;
        }
        int fixedpos = outsideTag(ret, it.key() + offset);
        ret.insert(fixedpos, QStringLiteral("<a name=\"") + it.value() + QStringLiteral("\">&nbsp;</a>"));
        // inserting anchor shifts all offsets after the anchor
        offset += 21 + it.value().size();
    }

    // replace links referencing filepos with normal internal links
    ret.replace(anchors, QStringLiteral("<a href=\"#\\1\""));
    // Mobipocket uses strange variang of IMG tags: <img recindex="3232"> where recindex is number of
    // record containing image
    static QRegularExpression imgs(QStringLiteral("<img.*?recindex=\"([\\d]*)\".*?>"), QRegularExpression::CaseInsensitiveOption);

    ret.replace(imgs, QStringLiteral("<img src=\"pdbrec:/\\1\">"));
    ret.replace(QStringLiteral("<mbp:pagebreak/>"), QStringLiteral("<p style=\"page-break-after:always\"></p>"));

    return ret;
}
