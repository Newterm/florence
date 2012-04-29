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
#include "settings.h"
#include "style.h"
#include <math.h>

/* Maximum number of points in the ramble path */
#define RAMBLE_MAX_POINTS 200

/* Invalidate the region modified by the path */
void ramble_update_region(GList *path, GdkWindow *window)
{
	START_FUNC
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
	END_FUNC
}

/* Detect gesture based on distance */
void ramble_distance(struct ramble *ramble)
{
	START_FUNC
	GList *list;
	gint dw, dh;
	gdouble d=0.0, w, h, threshold;
	struct ramble_point *pt, *pt1, *pt2;

	list=ramble->end;
	pt=(struct ramble_point *)ramble->end->data;
	pt1=pt;
	/* Calculate path distance on that key */
	while (list) {
		pt2=(struct ramble_point *)list->data;
		if ( (!pt2->ev) && (pt2->k==pt->k) ) {
			dw=pt1->p.x-pt2->p.x;
			dh=pt1->p.y-pt2->p.y;
			w=((gdouble)dw)/pt->k->w;
			h=((gdouble)dh)/pt->k->h;
			d+=sqrt((w*w)+(h*h));
			pt1=pt2;
			list=list->prev;
		} else {
			list=NULL;
		}
	}
	if (pt2->ev) threshold=settings_get_double(SETTINGS_RAMBLE_THRESHOLD2);
	else threshold=settings_get_double(SETTINGS_RAMBLE_THRESHOLD1);
	threshold*=(settings_get_double(SETTINGS_SCALEX)+settings_get_double(SETTINGS_SCALEY))/2.0;
	if (d>=threshold) {
		pt->ev=TRUE;
		list=NULL;
	}
	END_FUNC
}

/* Detect gesture based on time */
void ramble_time(struct ramble *ramble)
{
	START_FUNC
	struct ramble_point *pt;
	pt=(struct ramble_point *)ramble->end->data;
	if (pt->k) {
		/* TODO:
		 * reset the timer when pointer is near the border */
		if ( (!ramble->end->prev) ||
			(((struct ramble_point *)ramble->end->prev->data)->k!=pt->k) ) {
			if (ramble->timer)
				g_timer_start(ramble->timer);
			else
				ramble->timer=g_timer_new();
		} else if (g_timer_elapsed(ramble->timer, NULL)>=
			(settings_get_double(SETTINGS_RAMBLE_TIMER)/1000.0)) {
			pt->ev=TRUE;
			g_timer_start(ramble->timer);
			g_timer_stop(ramble->timer);
		}
	} else if (ramble->timer) {
		g_timer_destroy(ramble->timer);
		ramble->timer=NULL;
	}
	END_FUNC
}

/* Reset timer */
void ramble_time_reset(struct ramble *ramble)
{
	START_FUNC
	g_timer_start(ramble->timer);
	END_FUNC
}

/* Add a point to the path and update the window.
 * returns TRUE if an event is detected. */
gboolean ramble_add(struct ramble *ramble, GdkWindow *window, gint x, gint y, struct key *k)
{
	START_FUNC
	gchar *val;
	struct ramble_point *pt=g_malloc(sizeof(struct ramble_point));

	/* Add the point to the path */
	pt->p.x=x; pt->p.y=y; pt->k=k; pt->ev=FALSE;
	ramble->end=g_list_append(ramble->end, (gpointer)pt);
	if (!ramble->path) {
		ramble->path=ramble->end;
		pt->ev=TRUE;
	}
	if (ramble->end->next) ramble->end=ramble->end->next;
	ramble->n++;
	ramble_update_region(ramble->end, window);
	if (ramble->n > RAMBLE_MAX_POINTS) {
		ramble_update_region(ramble->path->next, window);
		g_free(ramble->path->data);
		ramble->path=g_list_delete_link(ramble->path, ramble->path);
		ramble->n--;
	}
	if (!k) return FALSE;

	/* Gesture detection */
	val=settings_get_string(SETTINGS_RAMBLE_ALGO);
	if (!strcmp("distance", val))
		ramble_distance(ramble);
	else if (!strcmp("time", val))
		ramble_time(ramble);
	else {
		flo_warn(_("Invalid ramble algorithm selected. Using default."));
		ramble_distance(ramble);
	}
	if (val) g_free(val);

	ramble->started=TRUE;
	END_FUNC
	return pt->ev;
}

/* Start rambling. Note: when ramble_button is FALSE, ramble is always started.
 * returns TRUE if an event is detected. */
gboolean ramble_start(struct ramble *ramble, GdkWindow *window, gint x, gint y, struct key *k)
{
	START_FUNC
	ramble->started=TRUE;
	END_FUNC
	return ramble_add(ramble, window, x, y, k);
}

/* Return TRUE if rambling is started */
gboolean ramble_started(struct ramble *ramble)
{
	START_FUNC
	END_FUNC
	return (!settings_get_bool(SETTINGS_RAMBLE_BUTTON)) || ramble->started;
}

/* Reset ramble path
 * returns TRUE if an event is detected. */
gboolean ramble_reset(struct ramble *ramble, GdkWindow *window, struct key *k)
{
	START_FUNC
	GdkRectangle *rect=NULL;
	GList *list=ramble->path;
	struct ramble_point *pt;
	struct key *last=NULL;
	while (list) {
		pt=(struct ramble_point *)list->data;
		if (!rect) {
			rect=g_malloc(sizeof(GdkRectangle));
			rect->x=pt->p.x;
			rect->y=pt->p.y;
			rect->width=0;
			rect->height=0;
		} else {
			if (pt->p.x<rect->x) { rect->width+=(rect->x-pt->p.x); rect->x=pt->p.x; }
			else rect->width+=(pt->p.x-rect->x);
			if (pt->p.y<rect->y) { rect->height+=(rect->y-pt->p.y); rect->y=pt->p.y; }
			else rect->height+=(pt->p.y-rect->y);
		}
		if (pt->ev) last=pt->k;
		g_free(list->data);
		list=list->next;
	}
	g_list_free(ramble->path);
	if (rect) {
		rect->x=rect->x-10; rect->y=rect->y-10;
		rect->width=rect->width+20; rect->height=rect->height+20;
		gdk_window_invalidate_rect(window, rect, TRUE);
		g_free(rect);
	}
	ramble->path=NULL;
	ramble->end=NULL;
	ramble->n=0;
	ramble->started=FALSE;
	END_FUNC
	return (last!=k);
}

/* Draw the ramble path to the cairo context */
void ramble_draw(struct ramble *ramble, cairo_t *ctx)
{
	START_FUNC
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
		cairo_set_operator(ctx, CAIRO_OPERATOR_OVER);
		cairo_set_line_cap (ctx, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(ctx, CAIRO_LINE_JOIN_ROUND);
		style_cairo_set_color(ctx, STYLE_RAMBLE_COLOR);
		cairo_set_line_width(ctx, 5);
		cairo_stroke(ctx);
	}
	END_FUNC
}

/* called when the input method changes */
void ramble_input_method_check(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct ramble *ramble=(struct ramble *)user_data;
	gchar *val=settings_get_string(SETTINGS_INPUT_METHOD);
	if (strcmp("ramble", val))
		ramble_reset(ramble, NULL, NULL);
	if (val) g_free(val);
	END_FUNC
}

/* Create a ramble structure */
struct ramble *ramble_new()
{
	START_FUNC
	struct ramble *ramble=g_malloc(sizeof(struct ramble));
	if (!ramble) flo_fatal(_("Unable to allocate memory for ramble"));
	memset(ramble, 0, sizeof(struct ramble));
	settings_changecb_register(SETTINGS_INPUT_METHOD, ramble_input_method_check, ramble);
	END_FUNC
	return ramble;
}

/* Destroy a ramble structure */
void ramble_free(struct ramble *ramble)
{
	START_FUNC
	GList *list=ramble->path;
	while (list) {
		g_free(list->data);
		list=list->next;
	}
	g_list_free(ramble->path);
	g_free(ramble);
	END_FUNC
}

#endif

