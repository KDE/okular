/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "epubdocument.h"
#include <QDir>
#include <QTemporaryFile>

#include <QRegExp>

Q_LOGGING_CATEGORY(OkularEpuDebug, "org.kde.okular.generators.epu", QtWarningMsg)
using namespace Epub;

EpubDocument::EpubDocument(const QString &fileName)
    : QTextDocument()
    , padding(20)
{
    mEpub = epub_open(qPrintable(fileName), 3);

    setPageSize(QSizeF(600, 800));
}

bool EpubDocument::isValid()
{
    return (mEpub ? true : false);
}

EpubDocument::~EpubDocument()
{
    if (mEpub)
        epub_close(mEpub);

    epub_cleanup();
}

struct epub *EpubDocument::getEpub()
{
    return mEpub;
}

void EpubDocument::setCurrentSubDocument(const QString &doc)
{
    mCurrentSubDocument.clear();
    int index = doc.indexOf('/');
    if (index > 0) {
        mCurrentSubDocument = QUrl::fromLocalFile(doc.left(index + 1));
    }
}

int EpubDocument::maxContentHeight() const
{
    return pageSize().height() - (2 * padding);
}

int EpubDocument::maxContentWidth() const
{
    return pageSize().width() - (2 * padding);
}

void EpubDocument::checkCSS(QString &css)
{
    // remove paragraph line-heights
    css.remove(QRegExp(QStringLiteral("line-height\\s*:\\s*[\\w\\.]*;")));
}

QVariant EpubDocument::loadResource(int type, const QUrl &name)
{
    int size;
    char *data;

    QString fileInPath = mCurrentSubDocument.resolved(name).path();

    // Get the data from the epub file
    size = epub_get_data(mEpub, fileInPath.toUtf8().constData(), &data);

    QVariant resource;

    if (data) {
        switch (type) {
        case QTextDocument::ImageResource: {
            QImage img = QImage::fromData((unsigned char *)data, size);
            const int maxHeight = maxContentHeight();
            const int maxWidth = maxContentWidth();
            if (img.height() > maxHeight)
                img = img.scaledToHeight(maxHeight, Qt::SmoothTransformation);
            if (img.width() > maxWidth)
                img = img.scaledToWidth(maxWidth, Qt::SmoothTransformation);
            resource.setValue(img);
            break;
        }
        case QTextDocument::StyleSheetResource: {
            QString css = QString::fromUtf8(data);
            checkCSS(css);
            resource.setValue(css);
            break;
        }
        case EpubDocument::MovieResource: {
            QTemporaryFile *tmp = new QTemporaryFile(QStringLiteral("%1/okrXXXXXX").arg(QDir::tempPath()), this);
            if (!tmp->open())
                qCWarning(OkularEpuDebug) << "EPUB : error creating temporary video file";
            if (tmp->write(data, size) == -1)
                qCWarning(OkularEpuDebug) << "EPUB : error writing data" << tmp->errorString();
            tmp->flush();
            resource.setValue(tmp->fileName());
            break;
        }
        case EpubDocument::AudioResource: {
            QByteArray ba(data, size);
            resource.setValue(ba);
            break;
        }
        default:
            resource.setValue(QString::fromUtf8(data));
            break;
        }

        free(data);
    }

    // add to cache
    addResource(type, name, resource);

    return resource;
}
