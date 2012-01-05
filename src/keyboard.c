/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2012 François Agrech

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
#include <gconf/gconf-client.h>
#include "system.h"
#include "trace.h"
#include "keyboard.h"
#include "key.h"
#include "settings.h"

/* Checks a colon separated list of strings for the presence of a particular string.
 * This is useful to check the "extensions" gconf parameter which is a list of colon separated strings */
void keyboard_status_update(struct keyboard *keyboard, struct status *status)
{
	START_FUNC
	gboolean active=FALSE;
	gchar *allextstr=NULL;
	gchar **extstrs=NULL;
	gchar **extstr=NULL;
	if (keyboard->id) {
		if ((allextstr=settings_get_string("layout/extensions"))) {
			extstrs=g_strsplit(allextstr, ":", -1);
			extstr=extstrs;
			while (extstr && *extstr) {
				if (!strcmp(keyboard->id, *(extstr++))) { active=TRUE; break; }
			}
			g_strfreev(extstrs);
			g_free(allextstr);
		}
		if ((!active)&&keyboard->activated&&keyboard->onhide) {
			/* onhide trigger */
			if (!keyboard->onhide->key) flo_error(_("No key associated with onhide trigger"));
			else if (KEY_RELEASED!=keyboard->onhide->key->state) {
				status_pressed_set(status, keyboard->onhide->key);
				status_pressed_set(status, NULL);
			}
		}
		keyboard->activated=active;
	}
	END_FUNC
}

/* Return TRUE if the keyboard is active */
gboolean keyboard_activated(struct keyboard *keyboard)
{
	START_FUNC
	END_FUNC
	return (!keyboard->id) || keyboard->activated;
}

/* populate keyboard structure */
void keyboard_populate(struct keyboard *keyboard, struct layout *layout, gchar *id, gchar *name,
        enum layout_placement placement, struct keyboard_globaldata *data)
{
	START_FUNC
	struct layout_size *size=NULL;
	struct key *key=NULL;

	flo_debug(TRACE_DEBUG, _("[new keyboard] name=%s id=%s"), name, id);

	if (name) keyboard->name=g_strdup(name);
	if (id) keyboard->id=g_strdup(id);
	keyboard->placement=placement;
	size=layoutreader_keyboard_new(layout);
	if (!size) flo_fatal(_("Unreachable code in keyboard_new"));
	keyboard->width=size->w;
	keyboard->height=size->h;
	keyboard_status_update(keyboard, data->status);

	/* insert all keyboard keys */
#ifdef ENABLE_XKB
	while ((key=key_new(layout, data->style, data->status->xkeyboard, (void *)keyboard))) {
		/* if locker is locked then update the status */
		if (key_get_modifier(key)&data->status->xkeyboard->xkb_state.locked_mods) {
			status_globalmod_set(data->status, key_get_modifier(key));
			fsm_process(data->status, key, FSM_PRESSED);
		}
#else
	while ((key=key_new(layout, data->style, data->status->xkeyboard, (void *)keyboard))) {
#endif
		keyboard->keys=g_slist_append(keyboard->keys, key);
	}

	layoutreader_keyboard_free(layout, size);
	END_FUNC
}

/* loads the main keyboard from the layout. */
struct keyboard *keyboard_new (struct layout *layout, struct keyboard_globaldata *data)
{
	START_FUNC
	struct keyboard *keyboard=NULL;

	/* allocate memory for keyboard */
	if (!(keyboard=(struct keyboard*)g_malloc(sizeof(struct keyboard))))
		flo_fatal(_("Unable to allocate memory for keyboard"));
	memset(keyboard, 0, sizeof(struct keyboard));

	keyboard_populate(keyboard, layout, NULL, NULL, LAYOUT_VOID, data);

	END_FUNC
	return keyboard;
}

/* loads an extension from the layout */
struct keyboard *keyboard_extension_new (struct layout *layout, struct keyboard_globaldata *data)
{
	START_FUNC
	struct layout_extension *extension=NULL;
	struct layout_trigger *trigger=NULL;
	struct keyboard *keyboard=NULL;

	/* allocate memory for keyboard */
	if (!(keyboard=g_malloc(sizeof(struct keyboard))))
		flo_fatal(_("Unable to allocate memory for keyboard"));
	memset(keyboard, 0, sizeof(struct keyboard));

	if ((extension=layoutreader_extension_new(layout))) {
		keyboard_populate(keyboard, layout, extension->identifiant, extension->name,
			extension->placement, data);
		if ((trigger=layoutreader_trigger_new(layout))) {
			if (!(keyboard->onhide=g_malloc(sizeof(struct keyboard_trigger))))
				flo_fatal(_("Unable to allocate memory for keyboard trigger"));
			keyboard->onhide->key=trigger->object;
			layoutreader_trigger_free(layout, trigger);
		}
		layoutreader_extension_free(layout, extension);
	} else {
		g_free(keyboard);
		keyboard=NULL;
	}

	END_FUNC
	return keyboard;
}

/* delete a key from the keyboard */
void keyboard_key_free (gpointer data, gpointer userdata)
{
	START_FUNC
	struct key *key=(struct key *)data;
	key_free(key);
	END_FUNC
}

/* delete a keyboard */
void keyboard_free (struct keyboard *keyboard)
{
	START_FUNC
	if (keyboard) {
		g_slist_foreach(keyboard->keys, keyboard_key_free, NULL);
		g_slist_free(keyboard->keys);
		if (keyboard->name) g_free(keyboard->name);
		if (keyboard->id) g_free(keyboard->id);
		if (keyboard->onhide) g_free(keyboard->onhide);
		g_free(keyboard);
	}
	END_FUNC
}

/* update the relative position of the keyboard to the view */
void keyboard_set_pos(struct keyboard *keyboard, gdouble x, gdouble y)
{
	START_FUNC
	keyboard->xpos=x; keyboard->ypos=y;
	END_FUNC
}

/* tell the keyboard that it is under another one */
void keyboard_set_under(struct keyboard *keyboard)
{
	START_FUNC
	keyboard->under=TRUE;
	END_FUNC
}

/* tell the keyboard that it is above other keyboards */
void keyboard_set_over(struct keyboard *keyboard)
{
	START_FUNC
	keyboard->under=FALSE;
	END_FUNC
}

/* draw the keyboard to cairo surface */
void keyboard_draw (struct keyboard *keyboard, cairo_t *cairoctx,
	struct style *style, struct status *status, enum style_class class)
{
	START_FUNC
	GSList *list=keyboard->keys;
	if (keyboard->under && (class==STYLE_SYMBOL)) return;
	cairo_save(cairoctx);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	while (list)
	{
		switch(class) {
			case STYLE_SHAPE:
				key_shape_draw((struct key *)list->data, style, cairoctx);
				if (keyboard->under) key_symbol_draw((struct key *)list->data, style, cairoctx, status, FALSE);
				break;
			case STYLE_SYMBOL:
				key_symbol_draw((struct key *)list->data, style, cairoctx, status, FALSE);
				break;
		}
		list = list->next;
	}
	cairo_restore(cairoctx);
	END_FUNC
}

/* draw the keyboard background to cairo surface */
void keyboard_background_draw (struct keyboard *keyboard, cairo_t *cairoctx, struct style *style, struct status *status)
{
	START_FUNC
	keyboard_draw(keyboard, cairoctx, style, status, STYLE_SHAPE);
	END_FUNC
}

/* draw the keyboard symbols  to cairo surface */
void keyboard_symbols_draw (struct keyboard *keyboard, cairo_t *cairoctx, struct style *style, struct status *status)
{
	START_FUNC
	keyboard_draw(keyboard, cairoctx, style, status, STYLE_SYMBOL);
	END_FUNC
}

/* draw the focus indicator on a key */
void keyboard_focus_draw (struct keyboard *keyboard, cairo_t *cairoctx, gdouble w, gdouble h,
	struct style *style, struct key *key, struct status *status)
{
	START_FUNC
	cairo_save(cairoctx);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	key_focus_draw(key, style, cairoctx, w, h, status);
	cairo_restore(cairoctx);
	END_FUNC
}

/* draw the pressed indicator on a key */
void keyboard_press_draw (struct keyboard *keyboard, cairo_t *cairoctx,
	struct style *style, struct key *key, struct status *status)
{
	START_FUNC
	if (keyboard_activated(keyboard)) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
		if (key!=status_focus_get(status))
			key_press_draw(key, style, cairoctx, status);
		cairo_restore(cairoctx);
	}
	END_FUNC
}

/* getters */
gdouble keyboard_get_width(struct keyboard *keyboard) { return keyboard->width; }
gdouble keyboard_get_height(struct keyboard *keyboard) { return keyboard->height; }
enum layout_placement keyboard_get_placement(struct keyboard *keyboard) { return keyboard->placement; }

/* returns a rectangle containing the key */
/* WARNING: not thread safe */
GdkRectangle *keyboard_key_getrect(struct keyboard *keyboard, struct key *key, gboolean focus_zoom)
{
	START_FUNC
	static GdkRectangle rect;
	gdouble x, y, w, h, xmargin, ymargin;
	x=keyboard->xpos+(key->x-(key->w/2.0));
	y=keyboard->ypos+(key->y-(key->h/2.0));
	w=key->w;
	h=key->h;
	if (focus_zoom) {
		xmargin=(w*settings_double_get("window/scalex")*(settings_double_get("style/focus_zoom")-1.0))+5.0;
		ymargin=(h*settings_double_get("window/scaley")*(settings_double_get("style/focus_zoom")-1.0))+5.0;
	} else {
		xmargin=5.0;
		ymargin=5.0;
	}
	rect.x=(x*settings_double_get("window/scalex"))-xmargin;
	rect.y=(y*settings_double_get("window/scaley"))-ymargin;
	rect.width=(w*settings_double_get("window/scalex"))+(xmargin*2);
	rect.height=(h*settings_double_get("window/scaley"))+(ymargin*2);
	END_FUNC
	return &rect;
}

/* Get the key at position (x,y) */
#ifdef ENABLE_RAMBLE
struct key *keyboard_hit_get(struct keyboard *keyboard, gint x, gint y, gdouble zx, gdouble zy, enum key_hit *hit)
#else
struct key *keyboard_hit_get(struct keyboard *keyboard, gint x, gint y, gdouble zx, gdouble zy)
#endif
{
	START_FUNC
	GSList *list=keyboard->keys;
#ifdef ENABLE_RAMBLE
	enum key_hit kh;
#endif
	if (keyboard->under) return NULL;
	while (list &&
#ifdef ENABLE_RAMBLE
		(!(kh=key_hit((struct key *)list->data, x, y, zx, zy))))
#else
		(!key_hit((struct key *)list->data, x, y, zx, zy)))
#endif
		list=list->next;
#ifdef ENABLE_RAMBLE
	if (hit) *hit=kh;
#endif
	END_FUNC
	return list?(struct key *)list->data:NULL;
}

