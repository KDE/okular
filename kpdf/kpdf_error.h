/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid                               *
 *   tsdgeos@terra.es                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
 
#ifndef KPDF_ERROR_H
#define KPDF_ERROR_H

#include <qstringlist.h>

class errors
{
	public:
		static void add(const QString &s); 
		static bool exists(const QString &s); 
		static void clear(); 
	
	private:
		static QStringList p_errors;
};

#endif
