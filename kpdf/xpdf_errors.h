/***************************************************************************
 *   Copyright (C) 2004 by Andrew Coles <andrew_coles@yahoo.co.uk>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
 
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

