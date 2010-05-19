/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009, 2010 François Agrech

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

#ifndef RAMBLE
#define RAMBLE

#include "system.h"
#ifdef ENABLE_RAMBLE
#include <glib.h>
#include <gdk/gdk.h>

struct ramble_point {
	GdkPoint p; /* Point coordinates */
	gdouble a; /* Angle relative to the previous point (10px away) */
	gboolean set; /* TRUE if angle is set */
};

/* Ramble structure is used to track the path of the mouse. */
struct ramble {
	GList *path; /* This is a list of points */
	GList *end; /* this is the last element of the path */
	guint n; /* number of elements in the path */
};

/* Add a point to the path and update the window.
 * returns TRUE if an edge is detected. */
gboolean ramble_add(struct ramble *ramble, GdkWindow *window, gint x, gint y);

/* Draw the ramble path to the cairo context */
void ramble_draw(struct ramble *ramble, cairo_t *ctx);

/* Create a ramble structure */
struct ramble *ramble_new();

/* Destroy a ramble structure */
void ramble_free(struct ramble *ramble);

#endif

#endif

