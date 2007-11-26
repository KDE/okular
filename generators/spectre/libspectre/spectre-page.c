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

#include <stdio.h>
#include <stdlib.h>

#include "spectre-page.h"

#include "spectre-device.h"
#include "spectre-private.h"

struct SpectrePage
{
	struct document *doc;
	
	SpectreStatus    status;

	unsigned int     index;
};

SpectrePage *
_spectre_page_new (unsigned int     page_index,
		   struct document *doc)
{
	SpectrePage *page;

	page = calloc (1, sizeof (SpectrePage));
	if (!page)
		return NULL;

	page->index = page_index;
	page->doc = psdocreference (doc);

	return page;
}

void
spectre_page_free (SpectrePage *page)
{
	if (!page)
		return;

	if (page->doc) {
		psdocdestroy (page->doc);
		page->doc = NULL;
	}
	
	free (page);
}

SpectreStatus
spectre_page_status (SpectrePage *page)
{
	return page->status;
}

unsigned int
spectre_page_get_index (SpectrePage *page)
{
	return page->index;
}

const char *
spectre_page_get_label (SpectrePage *page)
{
	return page->doc->numpages > 0 ? page->doc->pages[page->index].label : NULL;
}

SpectreOrientation
spectre_page_get_orientation (SpectrePage *page)
{
	int page_orientation = NONE;

	if (page->doc->numpages > 0) {
		page_orientation = page->doc->pages[page->index].orientation != NONE ?
			page->doc->pages[page->index].orientation :
			page->doc->default_page_orientation;
	}

	if (page_orientation == NONE)
		page_orientation = page->doc->orientation;

	switch (page_orientation) {
	default:
	case PORTRAIT:
		return SPECTRE_ORIENTATION_PORTRAIT;
	case LANDSCAPE:
		return SPECTRE_ORIENTATION_LANDSCAPE;
	case SEASCAPE:
		return SPECTRE_ORIENTATION_REVERSE_LANDSCAPE;
	case UPSIDEDOWN:
		return SPECTRE_ORIENTATION_REVERSE_PORTRAIT;
	}
}

void
spectre_page_get_size (SpectrePage *page,
		       int         *width,
		       int         *height)
{
	int urx, ury, llx, lly;
	
	psgetpagebox (page->doc, page->index,
		      &urx, &ury, &llx, &lly);

	if (width)
		*width = urx - llx;
	if (height)
		*height = ury - lly;
}

void
spectre_page_render (SpectrePage          *page,
		     SpectreRenderContext *rc,
		     unsigned char       **page_data,
		     int                  *row_length)
{
	SpectreDevice *device;

	if (!rc)
		return;

	device = spectre_device_new (page->doc);
	
	page->status = spectre_device_render (device, page->index, rc,
					      page_data, row_length);
	
	spectre_device_free (device);
}
