/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_document_p.h"

#include <qwidget.h>

#include <QDebug>
#include <QJSEngine>
#include <assert.h>

#include "../document_p.h"
#include "../form.h"
#include "../page.h"
#include "js_data_p.h"
#include "js_field_p.h"
#include "js_ocg_p.h"

using namespace Okular;

// Document.numPages
int JSDocument::numPages() const
{
    return m_doc->m_pagesVector.count();
}

// Document.pageNum (getter)
int JSDocument::pageNum() const
{
    return m_doc->m_parent->currentPage();
}

// Document.pageNum (setter)
void JSDocument::setPageNum(int page)
{
    if (page == (int)m_doc->m_parent->currentPage()) {
        return;
    }

    m_doc->m_parent->setViewportPage(page);
}

// Document.documentFileName
QString JSDocument::documentFileName() const
{
    return m_doc->m_url.fileName();
}

// Document.filesize
int JSDocument::filesize() const
{
    return m_doc->m_docSize;
}

// Document.path
QString JSDocument::path() const
{
    return m_doc->m_url.toDisplayString(QUrl::PreferLocalFile);
}

// Document.URL
QString JSDocument::URL() const
{
    return m_doc->m_url.toDisplayString();
}

// Document.permStatusReady
bool JSDocument::permStatusReady() const
{
    return true;
}

// Document.dataObjects
QJSValue JSDocument::dataObjects() const
{
    const QList<EmbeddedFile *> *files = m_doc->m_generator->embeddedFiles();

    QJSValue dataObjects = qjsEngine(this)->newArray(files ? files->count() : 0);
    if (files) {
        QList<EmbeddedFile *>::ConstIterator it = files->begin(), itEnd = files->end();
        for (int i = 0; it != itEnd; ++it, ++i) {
            QJSValue newdata = qjsEngine(this)->newQObject(new JSData(*it));
            dataObjects.setProperty(i, newdata);
        }
    }
    return dataObjects;
}

// Document.external
bool JSDocument::external() const
{
    QWidget *widget = m_doc->m_widget;

    const bool isShell = (widget && widget->parentWidget() && widget->parentWidget()->objectName().startsWith(QLatin1String("okular::Shell")));
    return !isShell;
}

// Document.numFields
int JSDocument::numFields() const
{
    unsigned int numFields = 0;

    for (const Page *pIt : qAsConst(m_doc->m_pagesVector)) {
        numFields += pIt->formFields().size();
    }

    return numFields;
}

QJSValue JSDocument::info() const
{
    QJSValue obj = qjsEngine(this)->newObject();
    QSet<DocumentInfo::Key> keys;
    keys << DocumentInfo::Title << DocumentInfo::Author << DocumentInfo::Subject << DocumentInfo::Keywords << DocumentInfo::Creator << DocumentInfo::Producer;
    const DocumentInfo docinfo = m_doc->m_parent->documentInfo(keys);
#define KEY_GET(key, property)                                                                                                                                                                                                                 \
    do {                                                                                                                                                                                                                                       \
        const QString data = docinfo.get(key);                                                                                                                                                                                                 \
        if (!data.isEmpty()) {                                                                                                                                                                                                                 \
            obj.setProperty(QStringLiteral(property), data);                                                                                                                                                                                   \
            obj.setProperty(QStringLiteral(property).toLower(), data);                                                                                                                                                                         \
        }                                                                                                                                                                                                                                      \
    } while (0);
    KEY_GET(DocumentInfo::Title, "Title");
    KEY_GET(DocumentInfo::Author, "Author");
    KEY_GET(DocumentInfo::Subject, "Subject");
    KEY_GET(DocumentInfo::Keywords, "Keywords");
    KEY_GET(DocumentInfo::Creator, "Creator");
    KEY_GET(DocumentInfo::Producer, "Producer");
#undef KEY_GET
    return obj;
}

#define DOCINFO_GET_METHOD(key, name)                                                                                                                                                                                                          \
    QString JSDocument::name() const                                                                                                                                                                                                           \
    {                                                                                                                                                                                                                                          \
        const DocumentInfo docinfo = m_doc->m_parent->documentInfo(QSet<DocumentInfo::Key>() << key);                                                                                                                                          \
        return docinfo.get(key);                                                                                                                                                                                                               \
    }

DOCINFO_GET_METHOD(DocumentInfo::Author, author)
DOCINFO_GET_METHOD(DocumentInfo::Creator, creator)
DOCINFO_GET_METHOD(DocumentInfo::Keywords, keywords)
DOCINFO_GET_METHOD(DocumentInfo::Producer, producer)
DOCINFO_GET_METHOD(DocumentInfo::Title, title)
DOCINFO_GET_METHOD(DocumentInfo::Subject, subject)

#undef DOCINFO_GET_METHOD

// Document.getField()
QJSValue JSDocument::getField(const QString &cName) const
{
    QVector<Page *>::const_iterator pIt = m_doc->m_pagesVector.constBegin(), pEnd = m_doc->m_pagesVector.constEnd();
    for (; pIt != pEnd; ++pIt) {
        const QList<Okular::FormField *> pageFields = (*pIt)->formFields();
        for (FormField *form : pageFields) {
            if (form->fullyQualifiedName() == cName) {
                return JSField::wrapField(qjsEngine(this), form, *pIt);
            }
        }
    }
    return QJSValue(QJSValue::UndefinedValue);
}

// Document.getPageLabel()
QString JSDocument::getPageLabel(int nPage) const
{
    Page *p = m_doc->m_pagesVector.value(nPage);
    return p ? p->label() : QString();
}

// Document.getPageRotation()
int JSDocument::getPageRotation(int nPage) const
{
    Page *p = m_doc->m_pagesVector.value(nPage);
    return p ? p->orientation() * 90 : 0;
}

// Document.gotoNamedDest()
void JSDocument::gotoNamedDest(const QString &cName) const
{
    DocumentViewport viewport(m_doc->m_generator->metaData(QStringLiteral("NamedViewport"), cName).toString());
    if (viewport.isValid()) {
        m_doc->m_parent->setViewport(viewport);
    }
}

// Document.syncAnnotScan()
void JSDocument::syncAnnotScan() const
{
}

// Document.getNthFieldName
QJSValue JSDocument::getNthFieldName(int nIndex) const
{
    for (const Page *pIt : qAsConst(m_doc->m_pagesVector)) {
        const QList<Okular::FormField *> pageFields = pIt->formFields();

        if (nIndex < pageFields.size()) {
            const Okular::FormField *form = pageFields[nIndex];

            return form->fullyQualifiedName();
        }

        nIndex -= pageFields.size();
    }

    return QJSValue(QJSValue::UndefinedValue);
}

QJSValue JSDocument::getOCGs([[maybe_unused]] int nPage) const
{
    QAbstractItemModel *model = m_doc->m_parent->layersModel();

    QJSValue array = qjsEngine(this)->newArray(model->rowCount());

    for (int i = 0; i < model->rowCount(); ++i) {
        for (int j = 0; j < model->columnCount(); ++j) {
            const QModelIndex index = model->index(i, j);

            QJSValue item = qjsEngine(this)->newQObject(new JSOCG(model, i, j));
            item.setProperty(QStringLiteral("name"), model->data(index, Qt::DisplayRole).toString());
            item.setProperty(QStringLiteral("initState"), model->data(index, Qt::CheckStateRole).toBool());

            array.setProperty(i, item);
        }
    }

    return array;
}

JSDocument::JSDocument(DocumentPrivate *doc, QObject *parent)
    : QObject(parent)
    , m_doc(doc)
{
}

JSDocument::~JSDocument() = default;
