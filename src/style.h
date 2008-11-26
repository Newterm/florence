/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008 François Agrech

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

#ifndef FLO_STYLE
#define FLO_STYLE

#include <glib.h>
#include <libxml/xmlreader.h>
#include <cairo.h>
#include <librsvg/rsvg.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* a shape is the background of a key */
struct shape {
	gchar *name;
	RsvgHandle *svg;
	guchar *source;
	cairo_surface_t *mask; /* mask of the shape (alpha channel) */
	guint maskw, maskh; /* size of the mask */
};

/* There are 6 classes of color for the style */
/* TODO: We will replace that by css */
enum style_colours {
	STYLE_KEY_COLOR, /* color of the background of the key */
	STYLE_OUTLINE_COLOR, /* color of the outline of the key */
	STYLE_ACTIVATED_COLOR, 
	STYLE_TEXT_COLOR,
	STYLE_MOUSE_OVER_COLOR,
	STYLE_NUM_COLOURS
};

/* class of style */
enum style_class {
	STYLE_SHAPE,
	STYLE_SYMBOL
};

/* A style is a list of symbols and shapes.
 * A shape is the background of a key, and the symbol is the foreground of the key */
struct style {
	gchar *base_uri;
	GSList *symbols;
	GSList *shapes;
	struct shape *default_shape;
};

struct style *style_new(xmlTextReaderPtr reader, gchar *base_uri);
void style_free(struct style *style);
/* draw a style preview to 32x32 gdk pixbuf 
 * this function is called by the settings dialog */
GdkPixbuf *style_pixbuf_draw(struct style *style);

/* set cairo color to one of the style colors */
void style_cairo_set_color(struct style *style, cairo_t *cairoctx, enum style_colours c);
/* update the colours */
void style_update_colors(struct style *style);

struct shape *style_shape_get(struct style *style, gchar *name);
void style_shape_draw(struct shape *shape, cairo_t *cairoctx, gdouble w, gdouble h);
/* create a mask surface for the shape, if it doesn't already exist */
cairo_surface_t *style_shape_get_mask(struct shape *shape, guint w, guint h);
void style_symbol_draw(struct style *style, cairo_t *cairoctx, guint keyval, gdouble w, gdouble h);

#endif

