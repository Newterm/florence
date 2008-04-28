/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008 François Agrech

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#include "system.h"
#include "florence.h"
#include "trace.h"
#include "trayicon.h"
#include "settings.h"
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <cspi/spi.h>
#include <libxml/xmlreader.h>

/* Time in ms between checks for SPI events */
#define FLO_SPI_TIME_INTERVAL 100

GSList *extensions=NULL; /* Main list of keyboard extensions */
struct key **keys=NULL; /* key map indexed by key codes. Note: key[0] doesn't exist. */
guint flo_width, flo_height; /* Total width and height in pixel of the keyboard including visible extensions */
/* TRUE=> the keyboard is always visible. FALSE=> the keyboard is only shown on editable widgets */
gboolean always_on_screen=TRUE; 

/* Called on destroy event (systray quit or close window) */
void flo_destroy (void)
{
	gtk_exit (0);
}

/* Called when always_on_screen is FALSE when a widget is focused.
 * Check if the widget is editable and show the keyboard or hide if not. */
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

/* Shouldn't be used but it seems like we need to traverse accessible widgets when a new window is open to trigger
 * focus events. This is at least the case for gedit. Will need to check how this all work. */
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

/* Called when a window is created */
void flo_window_create_event (const AccessibleEvent *event, gpointer user_data)
{
	/* For some reason, focus state change does happen after traverse 
	 * ==> did I misunderstand? */
	/* TODO: remettre le keyboard au front. Attention: always_on_screen désactive cette fonction */
	flo_traverse(event->source);
}

/* Called at regular FLO_SPI_TIME_INTERVAL periodic time
 * dispatches spi events (includes widget focus and windows creation/destruction) */
gboolean flo_spi_event_check (gpointer data)
{
	AccessibleEvent *event;
	while (event=SPI_nextEvent(FALSE)) {}
	return !always_on_screen;
}

/* This function fills data bitmap with one line of keyboard's byte map */
void flo_fill_mask_line(guchar *data, struct keyboard *keyboard, guint xoffset, guint yoffset,
	guint y, guint rowstride)
{
	guint x;
	for (x=0;x<keyboard_get_width(keyboard);x++) {
		/* here we may have a big/little endian problem
		 * unless gdk or Xlib handles it? 
		 * ==> TODO : check and use autotools' config.h if necessary */
		if (keyboard_get_map(keyboard)[x+((y-yoffset)*keyboard_get_width(keyboard))]) {
			data[(x+xoffset+(y*rowstride))>>3]|=1<<((x+xoffset)&7);
		} else {
			data[(x+xoffset+(y*rowstride))>>3]&=(~(1<<((x+xoffset)&7)));
		}
	}
}

/* In order to make the background is transparent (when you can see between what's behind the keyboard),
 * this function creates a gdk bitmap mask representing the shape of the keyboard. 
 * args: window is the gdk window to mask and shape is true if we have to mask and false to delete the mask */
void flo_set_mask(GdkWindow *window, gboolean shape)
{
	GSList *list;
	struct extension *extension;
	struct keyboard *keyboard;
	guint y, xoffset=0, yoffset=0;
	GdkBitmap *mask=NULL;
	guint width;
	guchar *data=NULL;
	guchar byte;

	if (shape) {
		width=(flo_width&0xFFFFFFF8)+(flo_width&0x7?8:0);
		data=g_malloc((sizeof(guchar)*width*flo_height)>>3);
		if (!data) flo_fatal(_("Unable to allocate memory for mask"));

		/* case when there is an extension at the top */
		if (keyboard_get_height(((struct extension *)(extensions->data))->keyboard)<flo_height) {
			list=extensions;
			while (list && ((struct extension *)(list->data))->placement!=LAYOUT_TOP)
				list=g_slist_next(list);
			extension=(struct extension *)(list->data);
			for (y=0;y<(yoffset=keyboard_get_height(extension->keyboard));y++) {
				flo_fill_mask_line(data, extension->keyboard, 0, 0, y, width);
			}
		} 
		for (y=yoffset;y<flo_height;y++) {
			xoffset=0;
			list=extensions;
			while (list) {
				extension=(struct extension *)(list->data);
				if ((extension->is_active) && extension->placement!=LAYOUT_TOP) {
					flo_fill_mask_line(data, extension->keyboard,
						xoffset, yoffset, y, width);
					xoffset+=keyboard_get_width(extension->keyboard);
				}
				list=g_slist_next(list);
			}
		}
		mask=gdk_bitmap_create_from_data(window, data, flo_width, flo_height);
		if (!mask) flo_fatal(_("Unable to create mask"));
		gdk_window_shape_combine_mask(window, mask, 0, 0);

		g_object_unref(G_OBJECT(mask));
		g_free(data);
	} else {
		gdk_window_shape_combine_mask(window, NULL, 0, 0);
	}
}

/* Switches between always on screen mode and hidden mode.
 * When in hidden mode, the spi events are registered to monitor focus and show on editable widgets.
 * the events are deregistered when always on screen mode is activated */
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

/* Triggered by gconf when the "shaped" parameter is changed. Calls flo_set_mask accordingly. */
void flo_set_shaped(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	gtk_widget_show(GTK_WIDGET(user_data));
	flo_set_mask(gtk_widget_get_parent_window(
		GTK_WIDGET(g_list_first(gtk_container_get_children(GTK_CONTAINER(user_data)))->data)),
		gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* Triggered by gconf when the "decorated" parameter is changed. Decorates or undecorate the window. */
void flo_set_decorated(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	gtk_window_set_decorated(GTK_WINDOW(user_data), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* Triggered by gconf when the "always_on_screen" parameter is changed. */
void flo_set_show_on_focus(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	flo_switch_mode(GTK_WIDGET(user_data), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* When the zoom parameter is changed, each extension ust be resized. This function resizes one extension
 * and is called glib's foreach function on each menber of the extension list */
void flo_resize_extension(gpointer data, gpointer user_data)
{
	struct extension *extension=(struct extension *)data;
	struct keyboard *keyboard=extension->keyboard;
	gdouble *zoom=(gdouble *)user_data;
	keyboard_resize(keyboard, *zoom);
	if (extension->is_active) {
		switch(extension->placement) {
			case LAYOUT_VOID:
				flo_width=keyboard_get_width(keyboard);
				flo_height=keyboard_get_height(keyboard);
				break;
			case LAYOUT_TOP:
			case LAYOUT_BOTTOM:
				flo_height+=keyboard_get_height(keyboard);
				break;
			case LAYOUT_LEFT:
			case LAYOUT_RIGHT:
				flo_width+=keyboard_get_width(keyboard);
				break;
		}
	}
}

/* Triggered by gconf when the "zoom" parameter is changed. Resizes each extension. */
void flo_set_zoom(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	gdouble zoom;
	zoom=gconf_value_get_float(gconf_entry_get_value(entry));
	flo_width=0; flo_height=0;
	g_slist_foreach(extensions, flo_resize_extension, (gpointer)&zoom);
	gtk_widget_set_size_request(GTK_WIDGET(user_data), flo_width, flo_height);
	flo_set_mask(gtk_widget_get_parent_window(
		GTK_WIDGET(g_list_first(gtk_container_get_children(GTK_CONTAINER(user_data)))->data)),
		settings_get_bool("window/shaped"));
}

/* Callback called by the layour reader while parsing the layout file. Provides informations about the layout */
void flo_layout_infos(char *name, char *version)
{
	flo_info("Layout name: \"%s\"", name);
	if (strcmp(version, VERSION)) {
		flo_warn(_("Layout version %s is different from program version %s"), version, VERSION);
	}
}

/* Checks a colon separated list of strings for the presence of a particular string.
 * This is useful to check the "extensions" gconf parameter which is a list of colon separated strings */
gboolean flo_extension_activated(char *name)
{
	gboolean ret=FALSE;
	gchar *allextstr=NULL;
	gchar **extstrs=NULL;
	gchar **extstr=NULL;
	if (allextstr=settings_get_string("layout/extensions")) {
		extstrs=g_strsplit(allextstr, ":", -1);
		extstr=extstrs;
		while (extstr && *extstr) {
			if (!strcmp(name, *(extstr++))) { ret=TRUE; break; }
		}
		g_strfreev(extstrs);
	}
	return ret;
}

/* Called one time for each extension to actually create and display it.
 * reader is a pointer to the layout file where the extension configuration can be read. */
void flo_add_extension(xmlTextReaderPtr reader, char *name, enum layout_placement placement, void *userdata)
{
	GtkWidget *canvas;
	struct extension *extension=NULL;
	GtkContainer *vbox=GTK_CONTAINER(userdata);
	GtkContainer *hbox=GTK_CONTAINER(gtk_container_children(vbox)->data);

	extension=(struct extension *)g_malloc(sizeof(struct extension));
	canvas=gnome_canvas_new_aa();
	extension->keyboard=keyboard_new((GnomeCanvas *)canvas, keys, reader, extensions?2:1);
	if (!extension) flo_fatal(_("Unable to allocate memory for extension"));

	/* Note: currently, right and left, top and bottom are treated like right and top,
	   so left is like right and bottom is like top. It is not yet necessary
	   to differenciate as the only existing layout (standard layout) doesn't use left and bottom */
	if (placement==LAYOUT_TOP||placement==LAYOUT_BOTTOM) gtk_box_pack_start(GTK_BOX(vbox), canvas, FALSE, TRUE, 0);
	else gtk_container_add(hbox, canvas);
	if (extension->is_active=(!extensions||flo_extension_activated(name))) { switch (placement) {
		case LAYOUT_VOID:
			flo_width=keyboard_get_width(extension->keyboard);
			flo_height=keyboard_get_height(extension->keyboard);
			break;
		case LAYOUT_LEFT:
		case LAYOUT_RIGHT:
			flo_width+=keyboard_get_width(extension->keyboard);
			break;
		case LAYOUT_BOTTOM:
		case LAYOUT_TOP:
			flo_height+=keyboard_get_height(extension->keyboard);
			break;
		default:
			flo_fatal("Unknown placement type: %d", placement);
	} gtk_widget_show(canvas); } else { gtk_widget_hide(canvas); }

	if (name) {
		extension->name=g_malloc(sizeof(gchar)*(strlen(name)+1));
		strcpy(extension->name, name);
	} else {
		extension->name=NULL;
	}
	extension->placement=placement;
	extensions=g_slist_append(extensions, (gpointer)extension);
}

/* Show or hide one extension, according to the colon separated list of extension names.
 * if the name is in the list, the extension is shown, otherwise, it's hidden.
 * data contains the extension data. */
void flo_update_extension(gpointer data, gpointer user_data)
{
	struct extension *extension=(struct extension *)data;
	if (extension->name) {
		if (extension->is_active=flo_extension_activated(extension->name)) {
			/* Here we don't care about the bottom case because it's not currently used */
			if (extension->placement==LAYOUT_TOP) {
				flo_height+=keyboard_get_height(extension->keyboard);
			} else {
				flo_width+=keyboard_get_width(extension->keyboard);
			}
			gtk_widget_show(GTK_WIDGET(keyboard_get_canvas(extension->keyboard)));
		} else {
			gtk_widget_hide(GTK_WIDGET(keyboard_get_canvas(extension->keyboard)));
		}
	}
}

/* Callback triggered by gconf when the list of extensions to show changes.
 * check each extension and show only those that should be visible. */
void flo_update_extensions(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct extension *extension=(struct extension *)extensions->data;
	flo_width=keyboard_get_width(extension->keyboard);
	flo_height=keyboard_get_height(extension->keyboard);
	g_slist_foreach(extensions, flo_update_extension, NULL);
	gtk_widget_set_size_request(GTK_WIDGET(user_data), flo_width, flo_height);
	if (settings_get_bool("window/shaped")) {
		flo_set_mask(gtk_widget_get_parent_window(
			GTK_WIDGET(g_list_first(gtk_container_get_children(GTK_CONTAINER(user_data)))->data)),
			TRUE);
	}
}

/* Deallocates memory used by an extension */
void flo_free_extension(gpointer data, gpointer user_data)
{
	struct extension *extension=(struct extension *)data;
	if (extension) {
		if (extension->name) g_free(extension->name);
		if (extension->keyboard) keyboard_free(extension->keyboard);
		g_free(extension);
	}
}

/* Registers above functions to gconf changes */
void flo_register_settings_cb(GtkWidget *window)
{
	settings_changecb_register("window/shaped", flo_set_shaped, window);
	settings_changecb_register("window/decorated", flo_set_decorated, window);
	settings_changecb_register("behaviour/always_on_screen", flo_set_show_on_focus, window);
	settings_changecb_register("window/zoom", flo_set_zoom, window);
	settings_changecb_register("layout/extensions", flo_update_extensions, window);
}

/* This is the main function.
 * Creates the window, the widgets and the extensions.
 * Registers the event callbacks.
 * Call the event loop.
 * Cleans up at exit. */
int florence (void)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	xmlTextReaderPtr layout;

	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(flo_destroy), NULL);

	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

	settings_init(FALSE);
	flo_register_settings_cb(window);

	if (!(keys=g_malloc(256*sizeof(struct key *)))) flo_fatal(_("Unable to allocate memory for keys"));
	memset(keys, 0, 256*sizeof(struct key *));
	layout=layoutreader_new(settings_get_string("layout/file"));
	layoutreader_readinfos(layout, flo_layout_infos);
	key_init(layout);
	vbox=gtk_vbox_new(FALSE, 0);
	hbox=gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	flo_add_extension(layout, NULL, LAYOUT_VOID, (void *)vbox);
	while (layoutreader_readextension(layout, flo_add_extension, (void *)vbox));
	layoutreader_free(layout);
	
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(hbox);
	gtk_widget_show(vbox);

	gtk_window_set_decorated((GtkWindow *)window, settings_get_bool("window/decorated"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	gtk_widget_set_size_request(GTK_WIDGET(window), flo_width, flo_height);
	if (settings_get_bool("window/shaped")) {
		gtk_widget_show(window);
		flo_set_mask(gtk_widget_get_parent_window(hbox), TRUE);
	}
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	SPI_init ();
	flo_switch_mode(window, settings_get_bool("behaviour/always_on_screen"));

	trayicon_create(window, G_CALLBACK(flo_destroy));
	gtk_main();

	settings_exit();
	if (extensions) {
		g_slist_foreach(extensions, flo_free_extension, NULL);
		g_slist_free(extensions);
	}
	if (keys) g_free(keys);
	return SPI_exit();
}

