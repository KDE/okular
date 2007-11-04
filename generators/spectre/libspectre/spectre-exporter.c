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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>

#include "spectre-exporter.h"
#include "spectre-private.h"

SpectreExporter *
spectre_exporter_new (SpectreDocument      *document,
		      SpectreExporterFormat format)
{
	struct document *doc;
	
	if (!document)
		return NULL;

	doc = _spectre_document_get_doc (document);
	
	switch (format) {
	case SPECTRE_EXPORTER_FORMAT_PS:
		return _spectre_exporter_ps_new (doc);
	case SPECTRE_EXPORTER_FORMAT_PDF:
		return _spectre_exporter_pdf_new (doc);
	}

	return NULL;
}

void
spectre_exporter_free (SpectreExporter *exporter)
{
	if (!exporter)
		return;

	if (exporter->doc) {
		psdocdestroy (exporter->doc);
		exporter->doc = NULL;
	}

	if (exporter->gs) {
		spectre_gs_free (exporter->gs);
		exporter->gs = NULL;
	}

	if (exporter->from) {
		fclose (exporter->from);
		exporter->from = NULL;
	}

	if (exporter->to) {
		fclose (exporter->to);
		exporter->to = NULL;
	}

	free (exporter->doc);
}

SpectreStatus
spectre_exporter_begin (SpectreExporter *exporter,
			const char      *filename)
{
	if (exporter->begin)
		return exporter->begin (exporter, filename);

	return SPECTRE_STATUS_SUCCESS;
}

SpectreStatus
spectre_exporter_do_page (SpectreExporter *exporter,
			  unsigned int     page_index)
{
	return exporter->do_page (exporter, page_index);
}

SpectreStatus
spectre_exporter_end (SpectreExporter *exporter)
{
	if (exporter->end)
		return exporter->end (exporter);
	
	return SPECTRE_STATUS_SUCCESS;		
}
