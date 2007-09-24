/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "gshandler.h"

#include "interpreter_cmd.h"

#include "ghostscript/ierrors.h"
#include "ghostscript/gdevdsp.h"

#include <kdebug.h>

#include <QFile>
#include <QImage>

static bool handleErrorCode(int code)
{
	if ( code>=0 )
		return true;
	else if ( code <= -100 )
	{
		switch (code)
		{
			case e_Fatal:
				kDebug() << "fatal internal error " << code;
				break;

			case e_ExecStackUnderflow:
				kDebug() << "stack overflow " << code;
				break;

			// no error or not important
			default:
				return true;
		}
	}
	else
	{
		const char* errors[]= { "", ERROR_NAMES };
		int x=(-1)*code;
		if (x < sizeof(errors)/sizeof(const char*)) {
			kDebug() << errors[x] << " " << code;
		}
		return false;
	}
	return true;
}

static int open(void * /*handle*/, void * /*device*/)
{
	return 0;
}

static int preclose(void * /*handle*/, void * /*device*/ )
{
	return 0;
}

static int close(void * /*handle*/, void * /*device*/ )
{
	return 0;
}

static int presize(void * /*handle*/, void * /*device*/, int /*width*/, int /*height*/,
        int /*raster*/, unsigned int /*format*/)
{
	return 0;
}

static int size(void *handle, void *device, int width, int height, int raster, unsigned int format, unsigned char *pimage)
{
	if (handle != 0)
		return static_cast <GSHandler*>(handle) -> size (device, width, height, raster, format, pimage);
	return 0;
}

static int sync(void * /*handle*/, void * /*device*/  )
{
	return 0;
}

static int page(void *handle, void * device, int copies, int flush)
{
	if (handle != 0)
	    return static_cast <GSHandler*>(handle) -> page (device, copies, flush);
	return 0;
}

static display_callback device =
{
	sizeof( display_callback ),
	DISPLAY_VERSION_MAJOR,
	DISPLAY_VERSION_MINOR,
	&open,
	&preclose,
	&close,
	&presize,
	&size,
	&sync,
	&page,
	0,
	0,
	0
};

GSHandler::GSHandler()
{
	m_ghostScriptInstance = 0;
}

void GSHandler::init(const QString &media, double magnify, int width, int height, int orientation, bool plaformFonts, int aaText, int aaGfx, GSInterpreterCMD *interpreter)
{
	int errorCode;
	
	if (m_ghostScriptInstance)
	{
		gsapi_exit(m_ghostScriptInstance);
		gsapi_delete_instance(m_ghostScriptInstance);
	}
	
	errorCode = gsapi_new_instance(&m_ghostScriptInstance, this);
	handleErrorCode(errorCode);
	
	errorCode = gsapi_set_display_callback(m_ghostScriptInstance, &device);
	handleErrorCode(errorCode);
	
	QStringList internalArgs;
	internalArgs << " "
		<< "-dMaxBitmap=10000000"
		<< "-dDELAYSAFER"
		<< "-dNOPAUSE"
		<< "-dNOPAGEPROMPT"
		<< QString("-dTextAlphaBits=%1").arg(aaText)
		<< QString("-dGraphicsAlphaBits=%1").arg(aaGfx)
		<< QString().sprintf("-g%dx%d", width, height)
		<< QString().sprintf("-r%fx%f", (magnify * Okular::Utils::dpiX()),
		                                (magnify * Okular::Utils::dpiY()))
		<< QString().sprintf("-dDisplayFormat=%d", DISPLAY_COLORS_RGB | DISPLAY_UNUSED_LAST | DISPLAY_DEPTH_8 | DISPLAY_LITTLEENDIAN | DISPLAY_TOPFIRST)
		<< QString().sprintf("-sDisplayHandle=16#%llx", (unsigned long long int) this );

	if (!media.isEmpty()) internalArgs << QString("-sPAPERSIZE=%1").arg(media.toLower());

	if ( !plaformFonts )
		internalArgs << "-dNOPLATFONTS";
	
	qDebug() << internalArgs;
	
	int t = internalArgs.count();
	char **args = new char*[t];
	for (int i = 0; i < t; ++i)
	{
		args[i] = new char[internalArgs[i].length() + 1];
		qstrcpy(*(args+i), internalArgs[i].toLocal8Bit());
	}
	
	errorCode = gsapi_init_with_args(m_ghostScriptInstance, t, args);
	handleErrorCode(errorCode);
	
	for (int i = 0; i < t; ++i) delete[] args[i];
	
	QString set = QString("<< /Orientation %1 >> setpagedevice .locksafe").arg(orientation);
	gsapi_run_string_with_length(m_ghostScriptInstance, set.toLatin1().constData(), set.length(), 0, &errorCode);
	handleErrorCode(errorCode);
	
	m_device = interpreter;
}

void GSHandler::process(const QString &filename, const PsPosition &pos)
{
	int errorCode;

	QFile f(filename);
	f.open(QIODevice::ReadOnly);
	f.seek(pos.first);
	static char buf[32768];
	unsigned int read;
	int wrote;
	size_t left = pos.second - pos.first;
	gsapi_run_string_begin(m_ghostScriptInstance, 0, &errorCode);
	handleErrorCode(errorCode);
	while (left > 0)
	{
		read = f.read(buf, qMin(sizeof(buf),left));
		wrote = gsapi_run_string_continue(m_ghostScriptInstance, buf, read, 0, &errorCode);
		handleErrorCode(errorCode);
		left -= read;
	}
	f.close();
	gsapi_run_string_end(m_ghostScriptInstance, 0, &errorCode);
}

int GSHandler::size(void * /*device*/, int width, int height, int raster, unsigned int /*format*/, unsigned char *pimage)
{
	m_GSwidth = width;
	m_GSheight = height;
	m_GSraster = raster;
	m_GSimage = pimage;
	
	return 0;
}

int GSHandler::page(void * /*device*/, int /*copies*/, int /*flush*/)
{
	QImage img;
	if (m_GSraster == m_GSwidth * 4)
	{
		img = QImage(m_GSimage, m_GSwidth, m_GSheight, QImage::Format_RGB32 );
	}
	else
	{
		// In case this ends up beign very slow we can try with some memmove
		QImage aux(m_GSimage, m_GSraster / 4, m_GSheight, QImage::Format_RGB32 );
		img = QImage(aux.copy(0, 0, m_GSwidth, m_GSheight));
	}
	m_device->fordwardImage(new QImage(img.copy()));
	return 0;
}
