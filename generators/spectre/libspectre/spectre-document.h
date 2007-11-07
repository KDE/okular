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

#ifndef SPECTRE_DOCUMENT_H
#define SPECTRE_DOCUMENT_H

#include <libspectre/spectre-macros.h>
#include <libspectre/spectre-status.h>
#include <libspectre/spectre-page.h>

SPECTRE_BEGIN_DECLS

/*! This is the object that represents a PostScript document. */
typedef struct SpectreDocument SpectreDocument;

/*! Creates a document */
SpectreDocument   *spectre_document_new                (void);

/*! Loads a the given file into the document. This function can fail
    @param document the document where the file will be loaded
    @param filename the file to loa
    @see spectre_document_status
*/
void               spectre_document_load               (SpectreDocument *document,
							const char      *filename);

/*! Returns the document status 
    @param document the document whose status will be returned
*/
SpectreStatus      spectre_document_status             (SpectreDocument *document);

/*! Frees the document
    @param document the document that will be freed
*/
void               spectre_document_free               (SpectreDocument *document);

/*! Returns the number of pages of the document. This function can fail
    @param document the document whose pages number will be returned
    @see spectre_document_status
*/
unsigned int       spectre_document_get_n_pages        (SpectreDocument *document);

/*! Returns the orientation of the document. This function can fail
    @param document the document whose orientation will be returned
    @see spectre_document_status
*/
SpectreOrientation spectre_document_get_orientation    (SpectreDocument *document);

/*! Returns the title of the document. It returns a null const char * if
    the document specifies no title. This function can fail
    @param document the document whose title will be returned
    @see spectre_document_status
*/
const char        *spectre_document_get_title          (SpectreDocument *document);

/*! Returns the creator of the document. It returns a null const char * if
    the document specifies no creator. This function can fail
    @param document the document whose creator will be returned
    @see spectre_document_status
*/
const char        *spectre_document_get_creator        (SpectreDocument *document);

/*! Returns the for of the document. It returns a null const char * if
    the document specifies no for. This function can fail
    @param document the document whose for will be returned
    @see spectre_document_status
*/
const char        *spectre_document_get_for            (SpectreDocument *document);

/*! Returns the creation date of the document. The date is copied verbatim from
    the document, so no format can be assumed on it. It returns a null const char * if
    the document specifies no creation date. This function can fail
    @param document the document whose creation date will be returned
    @see spectre_document_status
*/
const char        *spectre_document_get_creation_date  (SpectreDocument *document);

/*! Returns the format of the document. This function can fail
    @param document the document whose format will be returned
    @see spectre_document_status
*/
const char        *spectre_document_get_format         (SpectreDocument *document);

/*! Returns if the document is a Encapsulated PostScript file. This function can fail
    @param document the document to query
    @see spectre_document_status
*/
int                spectre_document_is_eps             (SpectreDocument *document);

/*! Returns the PostScript language level of the document. It returns 0 if no
    language level was defined on the file. This function can fail
    @param document the document whose language level will be returned
    @see spectre_document_status
*/
unsigned int       spectre_document_get_language_level (SpectreDocument *document);

/*! Returns a page of the document. This function can fail
    @param document the document whose page will be returned
    @param page_index the page index to get. First page has index 0.
    @see spectre_document_status
*/
SpectrePage       *spectre_document_get_page           (SpectreDocument *document,
							unsigned int     page_index);
/*! Save document as filename. This function can fail
    @param document the document that will be saved
    @param filename the path where document will be saved
    @see spectre_document_status
*/
void               spectre_document_save               (SpectreDocument *document,
							const char      *filename);
/* Save document as a pdf document. This function can fail
   @param document the document that will be saved
   @param filename the path where document will be saved as pdf
   @see spectre_document_status
*/
void               spectre_document_save_to_pdf        (SpectreDocument *document,
							const char      *filename);
SPECTRE_END_DECLS

#endif /* SPECTRE_DOCUMENT_H */
