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
#include <string.h>

/* For stat */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "spectre-document.h"
#include "spectre-private.h"
#include "spectre-exporter.h"

struct SpectreDocument
{
	struct document *doc;
	
	SpectreStatus    status;

	int              structured;
};

SpectreDocument *
spectre_document_new (void)
{
	SpectreDocument *doc;

	doc = calloc (1, sizeof (SpectreDocument));
	return doc;
}

void
spectre_document_load (SpectreDocument *document,
		       const char      *filename)
{
	if (document->doc && strcmp (filename, document->doc->filename) == 0) {
		document->status = SPECTRE_STATUS_SUCCESS;
		return;
	}

	if (document->doc) {
		psdocdestroy (document->doc);
		document->doc = NULL;
	}
	
	document->doc = psscan (filename, SCANSTYLE_NORMAL);
	if (!document->doc) {
		/* FIXME: OOM | INVALID_PS */
		document->status = SPECTRE_STATUS_LOAD_ERROR;
		return;
	}

	document->structured = ((!document->doc->epsf && document->doc->numpages > 0) ||
				(document->doc->epsf && document->doc->numpages > 1));

	if (document->status != SPECTRE_STATUS_SUCCESS)
		document->status = SPECTRE_STATUS_SUCCESS;
}

void
spectre_document_free (SpectreDocument *document)
{
	if (!document)
		return;

	if (document->doc) {
		psdocdestroy (document->doc);
		document->doc = NULL;
	}

	free (document);
}

SpectreStatus
spectre_document_status (SpectreDocument *document)
{
	return document->status;
}

unsigned int
spectre_document_get_n_pages (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return 0;
	}
	
	return document->structured ? document->doc->numpages : 1;
}

SpectreOrientation
spectre_document_get_orientation (SpectreDocument *document)
{
	int doc_orientation;
	
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return SPECTRE_ORIENTATION_PORTRAIT;
	}

	doc_orientation = document->doc->orientation != NONE ?
		document->doc->orientation : document->doc->default_page_orientation;
	
	switch (doc_orientation) {
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

const char *
spectre_document_get_title (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return NULL;
	}

	return document->doc->title;
}

const char *
spectre_document_get_creator (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return NULL;
	}

	return document->doc->creator;
}

const char *
spectre_document_get_for (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return NULL;
	}

	return document->doc->fortext;
}

const char *
spectre_document_get_creation_date (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return NULL;
	}
	
	return document->doc->date;
}

const char *
spectre_document_get_format (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return NULL;
	}

	return document->doc->format;
}

int
spectre_document_is_eps (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return FALSE;
	}

	return document->doc->epsf;
}

unsigned int 
spectre_document_get_language_level (SpectreDocument *document)
{
	if (!document->doc) {
		document->status = SPECTRE_STATUS_DOCUMENT_NOT_LOADED;
		return 0;
	}

	return document->doc->languagelevel ? atoi (document->doc->languagelevel) : 0;
}

SpectrePage *
spectre_document_get_page (SpectreDocument *document,
			   unsigned int     page_index)
{
	SpectrePage *page;

	if (page_index >= spectre_document_get_n_pages (document)) {
		document->status = SPECTRE_STATUS_INVALID_PAGE;
		return NULL;
	}
	
	page = _spectre_page_new (page_index, document->doc);
	if (!page) {
		document->status = SPECTRE_STATUS_NO_MEMORY;
		return NULL;
	}

	if (document->status != SPECTRE_STATUS_SUCCESS)
		document->status = SPECTRE_STATUS_SUCCESS;
	
	return page;
}

void
spectre_document_save (SpectreDocument *document,
		       const char      *filename)
{
	struct stat stat_buf;
	FILE *from;
	FILE *to;
	
	if (stat (document->doc->filename, &stat_buf) != 0) {
		document->status = SPECTRE_STATUS_SAVE_ERROR;
		return;
	}

	from = fopen (document->doc->filename, "r");
	if (!from) {
		document->status = SPECTRE_STATUS_SAVE_ERROR;
		return;
	}

	to = fopen (filename, "w");
	if (!to) {
		document->status = SPECTRE_STATUS_SAVE_ERROR;
		fclose (from);
		return;
	}

	pscopy (from, to, document->doc, 0, stat_buf.st_size - 1);

	fclose (from);
	fclose (to);

	document->status = SPECTRE_STATUS_SUCCESS;
}

void
spectre_document_save_to_pdf (SpectreDocument *document,
			      const char      *filename)
{
	SpectreExporter *exporter;
	SpectreStatus    status;
	unsigned int     i;

	exporter = spectre_exporter_new (document, SPECTRE_EXPORTER_FORMAT_PDF);
	if (!exporter) {
		document->status = SPECTRE_STATUS_NO_MEMORY;
		return;
	}

	status = spectre_exporter_begin (exporter, filename);
	if (status) {
		document->status = status == SPECTRE_STATUS_NO_MEMORY ?
			SPECTRE_STATUS_NO_MEMORY : SPECTRE_STATUS_SAVE_ERROR;
		spectre_exporter_free (exporter);
		return;
	}

	for (i = 0; i < spectre_document_get_n_pages (document); i++) {
		status = spectre_exporter_do_page (exporter, i);
		if (status)
			break;
	}

	if (status) {
		document->status = status == SPECTRE_STATUS_NO_MEMORY ?
			SPECTRE_STATUS_NO_MEMORY : SPECTRE_STATUS_SAVE_ERROR;
		spectre_exporter_free (exporter);
		return;
	}
		
	status = spectre_exporter_end (exporter);
	spectre_exporter_free (exporter);

	if (status) {
		document->status = status == SPECTRE_STATUS_NO_MEMORY ?
			SPECTRE_STATUS_NO_MEMORY : SPECTRE_STATUS_SAVE_ERROR;
	} else {
		document->status = SPECTRE_STATUS_SUCCESS;
	}
}

struct document *
_spectre_document_get_doc (SpectreDocument *document)
{
	return document->doc;
}
