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

#define FLO_ANIMATION_TIMEOUT 50

struct key *keyboard[256];
GnomeCanvasGroup *keyboard_canvas_group=NULL;
GnomeCanvas *keyboard_canvas=NULL;
guchar *keyboard_map;
guint keyboard_width, keyboard_height;
GdkModifierType keyboard_modstatus=0;
guint keyboard_shift=0;
guint keyboard_control=0;
struct key *keyboard_current=NULL;
struct key *keyboard_pressed=NULL;
static gdouble keyboard_timer=0.0;
gdouble keyboard_timer_step=0.0;

guint keyboard_get_width(void)
{
	return keyboard_width;
}

guint keyboard_get_height(void)
{
	return keyboard_height;
}

void keyboard_switch_mode(GdkModifierType mod, gboolean pressed)
{
	int i;

	/* assumption : only the keyboard_shift and keyboard_control keys are double */
	if (mod & GDK_SHIFT_MASK) { 
		if (pressed) keyboard_shift++; else keyboard_shift--;
		if (keyboard_shift) keyboard_modstatus|=mod; else keyboard_modstatus&=~mod;
	}
	else if (mod & GDK_CONTROL_MASK) { 
		if (pressed) keyboard_control++; else keyboard_control--;
		if (keyboard_control) keyboard_modstatus|=mod; else keyboard_modstatus&=~mod;
	}
	else if (pressed) keyboard_modstatus|=mod; else keyboard_modstatus&=~mod;

	for (i=0;i<256;i++) {
		if (keyboard[i]) key_switch_mode(keyboard[i], keyboard_modstatus);
	}
}

void keyboard_key_press(struct key *key)
{
	key_set_color(key, KEY_ACTIVATED_COLOR);
	SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_PRESS);
	if (key->modifier & GDK_LOCK_MASK) SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_RELEASE);
	key->pressed=TRUE;
	if (key->modifier) keyboard_switch_mode(key->modifier, TRUE);
}

void keyboard_key_release(struct key *key)
{
	if (key->modifier & GDK_LOCK_MASK) SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_PRESS);
	SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_RELEASE);
	key->pressed=FALSE;
	key_set_color(key, KEY_COLOR);
	if (key->modifier) keyboard_switch_mode(key->modifier, FALSE);
}

gboolean keyboard_leave_event (GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	if (keyboard_pressed && keyboard_pressed->pressed && !keyboard_pressed->modifier) keyboard_key_release(keyboard_pressed);
	keyboard_current=NULL;
	return FALSE;
}

gboolean keyboard_press_timeout (gpointer data)
{
	struct key *key=(struct key *)data;
	gboolean ret=TRUE;
	if (keyboard_timer>=1.0) {
		gboolean pressed=key->pressed;
		if (!key->modifier || !pressed) keyboard_key_press(key);
		if (!key->modifier || (key->modifier && pressed)) keyboard_key_release(key);
		ret=FALSE;
	}
	if ((keyboard_timer==0.0) || (!ret) || (keyboard_current!=key)) {
		key_update_timer(key, 0.0); ret=FALSE;
	}
	else { key_update_timer(key, keyboard_timer); keyboard_timer+=keyboard_timer_step; }
	return ret;
}

gboolean keyboard_handle_event (GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	guchar code;
	struct key *key=NULL;
	struct key *data=(struct key *)user_data;
	guint x, y;

	switch (event->type) {
		case GDK_MOTION_NOTIFY:
			gnome_canvas_w2c(keyboard_canvas, ((GdkEventMotion*)event)->x, 
				((GdkEventMotion*)event)->y, &x, &y);
			break;
		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_w2c(keyboard_canvas, ((GdkEventCrossing*)event)->x, 
				((GdkEventCrossing*)event)->y, &x, &y);
			break;
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			gnome_canvas_w2c(keyboard_canvas, ((GdkEventButton*)event)->x, 
				((GdkEventButton*)event)->y, &x, &y);
			break;
		default:
			return FALSE;
	}
	if (x<0 || y<0 || x>=keyboard_width || y>=keyboard_height) return FALSE;
	code=keyboard_map[x+(keyboard_width*y)];
	if (code) key=keyboard[code];

	if (keyboard_current!=key && (event->type==GDK_ENTER_NOTIFY || event->type==GDK_LEAVE_NOTIFY ||
		event->type==GDK_MOTION_NOTIFY)) {
		keyboard_current=key;
		if (keyboard_timer_step==0.0) {
			if (key) {
				key_set_color(key, KEY_MOUSE_OVER_COLOR);
			}
			else { 
				key_set_color(key, key->pressed?KEY_ACTIVATED_COLOR:KEY_COLOR);
				keyboard_current=NULL;
			}
		} else {
			if (key) {keyboard_timer=keyboard_timer_step; g_timeout_add(FLO_ANIMATION_TIMEOUT, keyboard_press_timeout, key);}
			else { keyboard_current=NULL; }
		}
	} else if (event->type==GDK_BUTTON_PRESS && (!data->modifier || !data->pressed)) {
		keyboard_timer=0.0; keyboard_pressed=data;
		keyboard_key_press(data);
	} else if ((event->type==GDK_BUTTON_RELEASE && !data->modifier) || 
		(event->type==GDK_BUTTON_PRESS && key->modifier && data->pressed)) {
		keyboard_timer=0.0;
		keyboard_key_release(data);
	}

	return FALSE;
}

struct key *keyboard_insertkey (char *code, char *label, double x, double y)
{
	guint icode;
	GnomeCanvasClipgroup *group;
	GdkModifierType mod=0;
	guint *keyvals;
	guint len;
	gchar *name;

	if (1!=sscanf(code, "%d", &icode)) flo_fatal(_("Invalid key."));
        group=(GnomeCanvasClipgroup *)gnome_canvas_item_new(keyboard_canvas_group, GNOME_TYPE_CANVAS_CLIPGROUP, 
		"x", x*2.0, "y", y*2.0, NULL);

	gdk_keymap_get_entries_for_keycode(NULL, icode, NULL, &keyvals, &len);
	name=gdk_keyval_name(*keyvals);
	if (name) {
		if ((!strcmp(name, "Shift_L"))||(!strcmp(name, "Shift_R"))) mod=GDK_SHIFT_MASK;
		else if ((!strcmp(name, "Control_L"))||(!strcmp(name, "Control_R"))) mod=GDK_CONTROL_MASK;
		else if (!strcmp(name, "Alt")) mod=GDK_MOD1_MASK;
		else if (!strcmp(name, "ISO_Level3_Shift")) mod=GDK_MOD5_MASK; /* AltGr */
		else if (!strcmp(name, "Caps_Lock")) mod=GDK_LOCK_MASK;
		else if ((!strcmp(name, "Super_L"))||(!strcmp(name, "Super_R"))) mod=GDK_MOD4_MASK;
	}

	keyboard[icode]=key_new(icode, group, mod, label);
	return keyboard[icode];
}

void keyboard_create(gchar *file, gdouble scale, gchar *colours[])
{
	struct key *nkey=NULL;
        gchar **layout;
        gchar **browse;
	gdouble *params;
	gchar *label;
	gsize len;
	GKeyFile *gkf;

	flo_info(_("Using layout file <%s>"), file);
	gkf=g_key_file_new();
	if (!g_key_file_load_from_file(gkf, file, G_KEY_FILE_NONE, NULL))
		flo_fatal (_("Unable to load layout file"));

	gnome_canvas_set_scroll_region(keyboard_canvas,0.0,0.0,
		2.0*g_key_file_get_double(gkf, "Size", "Width", NULL),
		2.0*g_key_file_get_double(gkf, "Size", "Height", NULL));
	gnome_canvas_w2c(keyboard_canvas, 2.0*g_key_file_get_double(gkf, "Size", "Width", NULL),
		2.0*g_key_file_get_double(gkf, "Size", "Height", NULL), &keyboard_width, &keyboard_height);
	keyboard_map=g_malloc(sizeof(guchar)*keyboard_width*keyboard_height);
	memset(keyboard_map, 0, sizeof(guchar)*keyboard_width*keyboard_height);
	key_init(keyboard_canvas, colours, keyboard_map, scale/10.0);

        layout=g_key_file_get_keys(gkf, "Layout", NULL, NULL);
	if (!layout) flo_fatal (_("Pas de catégorie Layout dans le fichier"));
        browse=layout;
        while (*browse) {
                params=g_key_file_get_double_list(gkf, "Layout", *browse, &len, NULL);
		label=g_key_file_get_string(gkf, "Labels", *browse, NULL);
		if (!params) flo_fatal(_("Program error"));
		nkey=keyboard_insertkey(*browse, label, *params, *(params+1));
		if (len==2) key_draw2(nkey, *params, *(params+1));
		else if (len==4) key_draw4(nkey, *params, *(params+1), *(params+2), *(params+3));
		else flo_fatal(_("Invalid key arguments"));
		gtk_signal_connect(GTK_OBJECT(nkey->group), "event", GTK_SIGNAL_FUNC(keyboard_handle_event), nkey);
		g_free(label);
                g_free(params); 
		if (!nkey) flo_fatal (_("Unable to create key"));

		nkey=NULL;
		browse++;	
        }
        g_strfreev(layout);
	g_key_file_free(gkf);
}

void keyboard_init (GnomeCanvas *canvas)
{
	gchar *colours[NUM_COLORS];
	gdouble click_time;
	gdouble scale;
	guint i;

	for (i=0;i<256;i++) {
		keyboard[i]=NULL;
	}

	click_time=gconf_value_get_float(settings_get_value("behaviour/auto_click"));
	if (click_time<=0.0) keyboard_timer_step=0.0; else keyboard_timer_step=FLO_ANIMATION_TIMEOUT/click_time;

	keyboard_canvas=canvas;
	scale=gconf_value_get_float(settings_get_value("window/zoom"));
        gnome_canvas_set_pixels_per_unit(keyboard_canvas, scale);
	keyboard_canvas_group=gnome_canvas_root(canvas);

	colours[KEY_COLOR]=gconf_value_get_string(settings_get_value("colours/key"));
	colours[KEY_OUTLINE_COLOR]=gconf_value_get_string(settings_get_value("colours/outline"));
	colours[KEY_TEXT_COLOR]=gconf_value_get_string(settings_get_value("colours/label"));
	colours[KEY_ACTIVATED_COLOR]=gconf_value_get_string(settings_get_value("colours/activated"));
	colours[KEY_MOUSE_OVER_COLOR]=gconf_value_get_string(settings_get_value("colours/mouseover"));

	keyboard_create(gconf_value_get_string(settings_get_value("layout/file")), scale, colours);
	gtk_signal_connect(GTK_OBJECT(canvas), "leave-notify-event", GTK_SIGNAL_FUNC(keyboard_leave_event), NULL);
}

void keyboard_exit (void)
{
	int i;
	for (i=0;i<256;i++) if (keyboard[i]) key_free(keyboard[i]);
	key_exit();
	g_free(keyboard_map);
}

guchar *keyboard_get_map(void)
{
	return keyboard_map;
}

