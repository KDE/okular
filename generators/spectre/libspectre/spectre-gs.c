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

#include "spectre-gs.h"
#include "spectre-utils.h"

/* ghostscript stuff */
#include "ghostscript/iapi.h"
#include "ghostscript/ierrors.h"

#define BUFFER_SIZE 32768

struct SpectreGS {
	void *ghostscript_instance;
};

static int
critic_error_code (int code)
{
	if (code >= 0)
		return FALSE;
	
	if (code <= -100) {
		switch (code) {
			case e_Fatal:
				fprintf (stderr, "fatal internal error %d", code);
				return TRUE;
				break;

			case e_ExecStackUnderflow:
				fprintf (stderr, "stack overflow %d", code);
				return TRUE;
				break;

			/* no error or not important */
			default:
				return FALSE;
		}
	} else {
		const char *errors[] = { "", ERROR_NAMES };
		int x = (-1) * code;

		if (x < (int) (sizeof (errors) / sizeof (const char*))) {
			fprintf (stderr, "%s %d\n", errors[x], code);
		}
		return TRUE;
	}
}

static int
spectre_gs_process (SpectreGS  *gs,
		    const char *filename,
		    int         x,
		    int         y,
		    long        begin,
		    long        end)
{
	FILE *fd;
	int error;
	static char buf[BUFFER_SIZE];
	unsigned int read;
	int wrote;
	size_t left = end - begin;
	void *ghostscript_instance = gs->ghostscript_instance;
	
	fd = fopen (filename, "r");
	if (!fd) {
		return FALSE;
	}
	
	fseek (fd, begin, SEEK_SET);

	gsapi_run_string_begin (ghostscript_instance, 0, &error);
	if (critic_error_code (error)) {
		fclose (fd);
		return FALSE;
	}

	if (x != 0 && y != 0) {
		char *set;

		set = _spectre_strdup_printf ("%d %d translate\n", -x, -y);
		gsapi_run_string_continue (ghostscript_instance, set, strlen (set),
					   0, &error);
		free (set);
		if (critic_error_code (error)) {
			fclose (fd);
			return FALSE;
		}
	}

	while (left > 0 && !critic_error_code (error)) {
		size_t to_read = BUFFER_SIZE;
		
		if (left < to_read)
			to_read = left;
		
		read = fread (buf, sizeof (char), to_read, fd);
		wrote = gsapi_run_string_continue (ghostscript_instance,
						   buf, read, 0, &error);
		left -= read;
	}
	
	fclose (fd);
	if (critic_error_code (error))
		return FALSE;
	
	gsapi_run_string_end (ghostscript_instance, 0, &error);
	if (critic_error_code (error))
		return FALSE;

	return TRUE;
}

SpectreGS *
spectre_gs_new (void)
{
	SpectreGS *gs;

	gs = calloc (1, sizeof (SpectreGS));
	
	return gs;
}

int
spectre_gs_create_instance (SpectreGS *gs,
			    void      *caller_handle)
{
	int error;
	
	error = gsapi_new_instance (&gs->ghostscript_instance, caller_handle);
	
	return !critic_error_code (error);
}

int
spectre_gs_set_display_callback (SpectreGS *gs,
				 void      *callback)
{
	int error;
	
	error = gsapi_set_display_callback (gs->ghostscript_instance,
					    callback);
	return !critic_error_code (error);
}

int
spectre_gs_run (SpectreGS *gs,
		int        n_args,
		char     **args)
{
	int error;
	
	error = gsapi_init_with_args (gs->ghostscript_instance, n_args, args);

	return !critic_error_code (error);
}

int
spectre_gs_send_string (SpectreGS  *gs,
			const char *str)
{
	int error;
	
	gsapi_run_string_with_length (gs->ghostscript_instance,
				      str, strlen (str), 0, &error);

	return !critic_error_code (error);
}

int
spectre_gs_send_page (SpectreGS       *gs,
		      struct document *doc,
		      unsigned int     page_index)
{
	int doc_llx = 0, doc_lly = 0;
	int page_llx = 0, page_lly = 0;

	if ((doc->boundingbox[URX] > doc->boundingbox[LLX]) &&
	    (doc->boundingbox[URY] > doc->boundingbox[LLY])) {
		doc_llx = doc->boundingbox[LLX];
		doc_lly = doc->boundingbox[LLY];
	}

	if (doc->numpages > 0 &&
	    (doc->pages[page_index].boundingbox[URX] >
	     doc->pages[page_index].boundingbox[LLX]) &&
	    (doc->pages[page_index].boundingbox[URY] >
	     doc->pages[page_index].boundingbox[LLY])) {
		/* Do not translate twice */
		if (doc->pages[page_index].boundingbox[LLX] != doc_llx &&
		    doc->pages[page_index].boundingbox[LLY] != doc_lly) {
			page_llx =  doc->pages[page_index].boundingbox[LLX];
			page_lly = doc->pages[page_index].boundingbox[LLY];
		}
	}

	if (!spectre_gs_process (gs,
				 doc->filename,
				 doc_llx, doc_lly,
				 doc->beginprolog,
				 doc->endprolog))
		return FALSE;
	
	if (!spectre_gs_process (gs,
				 doc->filename,
				 0, 0,
				 doc->beginsetup,
				 doc->endsetup))
		return FALSE;
	
	if (doc->numpages > 0) {
		if (!spectre_gs_process (gs,
					 doc->filename,
					 page_llx, page_lly,
					 doc->pages[page_index].begin,
					 doc->pages[page_index].end))
			return FALSE;
	}
	
	if (!spectre_gs_process (gs,
				 doc->filename,
				 0, 0,
				 doc->begintrailer,
				 doc->endtrailer))
		return FALSE;

	return TRUE;
}

void
spectre_gs_cleanup (SpectreGS           *gs,
		    SpectreGSCleanupFlag flag)
{
	if (gs->ghostscript_instance == NULL)
		return;

	if (flag & CLEANUP_EXIT)
		gsapi_exit (gs->ghostscript_instance);

	if (flag & CLEANUP_DELETE_INSTANCE)
		gsapi_delete_instance (gs->ghostscript_instance);

	gs->ghostscript_instance = NULL;
}

void
spectre_gs_free (SpectreGS *gs)
{
	if (!gs)
		return;

	spectre_gs_cleanup (gs,
			    CLEANUP_DELETE_INSTANCE |
			    CLEANUP_EXIT);
	free (gs);
}
