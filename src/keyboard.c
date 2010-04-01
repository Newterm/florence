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

#include <stdio.h>
#include <X11/XKBlib.h>
#include <gtk/gtk.h>
#ifdef ENABLE_AT_SPI
#include <cspi/spi.h>
#endif
#include <gconf/gconf-client.h>
#include "system.h"
#include "trace.h"
#include "keyboard.h"
#include "key.h"
#include "settings.h"

/* Checks a colon separated list of strings for the presence of a particular string.
 * This is useful to check the "extensions" gconf parameter which is a list of colon separated strings */
gboolean keyboard_activated(struct keyboard *keyboard)
{
	gboolean ret=FALSE;
	gchar *allextstr=NULL;
	gchar **extstrs=NULL;
	gchar **extstr=NULL;
	if (!keyboard->id) return TRUE;
	/* TODO: cache this and register callback for change */
	/* check race condition with view.c */
	if ((allextstr=settings_get_string("layout/extensions"))) {
		extstrs=g_strsplit(allextstr, ":", -1);
		extstr=extstrs;
		while (extstr && *extstr) {
			if (!strcmp(keyboard->id, *(extstr++))) { ret=TRUE; break; }
		}
		g_strfreev(extstrs);
		g_free(allextstr);
	}
	return ret;
}

/* Create a keyboard: the layout is passed as a text reader */
struct keyboard *keyboard_new (struct layout *layout, struct style *style, gchar *id, gchar *name,
	enum layout_placement placement, struct keyboard_globaldata *data)
{
	struct keyboard *keyboard=NULL;
	struct layout_size *size=NULL;
	struct key *key=NULL;

	flo_debug(_("[new keyboard] name=%s id=%s"), name, id);

	/* allocate memory for keyboard */
	if (!(keyboard=g_malloc(sizeof(struct keyboard))))
		flo_fatal(_("Unable to allocate memory for keyboard"));
	memset(keyboard, 0, sizeof(struct keyboard));

	if (name) keyboard->name=g_strdup(name);
	if (id) keyboard->id=g_strdup(id);
	keyboard->placement=placement;
	size=layoutreader_keyboard_new(layout);
	if (!size) flo_fatal(_("Unreachable code in keyboard_new"));
	keyboard->width=size->w;
	keyboard->height=size->h;

	/* insert all keyboard keys */
#ifdef ENABLE_XKB
	while ((key=key_new(layout, style, data->status->xkeyboard, (void *)keyboard))) {
		/* if locker is locked then update the status */
		if (key_get_modifier(key)&data->status->xkeyboard->xkb_state.locked_mods) {
			status_globalmod_set(data->status, key_get_modifier(key));
			status_fsm_process(data->status, key, STATUS_PRESSED);
		}
#else
	while ((key=key_new(layout, style, data->status->xkeyboard, (void *)keyboard))) {
#endif
		keyboard->keys=g_slist_append(keyboard->keys, key);
	}

	layoutreader_keyboard_free(layout, size);
	return keyboard;
}

/* delete a key from the keyboard */
void keyboard_key_free (gpointer data, gpointer userdata)
{
	struct key *key=(struct key *)data;
	key_free(key);
}

/* delete a keyboard */
void keyboard_free (struct keyboard *keyboard)
{
	if (keyboard) {
		g_slist_foreach(keyboard->keys, keyboard_key_free, NULL);
		g_slist_free(keyboard->keys);
		if (keyboard->name) g_free(keyboard->name);
		if (keyboard->id) g_free(keyboard->id);
		g_free(keyboard);
	}
}

/* update the relative position of the keyboard to the view */
void keyboard_set_pos(struct keyboard *keyboard, gdouble x, gdouble y)
{
	keyboard->xpos=x; keyboard->ypos=y;
}

/* draw the keyboard to cairo surface */
void keyboard_draw (struct keyboard *keyboard, cairo_t *cairoctx,
	struct style *style, struct status *status, enum style_class class)
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
				key_symbol_draw((struct key *)list->data, style, cairoctx, status, FALSE);
				break;
		}
		list = list->next;
	}
	cairo_restore(cairoctx);
}

/* draw the keyboard background to cairo surface */
void keyboard_background_draw (struct keyboard *keyboard, cairo_t *cairoctx, struct style *style)
{
	keyboard_draw(keyboard, cairoctx, style, NULL, STYLE_SHAPE);
}

/* draw the keyboard symbols  to cairo surface */
void keyboard_symbols_draw (struct keyboard *keyboard, cairo_t *cairoctx, struct style *style, struct status *status)
{
	keyboard_draw(keyboard, cairoctx, style, status, STYLE_SYMBOL);
}

/* clear the focus key from surface */
void keyboard_shape_clear (struct keyboard *keyboard, cairo_surface_t *surface,
	struct style *style, struct key *key, gdouble zoom)
{
	cairo_t *cairoctx;
	cairoctx=cairo_create(surface);
	cairo_scale(cairoctx, zoom, zoom);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	cairo_set_operator(cairoctx, CAIRO_OPERATOR_CLEAR);
	key_shape_draw(key, style, cairoctx);
	cairo_destroy(cairoctx);
}

/* add the focus key to surface */
void keyboard_shape_draw (struct keyboard *keyboard, cairo_surface_t *surface,
	struct style *style, struct key *key, gdouble zoom)
{
	cairo_t *cairoctx;
	cairoctx=cairo_create(surface);
	cairo_scale(cairoctx, zoom, zoom);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	cairo_set_operator(cairoctx, CAIRO_OPERATOR_OVER);
	key_shape_draw(key, style, cairoctx);
	cairo_destroy(cairoctx);
}

/* draw the focus indicator on a key */
void keyboard_focus_draw (struct keyboard *keyboard, cairo_t *cairoctx, gdouble w, gdouble h,
	struct style *style, struct key *key, struct status *status)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	key_focus_draw(key, style, cairoctx, w, h, status);
	cairo_restore(cairoctx);
}

/* draw the pressed indicator on a key */
void keyboard_press_draw (struct keyboard *keyboard, cairo_t *cairoctx,
	struct style *style, struct key *key, struct status *status)
{
	if (keyboard_activated(keyboard)) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
		if (key!=status_focus_get(status))
			key_press_draw(key, style, cairoctx, status);
		cairo_restore(cairoctx);
	}
}

/* getters */
gdouble keyboard_get_width(struct keyboard *keyboard) { return keyboard->width; }
gdouble keyboard_get_height(struct keyboard *keyboard) { return keyboard->height; }
enum layout_placement keyboard_get_placement(struct keyboard *keyboard) { return keyboard->placement; }

/* returns a rectangle containing the key */
/* WARNING: not thread safe */
GdkRectangle *keyboard_key_getrect(struct keyboard *keyboard, struct key *key,
	gdouble zoom, gboolean focus_zoom)
{
	static GdkRectangle rect;
	gdouble x, y, w, h, xmargin, ymargin;
	x=keyboard->xpos+(key->x-(key->w/2.0));
	y=keyboard->ypos+(key->y-(key->h/2.0));
	w=key->w;
	h=key->h;
	if (focus_zoom) {
		xmargin=(w*zoom*(settings_double_get("style/focus_zoom")-1.0))+5.0;
		ymargin=(h*zoom*(settings_double_get("style/focus_zoom")-1.0))+5.0;
	} else {
		xmargin=5.0;
		ymargin=5.0;
	}
	rect.x=(x*zoom)-xmargin; rect.y=(y*zoom)-ymargin;
	rect.width=(w*zoom)+(xmargin*2); rect.height=(h*zoom)+(ymargin*2);
	return &rect;
}

/* Get the key at position (x,y) */
struct key *keyboard_hit_get(struct keyboard *keyboard, gint x, gint y, gdouble z)
{
	GSList *list=keyboard->keys;
	while (list &&
		(!key_hit((struct key *)list->data, x, y, z)))
		list=list->next;
	return list?(struct key *)list->data:NULL;
}

