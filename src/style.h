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
#include <libgnomecanvas/libgnomecanvas.h>
#include <libxml/xmlreader.h>

struct style {
        GSList *symbols;
        GSList *shapes;
};

enum style_colours {
        STYLE_KEY_COLOR,
        STYLE_OUTLINE_COLOR,
        STYLE_ACTIVATED_COLOR,
        STYLE_TEXT_COLOR,
        STYLE_MOUSE_OVER_COLOR,
        STYLE_NUM_COLOURS
};

struct style *style_new(xmlTextReaderPtr reader);
void style_free(struct style *style);
gchar *style_get_color(enum style_colours c);
void style_set_color(enum style_colours c, gchar *color);
GnomeCanvasItem *style_shape_draw(struct style *style, GnomeCanvasGroup *group, gchar *name);
GnomeCanvasItem **style_symbol_draw(struct style *style, GnomeCanvasGroup *group, guint keyval);

#endif

