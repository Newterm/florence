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

#include <stdio.h>
#include <X11/XKBlib.h>
#include <gtk/gtk.h>
#include <cspi/spi.h>
#include <gconf/gconf-client.h>
#include "system.h"
#include "trace.h"
#include "keyboard.h"
#include "key.h"
#include "settings.h"

/* Add a key to the keyboard: Called by layoutparser when a key has been parsed in the layout 
 * userdata1 is the keyboard
 * userdata2 is the global key table */
void keyboard_insertkey (void *userdata1, char *shape,
	unsigned char code, double x, double y, double w, double h, void *userdata2)
{
	struct keyboard *keyboard=(struct keyboard *)userdata1;
	struct keyboard_globaldata *global=(struct keyboard_globaldata *)userdata2;
	struct key **key_table=global->key_table;
	struct style *style=global->style;
	XkbDescPtr xkb=global->xkb_desc;
	XkbStateRec rec=global->xkb_state;
	struct key *key;
	GdkModifierType mod;
	gboolean locker;

	flo_debug(_("[new key] code=%d x=%f y=%f w=%f h=%f shape=%s"), code, x, y, w, h, shape);

	locker=XkbKeyAction(xkb, code, 0)?XkbKeyAction(xkb, code, 0)->type==XkbSA_LockMods:FALSE;
	mod=xkb->map->modmap[code];

	key=key_new((void *)keyboard, code, mod, locker, x, y, w, h, style_shape_get(style, shape));
	keyboard->keys=g_slist_append(keyboard->keys, key);
	if (key_table) key_table[code]=key;

	/* Press the activated locker key if the hardware key is activated */
	if (mod && (mod&rec.locked_mods)) {
		key_set_pressed(key, TRUE);
		status_press(global->status, key);
	}
}

/* Set the logical size of the keyboard
 * Called while parsing xml layout file */
void keyboard_setsize(void *userdata, double width, double height)
{
	struct keyboard *keyboard=(struct keyboard *)userdata;

	keyboard->width=width;
	keyboard->height=height;
}

/* Checks a colon separated list of strings for the presence of a particular string.
 * This is useful to check the "extensions" gconf parameter which is a list of colon separated strings */
gboolean keyboard_activated(struct keyboard *keyboard)
{
	gboolean ret=FALSE;
	gchar *allextstr=NULL;
	gchar **extstrs=NULL;
	gchar **extstr=NULL;
	if (!keyboard->name) return TRUE;
	/* TODO: cache this and register callback for change */
	if ((allextstr=settings_get_string("layout/extensions"))) {
		extstrs=g_strsplit(allextstr, ":", -1);
		extstr=extstrs;
		while (extstr && *extstr) {
			if (!strcmp(keyboard->name, *(extstr++))) { ret=TRUE; break; }
		}
		g_strfreev(extstrs);
		g_free(allextstr);
	}
	return ret;
}

/* Create a keyboard: the layout is passed as a text reader */
struct keyboard *keyboard_new (xmlTextReaderPtr reader, int level, gchar *name, enum layout_placement placement, void *data)
{
	struct keyboard_globaldata *global=(struct keyboard_globaldata *)data;
	struct keyboard *keyboard=NULL;

	flo_debug(_("[new keyboard] name=%s"), name);

	/* allocate memory for keyboard */
	if (!(keyboard=g_malloc(sizeof(struct keyboard)))) flo_fatal(_("Unable to allocate memory for keyboard"));
	memset(keyboard, 0, sizeof(struct keyboard));

	if (name) {
		keyboard->name=g_strdup(name);
	}
	keyboard->placement=placement;
	layoutreader_readkeyboard(reader, keyboard_insertkey, keyboard_setsize, keyboard, global, level);

	return keyboard;
}

/* delete a keyboard */
void keyboard_free (struct keyboard *keyboard)
{
	if (keyboard->name) g_free(keyboard->name);
	g_free(keyboard);
}

/* fill the hitmap with key data */
void keyboard_hitmap_draw(struct keyboard *keyboard, guchar *hitmap, guint w, guint h, gdouble x, gdouble y, gdouble z)
{
	GSList *list=keyboard->keys;
	keyboard->xpos=x; keyboard->ypos=y;
	while (list)
	{
		key_hitmap_draw((struct key *)list->data, hitmap, w, h,
			keyboard->xpos, keyboard->ypos, z);
		list = list->next;
	}
}

/* draw the keyboard to cairo surface */
void keyboard_draw (struct keyboard *keyboard, cairo_t *cairoctx,
	struct style *style, GdkModifierType mod, enum style_class class)
{
	GSList *list=keyboard->keys;
	cairo_save(cairoctx);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	while (list)
	{
		switch(class) {
			case STYLE_SHAPE:
				key_shape_draw((struct key *)list->data, style, cairoctx);
				break;
			case STYLE_SYMBOL:
				key_symbol_draw((struct key *)list->data, style, cairoctx, mod);
				break;
		}
		list = list->next;
	}
	cairo_restore(cairoctx);
}

/* draw the keyboard background to cairo surface */
void keyboard_background_draw (struct keyboard *keyboard, cairo_t *cairoctx, struct style *style)
{
	keyboard_draw(keyboard, cairoctx, style, 0, STYLE_SHAPE);
}

/* draw the keyboard symbols  to cairo surface */
void keyboard_symbols_draw (struct keyboard *keyboard, cairo_t *cairoctx, struct style *style, GdkModifierType mod)
{
	keyboard_draw(keyboard, cairoctx, style, mod, STYLE_SYMBOL);
}

/* draw the focus indicator on a key */
void keyboard_focus_draw (struct keyboard *keyboard, cairo_t *cairoctx, gdouble z,
	struct style *style, struct key *key, gdouble timer)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	key_focus_draw(key, style, cairoctx, z, timer);
	cairo_restore(cairoctx);
}

/* draw the pressed indicator on a key */
void keyboard_press_draw (struct keyboard *keyboard, cairo_t *cairoctx, gdouble z,
	struct style *style, struct key *key)
{
	if (keyboard_activated(keyboard)) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
		key_press_draw(key, style, cairoctx, z);
		cairo_restore(cairoctx);
	}
}

/* getters */
gdouble keyboard_get_width(struct keyboard *keyboard) { return keyboard->width; }
gdouble keyboard_get_height(struct keyboard *keyboard) { return keyboard->height; }
enum layout_placement keyboard_get_placement(struct keyboard *keyboard) { return keyboard->placement; }

/* returns a rectangle containing the key */
void keyboard_key_getrect(struct keyboard *keyboard, struct key *key,
	gdouble *x, gdouble *y, gdouble *w, gdouble *h)
{
	*x=keyboard->xpos+(key->x-(key->w/2.0));
	*y=keyboard->ypos+(key->y-(key->h/2.0));
	*w=key->w;
	*h=key->h;
}

