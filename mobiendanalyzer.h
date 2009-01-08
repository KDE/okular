/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MOBIENDANALYZER
#define MOBIENDANALYZER

#include <strigi/streamendanalyzer.h>
#include <strigi/streambase.h>

class MobiEndAnalyzerFactory;
class MobiEndAnalyzer : public Strigi::StreamEndAnalyzer {
private:
    const MobiEndAnalyzerFactory* factory;
    bool checkHeader(const char* header, int32_t headersize) const; //krazy:exclude=typedefs
    signed char analyze(Strigi::AnalysisResult& idx, Strigi::InputStream* in);
    const char* name() const { return "MobiEndAnalyzer"; }
public:
    MobiEndAnalyzer(const MobiEndAnalyzerFactory* f);
};

class MobiEndAnalyzerFactory : public Strigi::StreamEndAnalyzerFactory {
friend class MobiEndAnalyzer;
private:
    const Strigi::RegisteredField* titleField;
    const Strigi::RegisteredField* authorField;
    const Strigi::RegisteredField* copyrightField;
    const Strigi::RegisteredField* subjectField;
    const Strigi::RegisteredField* descriptionField;
    const Strigi::RegisteredField* encryptedField;
//    const Strigi::RegisteredField* typeField;

    const char* name() const {
        return "MobiEndAnalyzer";
    }
    Strigi::StreamEndAnalyzer* newInstance() const {
        return new MobiEndAnalyzer(this);
    }
    void registerFields(Strigi::FieldRegister&);
};

#endif
