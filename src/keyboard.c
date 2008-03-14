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
#include <glib.h>
#include <gtk/gtk.h>
#include <cspi/spi.h>
#include <gconf/gconf-client.h>
#include "system.h"
#include "trace.h"
#include "keyboard.h"
#include "key.h"
#include "settings.h"

#define FLO_ANIMATION_PERIOD 20

GnomeCanvas *keyboard_get_canvas(struct keyboard *keyboard)
{
	return keyboard->canvas;
}

guint keyboard_get_width(struct keyboard *keyboard)
{
	return keyboard->width;
}

guint keyboard_get_height(struct keyboard *keyboard)
{
	return keyboard->height;
}

guchar *keyboard_get_map(struct keyboard *keyboard)
{
	return keyboard->map;
}

gdouble keyboard_get_zoom(struct keyboard *keyboard)
{
	return keyboard->zoom;
}

void keyboard_resize(struct keyboard *keyboard, gdouble zoom)
{
	gint i;
	keyboard->zoom=zoom;
	gnome_canvas_set_pixels_per_unit(keyboard->canvas, zoom);
	gnome_canvas_w2c(keyboard->canvas, keyboard->dwidth, keyboard->dheight , &(keyboard->width), &(keyboard->height));
	if (keyboard->map) g_free(keyboard->map);
	keyboard->map=g_malloc(sizeof(guchar)*keyboard->width*keyboard->height);
	if (!keyboard->map) flo_fatal(_("Out of memory"));
	memset(keyboard->map, 0, sizeof(guchar)*keyboard->width*keyboard->height);
        for(i=0;i<256;i++) {
                if (keyboard->keys[i]) {
			key_resize(keyboard->keys[i], zoom);
		}
	}
}

void keyboard_switch_mode(struct keyboard *keyboard, GdkModifierType mod, gboolean pressed)
{
	int i;

	/* assumption : only the shift and control keys are double */
	if (mod & GDK_SHIFT_MASK) { 
		if (pressed) (keyboard->shift)++; else (keyboard->shift)--;
		if (keyboard->shift) keyboard->modstatus|=mod; else keyboard->modstatus&=~mod;
	}
	else if (mod & GDK_CONTROL_MASK) { 
		if (pressed) (keyboard->control)++; else (keyboard->control)--;
		if (keyboard->control) keyboard->modstatus|=mod; else keyboard->modstatus&=~mod;
	}
	else if (pressed) keyboard->modstatus|=mod; else keyboard->modstatus&=~mod;

	for (i=0;i<256;i++) {
		if (keyboard->keys[i]) key_switch_mode(keyboard->keys[i], keyboard->modstatus);
	}
}

void keyboard_key_press(struct key *key)
{
	key_set_color(key, KEY_ACTIVATED_COLOR);
	SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_PRESS);
	if (key->modifier & GDK_LOCK_MASK) SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_RELEASE);
	key->pressed=TRUE;
	if (key->modifier) keyboard_switch_mode(key_get_keyboard(key), key->modifier, TRUE);
}

void keyboard_key_release(struct key *key)
{
	if (key->modifier & GDK_LOCK_MASK) SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_PRESS);
	SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_RELEASE);
	key->pressed=FALSE;
	key_set_color(key, KEY_COLOR);
	if (key->modifier) keyboard_switch_mode(key_get_keyboard(key), key->modifier, FALSE);
}

gboolean keyboard_leave_event (GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	if (keyboard->pressed && keyboard->pressed->pressed && !keyboard->pressed->modifier)
		keyboard_key_release(keyboard->pressed);
	if (keyboard->current) key_set_color(keyboard->current, keyboard->current->pressed?KEY_ACTIVATED_COLOR:KEY_COLOR);
	keyboard->current=NULL;
	return FALSE;
}

gboolean keyboard_press_timeout (gpointer data)
{
	struct key *key=(struct key *)data;
	struct keyboard *keyboard=key_get_keyboard(key);
	gboolean ret=TRUE;
	if (keyboard->timer>=1.0) {
		gboolean pressed=key->pressed;
		if (!key->modifier || !pressed) keyboard_key_press(key);
		if (!key->modifier || (key->modifier && pressed)) keyboard_key_release(key);
		ret=FALSE;
	}
	if ((keyboard->timer==0.0) || (!ret) || (keyboard->current!=key)) {
		key_update_timer(key, 0.0); ret=FALSE;
	}
	else { key_update_timer(key, keyboard->timer); keyboard->timer+=keyboard->timer_step; }
	return ret;
}

gboolean keyboard_handle_event (GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	guchar code;
	struct key *key=NULL;
	struct key *data=(struct key *)user_data;
	struct keyboard *keyboard=key_get_keyboard(data);
	guint x, y;

	switch (event->type) {
		case GDK_MOTION_NOTIFY:
			gnome_canvas_w2c(keyboard->canvas, ((GdkEventMotion*)event)->x, 
				((GdkEventMotion*)event)->y, &x, &y);
			break;
		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_w2c(keyboard->canvas, ((GdkEventCrossing*)event)->x, 
				((GdkEventCrossing*)event)->y, &x, &y);
			break;
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			gnome_canvas_w2c(keyboard->canvas, ((GdkEventButton*)event)->x, 
				((GdkEventButton*)event)->y, &x, &y);
			break;
		default:
			return FALSE;
	}
	if (x<0 || y<0 || x>=keyboard->width || y>=keyboard->height) return FALSE;
	code=keyboard->map[x+(keyboard->width*y)];
	if (code) key=keyboard->keys[code];

	if (keyboard->current!=key && (event->type==GDK_ENTER_NOTIFY || event->type==GDK_LEAVE_NOTIFY ||
		event->type==GDK_MOTION_NOTIFY)) {
		if (keyboard->timer_step==0.0) {
			if (key) {
				key_set_color(key, KEY_MOUSE_OVER_COLOR);
			}
			if (keyboard->current && keyboard->current!=key) { 
				key_set_color(keyboard->current, keyboard->current->pressed?KEY_ACTIVATED_COLOR:KEY_COLOR);
				keyboard->current=NULL;
			}
		} else {
			if (key) {keyboard->timer=keyboard->timer_step;
			g_timeout_add(FLO_ANIMATION_PERIOD, keyboard_press_timeout, key);}
			else { keyboard->current=NULL; }
		}
		keyboard->current=key;
	} else if (event->type==GDK_BUTTON_PRESS && (!data->modifier || !data->pressed)) {
		keyboard->timer=0.0; keyboard->pressed=data;
		keyboard_key_press(data);
	} else if ((event->type==GDK_BUTTON_RELEASE && !data->modifier) || 
		(event->type==GDK_BUTTON_PRESS && key->modifier && data->pressed)) {
		keyboard->timer=0.0;
		keyboard_key_release(data);
	}

	return FALSE;
}

struct key *keyboard_insertkey (struct keyboard *keyboard, enum key_class class, unsigned char code, double x,
	double y, double w, double h, char *label)
{
	GnomeCanvasClipgroup *group;
	GdkModifierType mod=0;
	guint *keyvals;
	guint len;
	gchar *name;

	flo_debug("[new key] class=%d code=%d x=%f y=%f w=%f h=%f label=%s", class, code, x, y, w, h, label);
        group=(GnomeCanvasClipgroup *)gnome_canvas_item_new(keyboard->canvas_group, GNOME_TYPE_CANVAS_CLIPGROUP, 
		"x", x*2.0, "y", y*2.0, NULL);

	gdk_keymap_get_entries_for_keycode(NULL, code, NULL, &keyvals, &len);
	name=gdk_keyval_name(*keyvals);
	if (name) {
		if ((!strcmp(name, "Shift_L"))||(!strcmp(name, "Shift_R"))) mod=GDK_SHIFT_MASK;
		else if ((!strcmp(name, "Control_L"))||(!strcmp(name, "Control_R"))) mod=GDK_CONTROL_MASK;
		else if (!strcmp(name, "Alt")) mod=GDK_MOD1_MASK;
		else if (!strcmp(name, "ISO_Level3_Shift")) mod=GDK_MOD5_MASK; /* AltGr */
		else if (!strcmp(name, "Caps_Lock")) mod=GDK_LOCK_MASK;
		else if ((!strcmp(name, "Super_L"))||(!strcmp(name, "Super_R"))) mod=GDK_MOD4_MASK;
	}

	keyboard->keys[code]=key_new(keyboard, code, group, mod, label);
	key_draw(keyboard->keys[code], class, w, h);
	gtk_signal_connect(GTK_OBJECT(group), "event", GTK_SIGNAL_FUNC(keyboard_handle_event), keyboard->keys[code]);
	return keyboard->keys[code];
}

void keyboard_change_color (struct keyboard *keyboard, GConfEntry *entry, enum colours colclass)
{
	guint i;
	gchar *color=(gchar *)gconf_value_get_string(gconf_entry_get_value(entry));
	key_update_color(colclass, color);
	for(i=0;i<256;i++) {
		if (keyboard->keys[i]) {
			switch(colclass) {
				case KEY_COLOR: 
					if (!keyboard->keys[i]->pressed) key_set_color(keyboard->keys[i], colclass);
					break;
				case KEY_TEXT_COLOR:
					key_update_text_color(keyboard->keys[i]);
					break;
				case KEY_ACTIVATED_COLOR:
					if (keyboard->keys[i]->pressed) key_set_color(keyboard->keys[i], colclass);
				case KEY_MOUSE_OVER_COLOR:
					/* unlikely and if that ever happen, just move to another key */
					break;
				default:
					flo_error(_("Should never happen: unknown color class updated"));
					break;
			}
		}
	}
}

void keyboard_change_key_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, KEY_COLOR);
}

void keyboard_change_text_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, KEY_TEXT_COLOR);
}

void keyboard_change_activated_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, KEY_ACTIVATED_COLOR);
}

void keyboard_change_mouseover_color (GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	keyboard_change_color(keyboard, entry, KEY_MOUSE_OVER_COLOR);
}

void keyboard_change_auto_click(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)user_data;
	gdouble click_time=gconf_value_get_float(gconf_entry_get_value(entry));
	if (click_time>=0.01)
		keyboard->timer_step=FLO_ANIMATION_PERIOD/click_time;
	else
		keyboard->timer_step=0.0;
}

void keyboard_settings_connect(struct keyboard *keyboard)
{
	settings_changecb_register("colours/key", keyboard_change_key_color, (gpointer)keyboard);
	settings_changecb_register("colours/label", keyboard_change_text_color, (gpointer)keyboard);
	settings_changecb_register("colours/activated", keyboard_change_activated_color, (gpointer)keyboard);
	settings_changecb_register("colours/mouseover", keyboard_change_mouseover_color, (gpointer)keyboard);
	settings_changecb_register("behaviour/auto_click", keyboard_change_auto_click, (gpointer)keyboard);
}

void keyboard_setsize(void *userdata, double width, double height)
{
	struct keyboard *keyboard=(struct keyboard *)userdata;
	keyboard->dwidth=2.0*width;
	keyboard->dheight=2.0*height;
	gnome_canvas_set_scroll_region(keyboard->canvas, 0.0, 0.0, keyboard->dwidth, keyboard->dheight);
        gnome_canvas_set_pixels_per_unit(keyboard->canvas, keyboard->zoom);
	gnome_canvas_w2c(keyboard->canvas, keyboard->dwidth, keyboard->dheight, &(keyboard->width), &(keyboard->height));
	keyboard->map=g_malloc(sizeof(guchar)*keyboard->width*keyboard->height);
	if (!keyboard->map) flo_fatal(_("Unable to allocate memory for map."));
	memset(keyboard->map, 0, sizeof(guchar)*keyboard->width*keyboard->height);
}

struct keyboard *keyboard_new (GnomeCanvas *canvas)
{
	struct keyboard *keyboard=NULL;
	gchar *colours[NUM_COLORS];
	gdouble click_time;
	guint i;

	if (!(keyboard=g_malloc(sizeof(struct keyboard)))) flo_fatal(_("Unable to allocate memory for keyboard"));
	memset(keyboard, 0, sizeof(struct keyboard));
	if (!(keyboard->keys=g_malloc(256*sizeof(struct key *)))) flo_fatal(_("Unable to allocate memory for keys"));
	memset(keyboard->keys, 0, 256*sizeof(struct key *));

	click_time=settings_get_double("behaviour/auto_click");
	if (click_time<=0.0) keyboard->timer_step=0.0; else keyboard->timer_step=FLO_ANIMATION_PERIOD/click_time;

	keyboard->canvas=canvas;
	keyboard->zoom=settings_get_double("window/zoom");
	keyboard->canvas_group=gnome_canvas_root(canvas);

	colours[KEY_COLOR]=(gchar *)settings_get_string("colours/key");
	colours[KEY_OUTLINE_COLOR]=(gchar *)settings_get_string("colours/outline");
	colours[KEY_TEXT_COLOR]=(gchar *)settings_get_string("colours/label");
	colours[KEY_ACTIVATED_COLOR]=(gchar *)settings_get_string("colours/activated");
	colours[KEY_MOUSE_OVER_COLOR]=(gchar *)settings_get_string("colours/mouseover");

	key_init(colours);
	layoutreader_iterate(settings_get_string("layout/file"), keyboard_insertkey, keyboard_setsize, keyboard);
	gtk_signal_connect(GTK_OBJECT(canvas), "leave-notify-event", GTK_SIGNAL_FUNC(keyboard_leave_event), keyboard);

	keyboard_settings_connect(keyboard);
	return keyboard;
}

void keyboard_free (struct keyboard *keyboard)
{
	int i;
	for (i=0;i<256;i++) if (keyboard->keys[i]) key_free(keyboard->keys[i]);
	key_exit();
	g_free(keyboard->keys);
	g_free(keyboard->map);
	g_free(keyboard);
}

