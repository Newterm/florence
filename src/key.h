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

#ifndef FLO_KEY
#define FLO_KEY

#include "config.h"
#ifdef ENABLE_XKB
#include <X11/XKBlib.h>
#endif
#include <gdk/gdk.h>
#include <glib.h>
#include "status.h"
#include "style.h"
#include "layoutreader.h"
#include "xkeyboard.h"

struct status;

/* the key state, used in the FSM table */
enum key_state {
	KEY_PRESSED,
	KEY_RELEASED,
	KEY_LOCKED, /* lockers and modifiers */
	KEY_LATCHED, /* modifiers */
	KEY_STATE_NUM
};

/* action keys have actions attached */
struct key_action {
	GdkModifierType modifier; /* modifier mask */
	enum layout_key_type type; /* action key type */
};

/* A key is an item of the keyboard. It represents a real keyboard key.
 * A key is replesented on the screen with a background (shape) and a foreground (symbol)
 * the background is constans, whereas the foreground change according to the global modifiers
 * when the auto-click timer is active, it is drawn between the background and the foreground */
struct key {
	guint code; /* hardware key code */
	struct key_action **actions; /* NULL terminated list of actions attached to the key */
	struct shape *shape; /* graphical representation of the background of the key */
	gdouble x, y; /* position of the key inside the keyboard */
	gdouble w, h; /* size of the key inside the keyboard */
	GdkModifierType modifier; /* Modifier mask. 0 When the key is not a modifier key. */
	gboolean locker; /* TRUE if the key is either the caps lock or num lock key. */
	enum key_state state; /* state of the key (pressed, released, latched or locked) */
	void *userdata; /* custom data attached to the key (used to attach to a keyboard) */
};

/* Instanciate a key
 * the key may have a static label which will be always drawn in place of the symbol */
struct key *key_new(struct layout *layout, struct style *style, struct xkeyboard *xkeyboard, void *userdata);
/* deallocate memory used by the key */
void key_free(struct key *key);

/* Send SPI events coresponding to the key */
void key_press(struct key *key, struct status *status);
void key_release(struct key *key, struct status *status);

/* Draw the shape of the key to the cairo surface. */
void key_shape_draw(struct key *key, struct style *style, cairo_t *cairoctx);
/* Draw the symbol of the key to the cairo surface. The symbol drawn on the key depends on the modifier */
void key_symbol_draw(struct key *key, struct style *style,
	cairo_t *cairoctx, struct status *status, gboolean use_matrix);
/* Draw the focus notifier to the cairo surface. */
void key_focus_draw(struct key *key, struct style *style, cairo_t *cairoctx,
	gdouble width, gdouble height, struct status *status);
/* Draw the key press notifier to the cairo surface. */
void key_press_draw(struct key *key, struct style *style, cairo_t *cairoctx, struct status *status);

/* setters and getters */
void key_state_set(struct key *key, enum key_state state);
gboolean key_is_locker(struct key *key);
void *key_get_userdata(struct key *key);
GdkModifierType key_get_modifier(struct key *key);

/* return if key is it at position */
gboolean key_hit(struct key *key, gint x, gint y, gdouble z);

#endif

