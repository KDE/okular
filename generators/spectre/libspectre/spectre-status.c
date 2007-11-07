/* This file is part of Libspectre.
 * 
 * Copyright (C) 2007 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 * Libspectre is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Libspectre is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "spectre-status.h"

const char *
spectre_status_to_string (SpectreStatus status)
{
	switch (status) {
	case SPECTRE_STATUS_SUCCESS:
		return "success";
	case SPECTRE_STATUS_NO_MEMORY:
		return "out of memory";
	case SPECTRE_STATUS_LOAD_ERROR:
		return "error loading document";
	case SPECTRE_STATUS_DOCUMENT_NOT_LOADED:
		return "document is not loaded";
	case SPECTRE_STATUS_INVALID_PAGE:
		return "page is invalid";
	case SPECTRE_STATUS_RENDER_ERROR:
		return "render error";
	case SPECTRE_STATUS_EXPORTER_ERROR:
		return "exporter error";
	case SPECTRE_STATUS_SAVE_ERROR:
		return "save error";
	}

	return "unknown error status";
}
