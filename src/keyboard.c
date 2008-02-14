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
#include "system.h"
#include "keyboard.h"
#include "key.h"

#define FLO_ANIMATION_TIMEOUT 50

struct key *keyboard[256];
GnomeCanvasGroup *canvas_group=NULL;
GnomeCanvas *canvas=NULL;
guchar *keyboard_map;
guint keyboard_width, keyboard_height;
GdkModifierType modstatus=0;
guint shift=0;
guint control=0;
struct key *current=NULL;
struct key *key_pressed=NULL;
static gdouble timer=0.0;
gdouble step=0.0;

void keyboard_switch_mode(GdkModifierType mod, gboolean pressed)
{
	int i;

	/* assumption : only the shift and control keys are double */
	if (mod & GDK_SHIFT_MASK) { 
		if (pressed) shift++; else shift--;
		if (shift) modstatus|=mod; else modstatus&=~mod;
	}
	else if (mod & GDK_CONTROL_MASK) { 
		if (pressed) control++; else control--;
		if (control) modstatus|=mod; else modstatus&=~mod;
	}
	else if (pressed) modstatus|=mod; else modstatus&=~mod;

	for (i=0;i<256;i++) {
		if (keyboard[i]) key_switch_mode(keyboard[i], modstatus);
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
	if (key_pressed && key_pressed->pressed && !key_pressed->modifier) keyboard_key_release(key_pressed);
	current=NULL;
	return FALSE;
}

gboolean keyboard_press_timeout (gpointer data)
{
	struct key *key=(struct key *)data;
	gboolean ret=TRUE;
	if (timer>=1.0) {
		gboolean pressed=key->pressed;
		if (!key->modifier || !pressed) keyboard_key_press(key);
		if (!key->modifier || (key->modifier && pressed)) keyboard_key_release(key);
		ret=FALSE;
	}
	if ((timer==0.0) || (!ret) || (current!=key)) {
		key_update_timer(key, 0.0); ret=FALSE;
	}
	else { key_update_timer(key, timer); timer+=step; }
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
			gnome_canvas_w2c(canvas, ((GdkEventMotion*)event)->x, 
				((GdkEventMotion*)event)->y, &x, &y);
			break;
		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_w2c(canvas, ((GdkEventCrossing*)event)->x, 
				((GdkEventCrossing*)event)->y, &x, &y);
			break;
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			gnome_canvas_w2c(canvas, ((GdkEventButton*)event)->x, 
				((GdkEventButton*)event)->y, &x, &y);
			break;
		default:
			return FALSE;
	}
	if (x<0 || y<0 || x>=keyboard_width || y>=keyboard_height) return FALSE;
	code=keyboard_map[x+(keyboard_width*y)];
	if (code) key=keyboard[code];

	if (current!=key && (event->type==GDK_ENTER_NOTIFY || event->type==GDK_LEAVE_NOTIFY ||
		event->type==GDK_MOTION_NOTIFY)) {
		current=key;
		if (step==0.0) {
			if (key) {
				key_set_color(key, KEY_MOUSE_OVER_COLOR);
			}
			else { 
				key_set_color(key, key->pressed?KEY_ACTIVATED_COLOR:KEY_COLOR);
				current=NULL;
			}
		} else {
			if (key) {timer=step; g_timeout_add(FLO_ANIMATION_TIMEOUT, keyboard_press_timeout, key);}
			else { current=NULL; }
		}
	} else if (event->type==GDK_BUTTON_PRESS && (!data->modifier || !data->pressed)) {
		timer=0.0; key_pressed=data;
		keyboard_key_press(data);
	} else if ((event->type==GDK_BUTTON_RELEASE && !data->modifier) || 
		(event->type==GDK_BUTTON_PRESS && key->modifier && data->pressed)) {
		timer=0.0;
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

	if (1!=sscanf(code, "%d", &icode)) fatal("Invalid key.");
        group=(GnomeCanvasClipgroup *)gnome_canvas_item_new(canvas_group, GNOME_TYPE_CANVAS_CLIPGROUP, 
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

void keyboard_init (GKeyFile *gkf, GnomeCanvas *lcanvas)
{
	struct key *nkey=NULL;
	gchar **general;
        gchar **layout;
        gchar **browse;
	gdouble *params;
	gchar *label;
	gsize len;
	gchar *colors[NUM_COLORS];
	gdouble click_time;
	guint i;

	for (i=0;i<256;i++) {
		keyboard[i]=NULL;
	}

	click_time=g_key_file_get_double(gkf, "Settings", "AutoClick", NULL);
	if (click_time<=0.0) step=0.0; else step=FLO_ANIMATION_TIMEOUT/click_time;

	canvas=lcanvas;
        gnome_canvas_set_pixels_per_unit(canvas,g_key_file_get_double(gkf, "Window", "zoom", NULL));
        gnome_canvas_set_scroll_region(canvas,0.0,0.0,
		2.0*g_key_file_get_double(gkf, "Window", "width", NULL),
		2.0*g_key_file_get_double(gkf, "Window", "height", NULL));
	canvas_group=gnome_canvas_root(canvas);

	colors[KEY_COLOR]=g_key_file_get_string(gkf, "Colors", "Key", NULL);
	colors[KEY_OUTLINE_COLOR]=g_key_file_get_string(gkf, "Colors", "Outline", NULL);
	colors[KEY_TEXT_COLOR]=g_key_file_get_string(gkf, "Colors", "Text", NULL);
	colors[KEY_ACTIVATED_COLOR]=g_key_file_get_string(gkf, "Colors", "Activated", NULL);
	colors[KEY_MOUSE_OVER_COLOR]=g_key_file_get_string(gkf, "Colors", "MouseOver", NULL);
	gtk_layout_get_size(GTK_LAYOUT(canvas), &keyboard_width, &keyboard_height);
	keyboard_map=g_malloc(sizeof(guchar)*keyboard_width*keyboard_height);
	memset(keyboard_map, 0, keyboard_width*keyboard_height);
	key_init(canvas, colors, keyboard_map);

        layout=g_key_file_get_keys(gkf, "Layout", NULL, NULL);
	if (!layout) fatal ("Pas de catégorie Layout dans le fichier");
        browse=layout;
        while (*browse) {
                params=g_key_file_get_double_list(gkf, "Layout", *browse, &len, NULL);
		label=g_key_file_get_string(gkf, "Labels", *browse, NULL);
		if (!params) fatal("Program error");
		nkey=keyboard_insertkey(*browse, label, *params, *(params+1));
		if (len==2) key_draw2(nkey, *params, *(params+1));
		else if (len==4) key_draw4(nkey, *params, *(params+1), *(params+2), *(params+3));
		else fatal("Invalid key arguments");
		gtk_signal_connect(GTK_OBJECT(nkey->group), "event", GTK_SIGNAL_FUNC(keyboard_handle_event), nkey);
		g_free(label);
                g_free(params); 
		if (!nkey) fatal ("Unable to create key");

		nkey=NULL;
		browse++;	
        }
        g_strfreev(layout);
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

