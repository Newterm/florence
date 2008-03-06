/* 
   florence - Florence is a simple virtual keyboard for Gnome.

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

#include "system.h"
#include "trace.h"
#include "keyboard.h"
#include "trayicon.h"
#include "settings.h"
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <cspi/spi.h>

#define FLO_SPI_TIME_INTERVAL 100

struct keyboard *keyboard;
gboolean always_on_screen=TRUE;

void flo_destroy (void)
{
	gtk_exit (0);
}

void flo_focus_event (const AccessibleEvent *event, void *user_data)
{
	GtkWindow *window=(GtkWindow *)user_data;
	AccessibleComponent *component;
	long int x, y, w, h;
	gint screen_width, screen_height;

	if (Accessible_isEditableText(event->source)) {
		if (event->detail1) {
			/* positionnement intelligent */
			component=Accessible_getComponent(event->source);
			if (component) {
				screen_height=gdk_screen_get_height(gdk_screen_get_default());
				screen_width=gdk_screen_get_width(gdk_screen_get_default());
				AccessibleComponent_getExtents(component, &x, &y, &w, &h, SPI_COORD_TYPE_SCREEN);
				if (x<0) { x=0;
				} else if (keyboard_get_width(keyboard)<(screen_width-x-w)) {
					x=screen_width-keyboard_get_width(keyboard);
				}
				if (keyboard_get_height(keyboard)<(screen_height-y-h)) {
					gtk_window_move(window, x, y+h);
				} else if (y>keyboard_get_height(keyboard)) {
					gtk_window_move(window, x, y-keyboard_get_height(keyboard));
				} else {
					gtk_window_move(window, x, screen_height-keyboard_get_height(keyboard));
				}
			}
			gtk_widget_show(GTK_WIDGET(window));
			/* Some winwow managers forget it */
			gtk_window_set_keep_above(window, TRUE);
		}
		else {
			gtk_widget_hide(GTK_WIDGET(window)); 
		}
	}
}

void flo_traverse (Accessible *obj)
{
	int n_children, i;
	Accessible *child;

	n_children = Accessible_getChildCount (obj);
	if (!Accessible_isTable (obj))
	{
		for (i = 0; i < n_children; ++i)
		{
			child=Accessible_getChildAtIndex (obj, i);
			flo_traverse(child);
			Accessible_unref(child);
		}
	}
}

void flo_window_create_event (const AccessibleEvent *event, gpointer user_data)
{
	/* For some reason, focus state change does happen after traverse 
	 * ==> did I misunderstand? */
	flo_traverse(event->source);
}

gboolean flo_spi_event_check (gpointer data)
{
	AccessibleEvent *event;
	while (event=SPI_nextEvent(FALSE)) {}
	return !always_on_screen;
}

void flo_set_mask(GdkWindow *window, gboolean shape)
{
	int x, y, o;
	GdkBitmap *mask=NULL;
	guchar *kbmap=keyboard_get_map(keyboard);
	guint width=(keyboard_get_width(keyboard)&0xFFFFFFF8)+(keyboard_get_width(keyboard)&0x7?8:0);
	guchar *data=g_malloc((sizeof(guchar)*width*keyboard_get_height(keyboard))>>3);
	guchar byte;

	if (!data) flo_fatal(_("Unable to allocate memory for mask"));
	for (y=0;y<keyboard_get_height(keyboard);y++) {
		for (x=0;x<(width>>3);x++) {
			if (shape) {
				byte=0;
				/* here with may have a big/little endian problem
				 * unless gdk or Xlib handles it? 
				 * ==> TODO : check and use autotools' config.h if necessary */
				for(o=7;o>=0;o--) {
					byte<<=1;
					if (kbmap[(x*8)+(y*keyboard_get_width(keyboard))+o]) byte|=1; else byte&=0xfe;
				}
			} else byte=0xFF;
			data[x+((y*width)>>3)]=byte;
		}
	}
	mask=gdk_bitmap_create_from_data(window, data, keyboard_get_width(keyboard), keyboard_get_height(keyboard));
	if (!mask) flo_fatal(_("Unable to create mask"));
	gdk_window_shape_combine_mask(window, mask, 0, 0);

	g_object_unref(G_OBJECT(mask));
	g_free(data);
}

void flo_switch_mode (GtkWidget *window, gboolean on_screen)
{
	static AccessibleEventListener *focus_listener;
	static AccessibleEventListener *window_listener;

	if (!on_screen) {
		gtk_widget_hide(window);
		focus_listener=SPI_createAccessibleEventListener (flo_focus_event, (void*)window);
		SPI_registerGlobalEventListener(focus_listener, "object:state-changed:focused");
		window_listener=SPI_createAccessibleEventListener (flo_window_create_event, NULL);
		SPI_registerGlobalEventListener(window_listener, "window:create");
		g_timeout_add(FLO_SPI_TIME_INTERVAL, flo_spi_event_check, NULL);
	} else {
		if (!always_on_screen) {
			SPI_deregisterGlobalEventListener(focus_listener, "object:state-changed:focused");
			SPI_deregisterGlobalEventListener(window_listener, "window:create");
		}
		gtk_widget_show(window);
	}

	always_on_screen=on_screen;
}

void flo_set_shaped(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	gtk_widget_show(GTK_WIDGET(user_data));
	flo_set_mask(gtk_widget_get_parent_window(
		GTK_WIDGET(g_list_first(gtk_container_get_children(GTK_CONTAINER(user_data)))->data)),
		gconf_value_get_bool(gconf_entry_get_value(entry)));
}

void flo_set_decorated(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	gtk_window_set_decorated(GTK_WIDGET(user_data), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

void flo_set_show_on_focus(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	flo_switch_mode(GTK_WIDGET(user_data), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

void flo_set_zoom(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	keyboard_resize(keyboard, gconf_value_get_float(gconf_entry_get_value(entry)));
	gtk_widget_set_size_request(GTK_WIDGET(user_data), keyboard_get_width(keyboard), keyboard_get_height(keyboard));
	flo_set_mask(gtk_widget_get_parent_window(
		GTK_WIDGET(g_list_first(gtk_container_get_children(GTK_CONTAINER(user_data)))->data)),
		gconf_value_get_bool(settings_get_value("window/shaped")));
}

void flo_register_settings_cb(GtkWidget *window)
{
	settings_changecb_register("window/shaped", flo_set_shaped, window);
	settings_changecb_register("window/decorated", flo_set_decorated, window);
	settings_changecb_register("behaviour/always_on_screen", flo_set_show_on_focus, window);
	settings_changecb_register("window/zoom", flo_set_zoom, window);
}

int florence (void)
{
	GtkWidget *window;
	GtkWidget *canvas;
	guint borderWidth;

	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(flo_destroy), NULL);

	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

	settings_init();
	flo_register_settings_cb(window);

	canvas=gnome_canvas_new_aa();
	keyboard=keyboard_new((GnomeCanvas *)canvas);
	gtk_container_add(GTK_CONTAINER(window), canvas);
	gtk_widget_show(canvas);

	gtk_window_set_decorated((GtkWindow *)window,
		gconf_value_get_bool(settings_get_value("window/decorated")));

	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	gtk_widget_set_size_request(GTK_WIDGET(window), keyboard_get_width(keyboard), keyboard_get_height(keyboard));
	if (gconf_value_get_bool(settings_get_value("window/shaped"))) {
		gtk_widget_show(window);
		flo_set_mask(gtk_widget_get_parent_window(GTK_WIDGET(canvas)), TRUE);
	}
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	SPI_init ();
	flo_switch_mode(window, gconf_value_get_bool(settings_get_value("behaviour/always_on_screen")));

	trayicon_create(window, flo_destroy);
	gtk_main();

	settings_exit();
	keyboard_free(keyboard);
	return SPI_exit();
}

