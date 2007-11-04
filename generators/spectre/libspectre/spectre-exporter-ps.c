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

#include "spectre-private.h"

static SpectreStatus
spectre_exporter_ps_begin (SpectreExporter *exporter,
			   const char      *filename)
{
	exporter->from = fopen (exporter->doc->filename, "r");
	exporter->to = fopen (filename, "w");

	pscopyheaders (exporter->from, exporter->to, exporter->doc);
	
	return SPECTRE_STATUS_SUCCESS;
}

static SpectreStatus
spectre_exporter_ps_do_page (SpectreExporter *exporter,
			     unsigned int     page_index)
{
	if (exporter->doc->numpages <= 0)
		return SPECTRE_STATUS_SUCCESS;
	
	pscopypage (exporter->from, exporter->to, exporter->doc,
		    page_index, exporter->n_pages++);
	
	return SPECTRE_STATUS_SUCCESS;
}

static SpectreStatus
spectre_exporter_ps_end (SpectreExporter *exporter)
{
	pscopytrailer (exporter->from, exporter->to, exporter->doc,
		       exporter->n_pages);

	fclose (exporter->from);
	exporter->from = NULL;
	fclose (exporter->to);
	exporter->to = NULL;
	
	return SPECTRE_STATUS_SUCCESS;
}

SpectreExporter *
_spectre_exporter_ps_new (struct document *doc)
{
	SpectreExporter *exporter;

	exporter = calloc (1, sizeof (SpectreExporter));
	if (!exporter)
		return NULL;

	exporter->doc = psdocreference (doc);

	exporter->begin = spectre_exporter_ps_begin;
	exporter->do_page = spectre_exporter_ps_do_page;
	exporter->end = spectre_exporter_ps_end;

	return exporter;
}
