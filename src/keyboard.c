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
	if (mod) {
		if (mod&rec.locked_mods) {
			key_set_pressed(key, TRUE);
		}
	}
}

/* GConf callback called when a color has changed: update the keyboard color accordingly */
/*void keyboard_change_color (struct keyboard *keyboard, GConfEntry *entry, enum style_colours colclass)
{
	guint i;
	gchar *color=(gchar *)gconf_value_get_string(gconf_entry_get_value(entry));
	key_update_color(colclass, color);
	for(i=0;i<256;i++) {
		if (keyboard->keys[i]) {
			switch(colclass) {
				case STYLE_KEY_COLOR: 
					if (!keyboard->keys[i]->pressed) key_set_color(keyboard->keys[i], colclass);
					break;
				case STYLE_TEXT_COLOR:
					key_update_text_color(keyboard->keys[i]);
					break;
				case STYLE_ACTIVATED_COLOR:
					if (keyboard->keys[i]->pressed) key_set_color(keyboard->keys[i], colclass);
				case STYLE_MOUSE_OVER_COLOR:
					*//* unlikely and if that ever happen, just move to another key *//*
					break;
				default:
					flo_error(_("Should never happen: unknown color class updated"));
					break;
			}
		}
	}
}*/

/* Callback for GConf color change */
/*
void keyboard_change_key_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, STYLE_KEY_COLOR);
}*/

/* Callback for GConf color change */
/*
void keyboard_change_text_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, STYLE_TEXT_COLOR);
}*/

/* Callback for GConf color change */
/*
void keyboard_change_activated_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, STYLE_ACTIVATED_COLOR);
}*/

/* Callback for GConf color change */
/*
void keyboard_change_mouseover_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, STYLE_MOUSE_OVER_COLOR);
}*/

/* Callback for GConf autoclick timer change */
/*
void keyboard_change_auto_click(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	gdouble click_time=gconf_value_get_float(gconf_entry_get_value(entry));
	if (click_time>=0.01)
		keyboard->timer_step=FLO_ANIMATION_PERIOD/click_time;
	else
		keyboard->timer_step=0.0;
}*/

/* Register GConf callbacks */
/*
void keyboard_settings_connect(struct keyboard *keyboard)
{
	settings_changecb_register("colours/key", keyboard_change_key_color, (gpointer)keyboard);
	settings_changecb_register("colours/label", keyboard_change_text_color, (gpointer)keyboard);
	settings_changecb_register("colours/activated", keyboard_change_activated_color, (gpointer)keyboard);
	settings_changecb_register("colours/mouseover", keyboard_change_mouseover_color, (gpointer)keyboard);
	settings_changecb_register("behaviour/auto_click", keyboard_change_auto_click, (gpointer)keyboard);
}*/

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

	/*click_time=settings_get_double("behaviour/auto_click");
	if (click_time<=0.0) keyboard->timer_step=0.0; else keyboard->timer_step=FLO_ANIMATION_PERIOD/click_time;*/

	if (name) {
		keyboard->name=g_malloc(sizeof(gchar)*(strlen(name)+1));
		strcpy(keyboard->name, name);
	}
	keyboard->placement=placement;
	layoutreader_readkeyboard(reader, keyboard_insertkey, keyboard_setsize, keyboard, global, level);


/*	keyboard_settings_connect(keyboard);*/
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
void keyboard_draw (struct keyboard *keyboard, cairo_t *cairoctx, gdouble z,
	struct style *style, GdkModifierType mod)
{
	GSList *list=keyboard->keys;
	cairo_save(cairoctx);
	cairo_scale(cairoctx, z, z);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	while (list)
	{
		key_draw((struct key *)list->data, style, cairoctx, z, mod, 0.0, FALSE);
		list = list->next;
	}
	cairo_restore(cairoctx);
}

/* redraw a single key of the keyboard (keyboard_draw must have been called first 
 * if activated, the key is drawn with the activated color. */
void keyboard_key_draw (struct keyboard *keyboard, cairo_t *cairoctx, gdouble z, struct style *style,
	struct key *key, GdkModifierType mod, gboolean activated, gdouble timer)
{
	cairo_save(cairoctx);
	cairo_scale(cairoctx, z, z);
	cairo_translate(cairoctx, keyboard->xpos, keyboard->ypos);
	key_draw(key, style, cairoctx, z, mod, timer, activated);
	cairo_restore(cairoctx);
}

/* returns a rectangle containing the key */
void keyboard_key_getrect(struct keyboard *keyboard, struct key *key,
        gdouble *x, gdouble *y, gdouble *w, gdouble *h)
{
	*x=keyboard->xpos+(key->x-(key->w/2.0));
	*y=keyboard->ypos+(key->y-(key->h/2.0));
	*w=key->w;
	*h=key->h;
}

/* getters */
gdouble keyboard_get_width(struct keyboard *keyboard) { return keyboard->width; }
gdouble keyboard_get_height(struct keyboard *keyboard) { return keyboard->height; }
enum layout_placement keyboard_get_placement(struct keyboard *keyboard) { return keyboard->placement; }

