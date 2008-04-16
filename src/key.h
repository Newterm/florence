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

#ifndef FLO_KEY
#define FLO_KEY

#include <libgnomecanvas/libgnomecanvas.h>
#include <libxml/xmlreader.h>
#include "style.h"

struct key_mod_sym {
	GdkModifierType mod;
	GnomeCanvasItem **sym;
};

struct key {
	struct keyboard *keyboard;
	gchar *label;
        guint code;
	gdouble width;
	gdouble height;
        GnomeCanvasClipgroup *group;
        GnomeCanvasItem **symbol;
	GSList *drawn_syms;
        GnomeCanvasItem *shape;
        GnomeCanvasItem *timer;
	gboolean pressed;
	GdkModifierType modifier;
};

void key_init(xmlTextReaderPtr layout);
void key_exit();
struct key *key_new(struct keyboard *keyboard, guint code, GnomeCanvasClipgroup *group, GdkModifierType mod, gchar *shape);
void key_free(struct key *key);

void key_resize(struct key *key, gdouble zoom);
void key_update_color(enum style_colours colclass, gchar *color);
void key_update_text_color(struct key *key);
void key_set_color(struct key *key, enum style_colours color);
void key_draw(struct key *key, double w, double h);

void key_switch_mode(struct key *key, GdkModifierType mod);
void key_update_timer(struct key *key, double value);

struct keyboard* key_get_keyboard(struct key *key);

#endif

