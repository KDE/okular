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

#include <stdlib.h>

#include "spectre-private.h"
#include "spectre-utils.h"

static SpectreStatus
spectre_exporter_pdf_begin (SpectreExporter *exporter,
			    const char      *filename)
{
	char *args[8];
	int arg = 0;
	char *output_file;
	
	exporter->gs = spectre_gs_new ();
	if (!spectre_gs_create_instance (exporter->gs, NULL)) {
		spectre_gs_cleanup (exporter->gs, CLEANUP_DELETE_INSTANCE);
		spectre_gs_free (exporter->gs);
		exporter->gs = NULL;

		return SPECTRE_STATUS_EXPORTER_ERROR;
	}

	args[arg++] = "-dMaxBitmap=10000000";
	args[arg++] = "-dBATCH";
	args[arg++] = "-dNOPAUSE";
	args[arg++] = "-dSAFER";
	args[arg++] = "-sDEVICE=pdfwrite";
	args[arg++] = output_file = _spectre_strdup_printf ("-sOutputFile=%s",
							    filename);
	args[arg++] = "-c";
	args[arg++] = ".setpdfwrite";

	if (!spectre_gs_run (exporter->gs, 8, args)) {
		free (output_file);
		spectre_gs_free (exporter->gs);
		exporter->gs = NULL;

		return SPECTRE_STATUS_EXPORTER_ERROR;
	}

	free (output_file);

	return SPECTRE_STATUS_SUCCESS;
}

static SpectreStatus
spectre_exporter_pdf_do_page (SpectreExporter *exporter,
			      unsigned int     page_index)
{
	if (!spectre_gs_send_page (exporter->gs, exporter->doc, page_index)) {
		spectre_gs_free (exporter->gs);
		exporter->gs = NULL;

		return SPECTRE_STATUS_EXPORTER_ERROR;
	}
	
	return SPECTRE_STATUS_SUCCESS;
}

static SpectreStatus
spectre_exporter_pdf_end (SpectreExporter *exporter)
{
	spectre_gs_free (exporter->gs);
	exporter->gs = NULL;

	return SPECTRE_STATUS_SUCCESS;
}

SpectreExporter *
_spectre_exporter_pdf_new (struct document *doc)
{
	SpectreExporter *exporter;

	exporter = calloc (1, sizeof (SpectreExporter));
	if (!exporter)
		return NULL;

	exporter->doc = psdocreference (doc);

	exporter->begin = spectre_exporter_pdf_begin;
	exporter->do_page = spectre_exporter_pdf_do_page;
	exporter->end = spectre_exporter_pdf_end;

	return exporter;
}
