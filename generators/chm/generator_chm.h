/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *   Copyright (C) 2008 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_CHM_H_
#define _OKULAR_GENERATOR_CHM_H_

#include <core/document.h>
#include <core/generator.h>

#include "lib/ebook_chm.h"

#include <QBitArray>

class KHTMLPart;

namespace Okular
{
class TextPage;
}

namespace DOM
{
class Node;
}

class CHMGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)
public:
    CHMGenerator(QObject *parent, const QVariantList &args);
    ~CHMGenerator() override;
    bool loadDocument(const QString &fileName, QVector<Okular::Page *> &pagesVector) override;

    Okular::DocumentInfo generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const override;
    const Okular::DocumentSynopsis *generateDocumentSynopsis() override;

    bool canGeneratePixmap() const override;
    void generatePixmap(Okular::PixmapRequest *request) override;

    QVariant metaData(const QString &key, const QVariant &option) const override;

public Q_SLOTS:
    void slotCompleted();

protected:
    bool doCloseDocument() override;
    Okular::TextPage *textPage(Okular::TextRequest *request) override;

private:
    void additionalRequestData();
    void recursiveExploreNodes(DOM::Node node, Okular::TextPage *tp);
    void preparePageForSyncOperation(const QString &url);
    QMap<QString, int> m_urlPage;
    QVector<QString> m_pageUrl;
    Okular::DocumentSynopsis m_docSyn;
    EBook *m_file;
    KHTMLPart *m_syncGen;
    QString m_fileName;
    QString m_chmUrl;
    Okular::PixmapRequest *m_request;
    QBitArray m_textpageAddedList;
    QBitArray m_rectsGenerated;
};

#endif
