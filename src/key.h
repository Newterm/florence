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
#include "style.h"
#include "layoutreader.h"
#include "xkeyboard.h"

struct status;

/* Keys can be of the following types */
enum key_type {
	KEY_CODE,
	KEY_ACTION
};

/* Action keys can be of the following types */
enum key_action_type {
	KEY_CLOSE, /* Close keyboard window */
	KEY_REDUCE, /* Hide keyboard window */
	KEY_CONFIG, /* Sow settings dialog */
	KEY_MOVE, /* Move keyboard window */
	KEY_BIGGER, /* Increade keyboard window size */
	KEY_SMALLER, /* Decrease keyboard window size */
	KEY_SWITCH,/* Switch layout group */
	KEY_EXTEND, /* argument = extension name */
	KEY_UNEXTEND, /* argument = extension name */
	KEY_UNKNOWN, /* unknown action */
	KEY_NOP /* no action */
};

/* the key state, used in the FSM table */
enum key_state {
	KEY_PRESSED,
	KEY_RELEASED,
	KEY_LOCKED, /* lockers and modifiers */
	KEY_LATCHED, /* modifiers */
	KEY_STATE_NUM
};

#ifdef ENABLE_RAMBLE
/* Informations about hit status of the key. */
enum key_hit {
	KEY_MISS, /* key not hit */
	KEY_HIT, /* key hit */
	KEY_BORDER /* key hit on the border */
};
#endif

/* Modified keys have actions attached */
struct key_mod {
	GdkModifierType modifier; /* modifier mask */
	enum key_type type; /* action key type */
	void *data; /* pointer to key data structure, depending on key type */
};

/* Code keys: send the key code event */
struct key_code {
	guint code; /* hardware key code */
	GdkModifierType modifier; /* Modifier mask. 0 When the key is not a modifier key. */
	gboolean locker; /* TRUE if the key is either the caps lock or num lock key. */
};

/* Action keys: act depending on action type */
struct key_action {
	enum key_action_type type; /* action type */
	gchar *argument; /* argument for the action */
};

/* A key is an item of the keyboard. It represents a real keyboard key.
 * A key is replesented on the screen with a background (shape) and a foreground (symbol)
 * the background is constant, whereas the foreground change according to the global modifiers
 * when the auto-click timer is active, it is drawn between the background and the foreground */
struct key {
	GSList *mods; /* list of modifications attached to the key (struct key_mod type) */
	struct shape *shape; /* graphical representation of the background of the key */
	gdouble x, y; /* position of the key inside the keyboard */
	gdouble w, h; /* size of the key inside the keyboard */
	void *keyboard; /* keyboard attached to the key */
	enum key_state state; /* state of the key (pressed, released, latched or locked) */
};

/* Instanciate a key
 * the key may have a static label which will be always drawn in place of the symbol */
struct key *key_new(struct layout *layout, struct style *style, struct xkeyboard *xkeyboard, void *keyboard);
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
void *key_get_keyboard(struct key *key);
GdkModifierType key_get_modifier(struct key *key);

/* return if key is it at position */
#ifdef ENABLE_RAMBLE
enum key_hit key_hit(struct key *key, gint x, gint y, gdouble zx, gdouble zy);
#else
gboolean key_hit(struct key *key, gint x, gint y, gdouble zx, gdouble zy);
#endif
/* Parse string into key type enumeration */
enum key_action_type key_action_type_get(gchar *str);
/* return the action type for the key and the status globalmod */
enum key_action_type key_get_action(struct key *key, struct status *status);

#endif

