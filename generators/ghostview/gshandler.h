/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GSHANDLER_H_
#define _OKULAR_GSHANDLER_H_

#include "internaldocument.h"

#include "ghostscript/iapi.h"

class GSInterpreterCMD;

class GSHandler
{
	public:
		GSHandler();
		
		void init(const QString &media, 
		          double magnify,
		          int width,
		          int height,
		          bool plaformFonts,
		          int aaText,
		          int aaGfx,
		          GSInterpreterCMD *interpreter);
		
		void process(const QString &filename, const PsPosition &pos);
		
		// callbacks
		int size(void *device, int width, int height, int raster, unsigned int format, unsigned char *pimage);
		int page(void *device, int copies, int flush);

	private:
		gs_main_instance *m_ghostScriptInstance; //! Pointer to the Ghostscript library instance
		
		int m_GSwidth, m_GSheight;
		int m_GSraster; //! Size of a horizontal row (y-line) in the image buffer
		const unsigned char *m_GSimage; //! Image buffer we received from Ghostscript library
		
		GSInterpreterCMD *m_device;
};

#endif
