/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include "mobiendanalyzer.h"

#define STRIGI_IMPORT_API
#include <strigi/strigiconfig.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>
#include <strigi/streamendanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <list>

#include "mobipocket.h"

using namespace Strigi;

class StrigiStream : public Mobipocket::Stream
{
public:
    StrigiStream(InputStream* str) : d(str) {}
    int read(char* buf, int len) {
         const char* b2;
         int l=d->read(b2, len,len);
         memcpy(buf,b2,len);
         return l;
    }
    bool seek(int pos) { d->reset(pos); return (pos==d->position()); }
private:
    InputStream *d;
};

void
MobiEndAnalyzerFactory::registerFields(FieldRegister& reg) {
    subjectField = reg.registerField(
        "http://freedesktop.org/standards/xesam/1.0/core#subject");
    titleField = reg.registerField(
        "http://freedesktop.org/standards/xesam/1.0/core#title");
    authorField = reg.registerField(
        "http://freedesktop.org/standards/xesam/1.0/core#author");
    descriptionField = reg.registerField(
        "http://freedesktop.org/standards/xesam/1.0/core#description");
    copyrightField = reg.registerField(
        "http://freedesktop.org/standards/xesam/1.0/core#copyright");
    encryptedField = reg.registerField(
        "http://freedesktop.org/standards/xesam/1.0/core#isContentEncrypted");
//    typeField = reg.typeField;
    //FIXME: check other fields - for example encryption

    /* add the fields to the internal list of fields */
    addField(subjectField);
    addField(titleField);
    addField(authorField);
    addField(descriptionField);
    addField(copyrightField);
    addField(encryptedField);
//    addField(typeField);
}

MobiEndAnalyzer::MobiEndAnalyzer(const MobiEndAnalyzerFactory* f) :factory(f) {
}
bool
MobiEndAnalyzer::checkHeader(const char* header, int32_t headersize) const { //krazy:exclude=typedefs
    static const char magic1[] = "TEXtREAd";
    static const char magic2[] = "BOOKMOBI";
    return headersize >= 67 && (!memcmp(header+60, magic1, 8) || !memcmp(header+60, magic2, 8));
}

signed char
MobiEndAnalyzer::analyze(AnalysisResult& as, InputStream* in) {
    StrigiStream str(in);
    Mobipocket::Document doc(&str);
    if (!doc.isValid()) return -1;
//    as.addValue(factory->typeField, "http://freedesktop.org/standards/xesam/1.0/core#Document");
    as.addValue(factory->encryptedField, doc.hasDRM());
    QMapIterator<Mobipocket::Document::MetaKey,QString> it(doc.metadata());
    while (it.hasNext()) {
        it.next();
        switch (it.key()) {
            case Mobipocket::Document::Title: as.addValue(factory->titleField, it.value().toUtf8().data() ); break;
            case Mobipocket::Document::Author: as.addValue(factory->authorField, it.value().toUtf8().data() ); break;
            case Mobipocket::Document::Description: as.addValue(factory->descriptionField, it.value().toUtf8().data() ); break;
            case Mobipocket::Document::Subject: as.addValue(factory->subjectField, it.value().toUtf8().data() ); break;
            case Mobipocket::Document::Copyright: as.addValue(factory->copyrightField, it.value().toUtf8().data() ); break;
        }
    }
    if (!doc.hasDRM()) {
        QByteArray text=doc.text(20480).toUtf8();
        as.addText(text.data(), text.size());
    }
    return 0;
}

class Factory : public AnalyzerFactoryFactory 
{
public:
    std::list<StreamEndAnalyzerFactory*>
    streamEndAnalyzerFactories() const {
        std::list<StreamEndAnalyzerFactory*> af;
        af.push_back(new MobiEndAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Factory)
