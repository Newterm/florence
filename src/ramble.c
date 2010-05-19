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
#include <math.h>

/* Maximum number of points in the ramble path */
#define RAMBLE_MAX_POINTS 100

/* Invalidate the region modified by the path */
void ramble_update_region(GList *path, GdkWindow *window)
{
	GdkRectangle rect;
	struct ramble_point *p1, *p2;

	if (path) {
		p1=(struct ramble_point *)path->data;
		if (path->prev) {
			p2=(struct ramble_point *)path->prev->data;
			if (p1->p.x > p2->p.x) {
				rect.x=p2->p.x;
				rect.width=p1->p.x-p2->p.x;
			} else {
				rect.x=p1->p.x;
				rect.width=p2->p.x-p1->p.x;
			}
			if (p1->p.y > p2->p.y) {
				rect.y=p2->p.y;
				rect.height=p1->p.y-p2->p.y;
			} else {
				rect.y=p1->p.y;
				rect.height=p2->p.y-p1->p.y;
			}
			rect.x=rect.x-10; rect.y=rect.y-10;
			rect.width=rect.width+20; rect.height=rect.height+20;
			gdk_window_invalidate_rect(window, &rect, TRUE);
		}
	}
}

/* Add a point to the path and update the window.
 * returns TRUE if an edge is detected. */
gboolean ramble_add(struct ramble *ramble, GdkWindow *window, gint x, gint y)
{
	struct ramble_point *pt=g_malloc(sizeof(struct ramble_point));
	gdouble w, h, velocity;
	GList *list;
	gboolean ret=FALSE;

	/* Add the point to the path */
	pt->p.x=x; pt->p.y=y; pt->a=0.0; pt->set=FALSE;
	ramble->end=g_list_append(ramble->end, (gpointer)pt);
	if (!ramble->path) ramble->path=ramble->end;
	if (ramble->end->next) ramble->end=ramble->end->next;
	ramble->n++;
	ramble_update_region(ramble->end, window);
	if (ramble->n > RAMBLE_MAX_POINTS) {
		ramble_update_region(ramble->path->next, window);
		g_free(ramble->path->data);
		ramble->path=g_list_delete_link(ramble->path, ramble->path);
		ramble->n--;
	}


	/* Gesture detection */
	/* Find a point at least 10 px away in order to get acceptable precision */
	pt=(struct ramble_point *)ramble->end->data; list=ramble->end;
	while (list) {
		w=pt->p.x-((struct ramble_point *)list->data)->p.x;
		h=pt->p.y-((struct ramble_point *)list->data)->p.y;
		if (sqrt((w*w)+(h*h))>=5.0) {
			/* Calculate the angle */
			pt->a=atan2(h, w); pt->set=TRUE;
			/* Calculate angle velocity */
			if (list->prev && ((struct ramble_point *)list->prev->data)->set) {
				velocity=pt->a-((struct ramble_point *)list->prev->data)->a;
				if (velocity<0.0) velocity=-velocity;
				if (velocity>M_PI) velocity=velocity-(2.0*M_PI);
				if (velocity<0.0) velocity=-velocity;
				if (velocity>(M_PI/6.0)) ret=TRUE;
			}
			list=NULL;
		} else list=list->prev;
	}

	return ret;
}

/* Draw the ramble path to the cairo context */
void ramble_draw(struct ramble *ramble, cairo_t *ctx)
{
	GList *list=ramble->end;
	struct ramble_point *p;
	if (list) {
		p=(struct ramble_point *)list->data;
		cairo_move_to(ctx, p->p.x, p->p.y);
		list=list->prev;
		while (list) {
			p=(struct ramble_point *)list->data;
			cairo_line_to(ctx, p->p.x, p->p.y);
			list=list->prev;
		}
		cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_cap (ctx, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(ctx, CAIRO_LINE_JOIN_ROUND);
		cairo_set_source_rgba (ctx, 1, 0, 0, 1);
		cairo_set_line_width(ctx, 5);
		cairo_stroke(ctx);
	}
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

