/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "document.h"

#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QXmlStreamReader>

#include <KLocalizedString>
#include <KTar>
#include <QMimeDatabase>
#include <QMimeType>
#include <kzip.h>
#if WITH_K7ZIP
#include <K7Zip>
#endif

#include <memory>

#include <core/document.h>
#include <core/page.h>

#include "debug_comicbook.h"
#include "directory.h"
#include "qnatsort.h"
#include "unrar.h"

using namespace ComicBook;

static void imagesInArchive(const QString &prefix, const KArchiveDirectory *dir, QStringList *entries)
{
    const QStringList entryList = dir->entries();
    for (const QString &file : entryList) {
        const KArchiveEntry *e = dir->entry(file);
        if (e->isDirectory()) {
            imagesInArchive(prefix + file + QLatin1Char('/'), static_cast<const KArchiveDirectory *>(e), entries);
        } else if (e->isFile()) {
            entries->append(prefix + file);
        }
    }
}

Document::Document()
    : mDirectory(nullptr)
    , mUnrar(nullptr)
    , mArchive(nullptr)
    , mArchiveDir(nullptr)
{
}

Document::~Document()
{
}

bool Document::open(const QString &fileName)
{
    close();

    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

    /**
     * We have a zip archive
     */
    if (mime.inherits(QStringLiteral("application/x-cbz")) || mime.inherits(QStringLiteral("application/zip"))) {
        mArchive = new KZip(fileName);

        if (!processArchive()) {
            return false;
        }
        /**
         * We have a TAR archive
         */
    } else if (mime.inherits(QStringLiteral("application/x-cbt")) || mime.inherits(QStringLiteral("application/x-gzip")) || mime.inherits(QStringLiteral("application/x-tar")) || mime.inherits(QStringLiteral("application/x-bzip"))) {
        mArchive = new KTar(fileName);

        if (!processArchive()) {
            return false;
        }
#if WITH_K7ZIP
        /**
         * We have a 7z archive
         */
    } else if (mime.inherits(QStringLiteral("application/x-cb7")) || mime.inherits(QStringLiteral("application/x-7z-compressed"))) {
        mArchive = new K7Zip(fileName);

        if (!processArchive()) {
            return false;
        }
#endif
    } else if (mime.inherits(QStringLiteral("application/x-cbr")) || mime.inherits(QStringLiteral("application/x-rar")) || mime.inherits(QStringLiteral("application/vnd.rar"))) {
        if (!Unrar::isAvailable()) {
            mLastErrorString = i18n("Cannot open document, neither unrar nor unarchiver were found.");
            return false;
        }

        if (!Unrar::isSuitableVersionAvailable()) {
            mLastErrorString = i18n("The version of unrar on your system is not suitable for opening comicbooks.");
            return false;
        }

        /**
         * We have a rar archive
         */
        mUnrar = new Unrar();

        if (!mUnrar->open(fileName)) {
            delete mUnrar;
            mUnrar = nullptr;

            return false;
        }

        mEntries = mUnrar->list();
    } else if (mime.inherits(QStringLiteral("inode/directory"))) {
        mDirectory = new Directory();

        if (!mDirectory->open(fileName)) {
            delete mDirectory;
            mDirectory = nullptr;

            return false;
        }

        mEntries = mDirectory->list();
    } else {
        mLastErrorString = i18n("Unknown ComicBook format.");
        return false;
    }

    return true;
}

void Document::close()
{
    mLastErrorString.clear();

    if (!(mArchive || mUnrar || mDirectory)) {
        return;
    }

    delete mArchive;
    mArchive = nullptr;
    delete mDirectory;
    mDirectory = nullptr;
    delete mUnrar;
    mUnrar = nullptr;
    mPageMap.clear();
    mEntries.clear();
}

bool Document::processArchive()
{
    if (!mArchive->open(QIODevice::ReadOnly)) {
        delete mArchive;
        mArchive = nullptr;

        return false;
    }

    const KArchiveDirectory *directory = mArchive->directory();
    if (!directory) {
        delete mArchive;
        mArchive = nullptr;

        return false;
    }

    mArchiveDir = directory;

    imagesInArchive(QString(), mArchiveDir, &mEntries);

    return true;
}

std::unique_ptr<QIODevice> Document::createDevice(const QString &file) const
{
    if (mArchive) {
        const KArchiveFile *entry = static_cast<const KArchiveFile *>(mArchiveDir->entry(file));
        if (entry) {
            return std::unique_ptr<QIODevice>(entry->createDevice());
        }
        return nullptr;
    } else if (mDirectory) {
        return std::unique_ptr<QIODevice>(mDirectory->createDevice(file));
    } else {
        return std::unique_ptr<QIODevice>(mUnrar->createDevice(file));
    }
}

void Document::pages(QList<Okular::Page *> *pagesVector)
{
    std::sort(mEntries.begin(), mEntries.end(), caseSensitiveNaturalOrderLessThen);
    std::unique_ptr<QIODevice> dev;

    int count = 0;
    pagesVector->clear();
    pagesVector->resize(mEntries.size());
    QImageReader reader;
    reader.setAutoTransform(true);
    for (const QString &file : std::as_const(mEntries)) {
        dev = createDevice(file);

        if (dev) {
            reader.setDevice(dev.get());
            if (reader.canRead()) {
                QSize pageSize = reader.size();
                if (reader.transformation() & QImageIOHandler::TransformationRotate90) {
                    pageSize.transpose();
                }
                if (!pageSize.isValid()) {
                    const QImage i = reader.read();
                    if (!i.isNull()) {
                        pageSize = i.size();
                    }
                }
                if (pageSize.isValid()) {
                    pagesVector->replace(count, new Okular::Page(count, pageSize.width(), pageSize.height(), Okular::Rotation0));
                    mPageMap.append(file);
                    count++;
                } else {
                    qCDebug(OkularComicbookDebug) << "Ignoring" << file << "doesn't seem to be an image even if QImageReader::canRead returned true";
                }
            }
        }
    }
    pagesVector->resize(count);
}

QStringList Document::pageTitles() const
{
    return QStringList();
}

std::unique_ptr<Okular::DocumentSynopsis> Document::documentSynopsis() const
{
    // ComicInfo.xml lives at the archive root; a nested one is a useful fallback
    QString comicInfo;
    for (const QString &entry : mEntries) {
        if (entry == QLatin1String("ComicInfo.xml")) {
            comicInfo = entry;
            break;
        }
        if (comicInfo.isEmpty() && entry.endsWith(QLatin1String("/ComicInfo.xml"))) {
            comicInfo = entry;
        }
    }
    if (comicInfo.isEmpty()) {
        return nullptr;
    }

    const std::unique_ptr<QIODevice> dev = createDevice(comicInfo);
    if (!dev) {
        return nullptr;
    }

    auto synopsis = std::make_unique<Okular::DocumentSynopsis>();
    QXmlStreamReader xml(dev.get());
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QLatin1String("Page")) {
            continue;
        }
        const QXmlStreamAttributes attrs = xml.attributes();
        const QString title = attrs.value(QLatin1String("Bookmark")).toString();
        bool ok = false;
        const int page = attrs.value(QLatin1String("Image")).toInt(&ok);
        if (ok && !title.isEmpty() && page >= 0 && page < mPageMap.size()) {
            Okular::DocumentSynopsis::ElementBuilder builder(title);
            builder.setViewPort(Okular::DocumentViewport(page));
            synopsis->addChild(Okular::DocumentSynopsis::Element(builder));
        }
    }

    if (synopsis->children().isEmpty()) {
        return nullptr;
    }
    return synopsis;
}

QImage Document::pageImage(int page) const
{
    if (mArchive) {
        const KArchiveFile *entry = static_cast<const KArchiveFile *>(mArchiveDir->entry(mPageMap[page]));
        if (entry) {
            std::unique_ptr<QIODevice> dev(entry->createDevice());
            // This could simply be
            //     QImageReader reader(dev.get());
            // but due to https://codereview.qt-project.org/c/qt/qtbase/+/349174 and https://invent.kde.org/frameworks/karchive/-/merge_requests/14
            // it can not, so it will have to be like this at least until Qt6
            // Test with https://bugs.kde.org/attachment.cgi?id=74039 (it's a cbz with a png inside)
            QBuffer b;
            b.setData(dev->readAll());
            QImageReader reader(&b);
            reader.setAutoTransform(true);
            return reader.read();
        }
    } else if (mDirectory) {
        return QImage(mPageMap[page]);
    } else {
        return QImage::fromData(mUnrar->contentOf(mPageMap[page]));
    }

    return QImage();
}

QString Document::lastErrorString() const
{
    return mLastErrorString;
}

Q_LOGGING_CATEGORY(OkularComicbookDebug, "org.kde.okular.generators.comicbook", QtWarningMsg)
