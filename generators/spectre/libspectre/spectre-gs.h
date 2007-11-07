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

#ifndef SPECTRE_GS_H
#define SPECTRE_GS_H

#include <libspectre/spectre-macros.h>

#include "ps.h"

SPECTRE_BEGIN_DECLS

typedef enum {
	CLEANUP_DELETE_INSTANCE = 1 << 0,
	CLEANUP_EXIT            = 1 << 1
} SpectreGSCleanupFlag;

typedef struct SpectreGS SpectreGS;

SpectreGS *spectre_gs_new                  (void);
int        spectre_gs_create_instance      (SpectreGS           *gs,
					    void                *caller_handle);
int        spectre_gs_set_display_callback (SpectreGS           *gs,
					    void                *callback);
int        spectre_gs_run                  (SpectreGS           *gs,
					    int                  n_args,
					    char               **args);
int        spectre_gs_send_string          (SpectreGS           *gs,
					    const char          *str);
int        spectre_gs_send_page            (SpectreGS           *gs,
					    struct document     *doc,
					    unsigned int         page_index);
void       spectre_gs_cleanup              (SpectreGS           *gs,
					    SpectreGSCleanupFlag flag);
void       spectre_gs_free                 (SpectreGS           *gs);
	
SPECTRE_END_DECLS

#endif /* SPECTRE_GS_H */
