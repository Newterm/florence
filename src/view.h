/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009 François Agrech

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#ifndef FLO_VIEW
#define FLO_VIEW

#include "config.h"
#include <gtk/gtk.h>
#include <cairo.h>
#ifdef ENABLE_AT_SPI
#include <cspi/spi.h>
#endif
#include "key.h"
#include "style.h"
#include "status.h"

struct key;

/* This represents a view of florence. */
struct view {
	struct status *status; /* the status being represented by the view */
	GtkWindow *window; /* GTK window of the view */
	gboolean composite; /* true if the screen has composite extension */
	guint width, height; /* dimensions of the view, in pixels */
	gdouble vwidth, vheight; /* virtual dimensions of the view */
	gdouble zoom; /* scaling factor of the window */
	GSList *keyboards; /* Main list of keyboard extensions */
	gdouble xoffset, yoffset; /* offset of the main keyboard */
	struct style *style; /* Do it with style */
	cairo_surface_t *background; /* contains the background image of florence */
	cairo_surface_t *symbols; /* contains the symbols image of florence */
	GdkRegion *redraw; /* region that needs to be redrawn */
	gboolean hand_cursor; /* true when the cursor is a hand */
};

/* create a view of florence */
struct view *view_new (struct style *style, GSList *keyboards);
/* liberate all the memory used by the view */
void view_free (struct view *view);

#ifdef ENABLE_AT_SPI
/* Show the view next to the accessible object if specified. */
void view_show (struct view *view, Accessible *object);
#endif
/* Hides the view */
void view_hide (struct view *view);
/* Redraw the key to the window */
void view_update (struct view *view, struct key *key, gboolean statechange);
/* Change the layout and style of the view and redraw */
void view_update_layout(struct view *view, struct style *style, GSList *keyboards);

/* get the key at position */
struct key *view_hit_get (struct view *view, gint x, gint y);
/* get gtk window of the view */
GtkWindow *view_window_get (struct view *view);
/* get gtk window of the view */
void view_status_set (struct view *view, struct status *status);

/* on screen change event: check for composite extension */
void view_screen_changed (GtkWidget *widget, GdkScreen *old_screen, struct view *view);

#endif

