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

#ifndef SPECTRE_EXPORTER_H
#define SPECTRE_EXPORTER_H

#include <libspectre/spectre-macros.h>
#include <libspectre/spectre-document.h>
#include <libspectre/spectre-status.h>

SPECTRE_BEGIN_DECLS

typedef enum {
	SPECTRE_EXPORTER_FORMAT_PS,
	SPECTRE_EXPORTER_FORMAT_PDF
} SpectreExporterFormat;

typedef struct SpectreExporter SpectreExporter;

SpectreExporter *spectre_exporter_new     (SpectreDocument      *document,
					   SpectreExporterFormat format);
void             spectre_exporter_free    (SpectreExporter      *exporter);
SpectreStatus    spectre_exporter_begin   (SpectreExporter      *exporter,
					   const char           *filename);
SpectreStatus    spectre_exporter_do_page (SpectreExporter      *exporter,
					   unsigned int          page_index);
SpectreStatus    spectre_exporter_end     (SpectreExporter      *exporter);


SPECTRE_END_DECLS

#endif /* SPECTRE_EXPORTER_H */
