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
#include "layoutreader.h"
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <cspi/spi.h>
#include <libxml/xmlreader.h>

#define FLO_SPI_TIME_INTERVAL 100

GSList *keyboards=NULL;
struct key **keys=NULL;
guint flo_width, flo_height;
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
				} else if (flo_width<(screen_width-x-w)) {
					x=screen_width-flo_width;
				}
				if (flo_height<(screen_height-y-h)) {
					gtk_window_move(window, x, y+h);
				} else if (y>flo_height) {
					gtk_window_move(window, x, y-flo_height);
				} else {
					gtk_window_move(window, x, screen_height-flo_height);
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
	GSList *list;
	struct keyboard *keyboard;
	int x, y, offset;
	GdkBitmap *mask=NULL;
	guint width=(flo_width&0xFFFFFFF8)+(flo_width&0x7?8:0);
	guchar *data=g_malloc((sizeof(guchar)*width*flo_height)>>3);
	guchar byte;

	if (!data) flo_fatal(_("Unable to allocate memory for mask"));

	if (shape) for (y=0;y<flo_height;y++) {
		list=keyboards; offset=0;
		while (list) {
			keyboard=(struct keyboard *)(list->data);
			for (x=0;x<keyboard_get_width(keyboard);x++) {
				/* here we may have a big/little endian problem
				 * unless gdk or Xlib handles it? 
				 * ==> TODO : check and use autotools' config.h if necessary */
				if (keyboard_get_map(keyboard)[x+(y*keyboard_get_width(keyboard))]) {
					data[(x+offset+(y*width))>>3]|=1<<((x+offset)&7);
				} else {
					data[(x+offset+(y*width))>>3]&=(~(1<<((x+offset)&7)));
				}
			}
			offset+=keyboard_get_width(keyboard);
			list=g_slist_next(list);
		}
	} else for (x=0;x<(width*flo_height)>>3;x++) data[x]=0xFF;

	mask=gdk_bitmap_create_from_data(window, data, flo_width, flo_height);
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
	gtk_window_set_decorated(GTK_WINDOW(user_data), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

void flo_set_show_on_focus(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	flo_switch_mode(GTK_WIDGET(user_data), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

void flo_resize_keyboard(gpointer data, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)data;
	gdouble *zoom=(gdouble *)user_data;
	keyboard_resize(keyboard, *zoom);
	flo_width+=keyboard_get_width(keyboard);
	flo_height=keyboard_get_height(keyboard);
}

void flo_set_zoom(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	gdouble zoom;
	zoom=gconf_value_get_float(gconf_entry_get_value(entry));
	flo_width=0; flo_height=0;
	g_slist_foreach(keyboards, flo_resize_keyboard, (gpointer)&zoom);
	gtk_widget_set_size_request(GTK_WIDGET(user_data), flo_width, flo_height);
	flo_set_mask(gtk_widget_get_parent_window(
		GTK_WIDGET(g_list_first(gtk_container_get_children(GTK_CONTAINER(user_data)))->data)),
		settings_get_bool("window/shaped"));
}

void flo_register_settings_cb(GtkWidget *window)
{
	settings_changecb_register("window/shaped", flo_set_shaped, window);
	settings_changecb_register("window/decorated", flo_set_decorated, window);
	settings_changecb_register("behaviour/always_on_screen", flo_set_show_on_focus, window);
	settings_changecb_register("window/zoom", flo_set_zoom, window);
}

void flo_layout_infos(char *name, char *version)
{
	flo_info("Layout name: \"%s\"", name);
	if (strcmp(version, VERSION)) {
		flo_warn(_("Layout version %s is different from program version %s"), version, VERSION);
	}
}

gboolean flo_extension_activated(char *name)
{
	gboolean ret=FALSE;
	gchar *extensionstr=NULL;
	gchar **extensions=NULL;
	gchar **extension=NULL;
	if (extensionstr=settings_get_string("layout/extensions")) {
		extensions=g_strsplit(extensionstr, ":", -1);
		extension=extensions;
		while (extension && *extension) {
			if (!strcmp(name, *(extension++))) { ret=TRUE; break; }
		}
		g_strfreev(extensions);
	}
	return ret;
}

void flo_add_extension(xmlTextReaderPtr reader, char *name, enum layout_placement placement, void *userdata)
{
	GtkWidget *canvas;
	struct keyboard *keyboard;
	GtkContainer *hbox=GTK_CONTAINER(userdata);

	if (!keyboards || flo_extension_activated(name)) {
		canvas=gnome_canvas_new_aa();
		keyboard=keyboard_new((GnomeCanvas *)canvas, keys, reader, keyboards?2:1);
		gtk_container_add(hbox, canvas);
		gtk_widget_show(canvas);
		keyboards=g_slist_append(keyboards, (gpointer)keyboard);

		switch (placement) {
			case LAYOUT_VOID:
				flo_width=keyboard_get_width(keyboard);
				flo_height=keyboard_get_height(keyboard);
				break;
			case LAYOUT_LEFT:
			case LAYOUT_RIGHT:
				flo_width+=keyboard_get_width(keyboard);
				break;
			case LAYOUT_DOWN:
			case LAYOUT_UP:
				flo_height+=keyboard_get_height(keyboard);
				break;
			default:
				flo_fatal("Unknown placement type: %d", placement);
		}
	}
}

void flo_free_keyboard(gpointer data, gpointer user_data)
{
	struct keyboard *keyboard=(struct keyboard *)data;
	keyboard_free(keyboard);
}

int florence (void)
{
	GtkWidget *window;
	GtkWidget *hbox;
	guint borderWidth;
	xmlTextReaderPtr layout;

	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(flo_destroy), NULL);

	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

	settings_init();
	flo_register_settings_cb(window);

        if (!(keys=g_malloc(256*sizeof(struct key *)))) flo_fatal(_("Unable to allocate memory for keys"));
        memset(keys, 0, 256*sizeof(struct key *));
	layout=layoutreader_new(settings_get_string("layout/file"));
	layoutreader_readinfos(layout, flo_layout_infos);
	hbox=gtk_hbox_new(FALSE, 0);
	flo_add_extension(layout, NULL, LAYOUT_VOID, (void *)hbox);
	while (layoutreader_readextension(layout, flo_add_extension, (void *)hbox));
	layoutreader_free(layout);
	
	gtk_container_add(GTK_CONTAINER(window), hbox);
	gtk_widget_show(hbox);

	gtk_window_set_decorated((GtkWindow *)window,
		settings_get_bool("window/decorated"));

	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	gtk_widget_set_size_request(GTK_WIDGET(window), flo_width, flo_height);
	if (settings_get_bool("window/shaped")) {
		gtk_widget_show(window);
		flo_set_mask(gtk_widget_get_parent_window(GTK_WIDGET(hbox)), TRUE);
	}
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	SPI_init ();
	flo_switch_mode(window, settings_get_bool("behaviour/always_on_screen"));

	trayicon_create(window, G_CALLBACK(flo_destroy));
	gtk_main();

	settings_exit();
	g_slist_foreach(keyboards, flo_free_keyboard, NULL);
	g_slist_free(keyboards);
	return SPI_exit();
}

