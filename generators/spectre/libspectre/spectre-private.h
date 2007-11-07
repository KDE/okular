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

#ifndef SPECTRE_PRIVATE_H
#define SPECTRE_PRIVATE_H

#include "spectre-macros.h"
#include "spectre-status.h"
#include "spectre-document.h"
#include "spectre-page.h"
#include "spectre-exporter.h"
#include "spectre-gs.h"
#include "ps.h"

SPECTRE_BEGIN_DECLS

struct SpectreRenderContext {
	double             scale;
	SpectreOrientation orientation;
	double             x_dpi;
	double             y_dpi;
	int                width;
	int                height;
	int                text_alpha_bits;
	int                graphic_alpha_bits;
	int                use_platform_fonts;
};

struct SpectreExporter {
	struct document *doc;

	/* PDF specific */
	SpectreGS       *gs;

	/* PS specific */
	FILE            *from;
	FILE            *to;
	int              n_pages;

	
	SpectreStatus (* begin)   (SpectreExporter *exporter,
				   const char      *filename);
	SpectreStatus (* do_page) (SpectreExporter *exporter,
				   unsigned int     page_index);
	SpectreStatus (* end)     (SpectreExporter *exporter);
};

SpectrePage     *_spectre_page_new         (unsigned int     page_index,
					    struct document *doc);
struct document *_spectre_document_get_doc (SpectreDocument *document);
SpectreExporter *_spectre_exporter_ps_new  (struct document *doc);
SpectreExporter *_spectre_exporter_pdf_new (struct document *doc);

SPECTRE_END_DECLS

#endif /* SPECTRE_PRIVATE_H */
