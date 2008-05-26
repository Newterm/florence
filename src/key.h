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
#include <gdk/gdk.h>
#include "style.h"

/* The key_symbol is the graphical representation of a keyval. */
struct key_symbol {
	guint keyval; /* gdk key value for that symbol */
	GnomeCanvasItem **sym; /* list of canvas item representing the symbol */
};

/* A key is an item of the keyboard. It represents a real keyboard key.
 * A modified key has either a label, or a symbol, or nothing.
 * If it has nothing, it is represented by it's keyval's unicode value (from gdk) on screen (for ex 'A')
 * Or else, it is represented by its label (ex ctrl), or it's symbol.
 * The key can be modified by modifiers and its on-screen representation will change.
 * For performance issue, all symbols the key has already drawn are recorded in a list for they are not
 * redrawn, but shown and hidden when the key is modified. */
struct key {
	struct keyboard *keyboard; /* keyboard the key belongs to */
	gchar *label; /* text written on the key, or NULL. */
        guint code; /* gdk key code of the key */
	gdouble width; /* width in world coordinate on the canvas */
	gdouble height; /* height in world coordinate on the canvas */
        GnomeCanvasClipgroup *group; /* Canvas group the key belongs to */
        GnomeCanvasItem **symbol; /* Symbol currently drawn on screen, or NULL */
	GSList *drawn_syms; /* List of symbols already drawn */
        GnomeCanvasItem *shape; /* Canvas item representing the shape of the key. */
        GnomeCanvasItem *timer; /* Canvas item representing the auto-click timer on the key */
	gboolean pressed; /* TRUE when the key is activated */
	GdkModifierType modifier; /* Modifier mask. 0 When the key is not a modifier key. */
	gboolean locker; /* TRUE if the key is either the caps lock or num lock key. */
};

/* Must be called before any other key function is called */
void key_init(xmlTextReaderPtr layout);
/* Must be called When the key module is no longer needed */
void key_exit();
/* Create a key tied to a canvas group and to a keeyboard */
struct key *key_new(struct keyboard *keyboard, guint code, GnomeCanvasClipgroup *group, GdkModifierType mod,
	gboolean locker, gchar *shape);
/* deallocate memory used by the key */
void key_free(struct key *key);

/* Change the size of the key on screen */
void key_resize(struct key *key, gdouble zoom);
/* Change the color of a style item for all keys */
void key_update_color(enum style_colours colclass, gchar *color);
/* Change the color of the symbol or the text that is drawn on the key */
void key_update_text_color(struct key *key);
/* Change the color of the background of the key */
void key_set_color(struct key *key, enum style_colours color);
/* Draw the key inside its canvas group. The symbol drawn on the key depends on the modifier */
void key_draw(struct key *key, double w, double h, GdkModifierType mod);

/* Change the symbol drawn on the key according to the modifiers passed as argument */
void key_switch_mode(struct key *key, GdkModifierType mod);
/* Update the representation of the auto-click timer on the key, according to the value 
 * The value must be >=0 and <=1 ; If 0, the timer representation is deleted. If 1, The timer
 * is represented as full. */
void key_update_timer(struct key *key, double value);

/* Returns the keyboard this key is associated to. */
struct keyboard* key_get_keyboard(struct key *key);

#endif

