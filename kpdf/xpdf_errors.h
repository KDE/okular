#ifndef XPDF_ERRORS_H
#define XPDF_ERRORS_H

#include <qmap.h>
#include <qstring.h>

class XPDFErrorTranslator {

private:

	static bool mapNotInitialised;
	static QMap<QString, QString> translationmap;

public:
	static QString translateError(const QString & originalError);

};

#endif

