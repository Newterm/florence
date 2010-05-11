/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008, 2009, 2010 François Agrech

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#include "ramble.h"
#ifdef ENABLE_RAMBLE
#include "trace.h"
#include <gdk/gdk.h>

/* Maximum number of points in the ramble path */
#define RAMBLE_MAX_POINTS 100

/* Add a point to the path and update the view */
void ramble_add(struct ramble *ramble, struct view *view, gint x, gint y)
{
	GdkPoint *pt;
	pt=g_malloc(sizeof(GdkPoint));
	pt->x=x; pt->y=y;
	ramble->end=g_list_append(ramble->end, (gpointer)pt);
	if (!ramble->path) ramble->path=ramble->end;
	if (ramble->end->next) ramble->end=ramble->end->next;
	ramble->n++;
	if (ramble->n > RAMBLE_MAX_POINTS) {
		view_update_path(view, ramble->path->next);
		g_free(ramble->path->data);
		ramble->path=g_list_delete_link(ramble->path, ramble->path);
		ramble->n--;
	}
	view->path=ramble->end;
	view_update_path(view, ramble->end);
}

/* Create a ramble structure */
struct ramble *ramble_new()
{
	struct ramble *ramble=g_malloc(sizeof(struct ramble));
	if (!ramble) flo_fatal(_("Unable to allocate memory for ramble"));
	memset(ramble, 0, sizeof(struct ramble));
	return ramble;
}

/* Destroy a ramble structure */
void ramble_free(struct ramble *ramble)
{
	GList *list=ramble->path;
	while (list) { g_free(list->data); list=list->next; }
	g_list_free(ramble->path);
}

#endif

