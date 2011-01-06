/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008, 2009, 2010 François Agrech

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

#include "florence.h"
#include "trace.h"
#include "settings.h"
#include "xkeyboard.h"
#include "keyboard.h"
#include "tools.h"
#include "layoutreader.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#ifdef ENABLE_AT_SPI
#include <cspi/spi.h>
#endif

/* bring the window back to front every seconds */
#define FLO_TO_TOP_TIMEOUT 1000

/* Called on destroy event (systray quit or close window) */
void flo_destroy (void)
{
#ifndef APPLET
	gtk_main_quit();
#endif
	//gtk_exit (0);
}

#ifdef ENABLE_AT_SPI
/* Called to destroy the icon */
void flo_icon_destroy (GtkWidget *widget, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
	if (florence->obj) {
		Accessible_unref(florence->obj);
		florence->obj=NULL;
	}
	florence->icon=NULL;
}

/* on button-press events: destroy the icon and show the actual keyboard */
void flo_icon_press (GtkWidget *window, GdkEventButton *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
	florence->icon=NULL;
	view_show(florence->view, florence->obj);
	if (florence->obj) {
		Accessible_unref(florence->obj);
		florence->obj=NULL;
	}
}

/* on expose event: display florence icon */
void flo_icon_expose (GtkWidget *window, GdkEventExpose* pExpose, void *userdata)
{
	cairo_t *context;
	RsvgHandle *handle;
	GError *error=NULL;

	context=gdk_cairo_create(window->window);
	cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);

	handle=rsvg_handle_new_from_file(ICONDIR "/florence.svg", &error);
	if (error) flo_error(_("Error loading florence icon: %s"), error->message);
	else {
		cairo_set_source_rgba(context, 0.0, 0.0, 0.0, 100.0);
		cairo_set_operator(context, CAIRO_OPERATOR_DEST_OUT);
		cairo_paint(context);
		cairo_set_operator(context, CAIRO_OPERATOR_OVER);
		style_render_svg(context, handle, settings_double_get("window/zoom")*2,
			settings_double_get("window/zoom")*2, TRUE, NULL);
		rsvg_handle_free(handle);
	}

	cairo_destroy(context);
}

/* Show an intermediate icon before showing the keyboard (if intermediate_icon is activated) 
 * otherwise, directly show the keyboard */
void flo_check_show (struct florence *florence, Accessible *obj)
{
	if (GTK_WIDGET_VISIBLE(florence->view->window)) view_hide(florence->view);
	if (settings_get_bool("behaviour/intermediate_icon")) {
		if (florence->obj) Accessible_unref(florence->obj);
		florence->obj=obj;
		if (!florence->icon) {
			florence->icon=GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
			gtk_window_set_keep_above(florence->icon, TRUE);
			gtk_window_set_skip_taskbar_hint(florence->icon, TRUE);
			gtk_widget_set_size_request(GTK_WIDGET(florence->icon), settings_double_get("window/zoom")*2,
				settings_double_get("window/zoom")*2);
			gtk_container_set_border_width(GTK_CONTAINER(florence->icon), 0);
			gtk_window_set_decorated(florence->icon, FALSE);
			gtk_window_set_position(florence->icon, GTK_WIN_POS_MOUSE);
			gtk_window_set_accept_focus(florence->icon, FALSE);
			gtk_widget_set_events(GTK_WIDGET(florence->icon), GDK_ALL_EVENTS_MASK);
			g_signal_connect(G_OBJECT(florence->icon), "expose-event", G_CALLBACK(flo_icon_expose), florence);
			g_signal_connect(G_OBJECT(florence->icon), "button-press-event",
				G_CALLBACK(flo_icon_press), florence);
			g_signal_connect(G_OBJECT(florence->view->window), "show",
				G_CALLBACK(flo_icon_destroy), florence);
			g_signal_connect(G_OBJECT(florence->icon), "screen-changed",
				G_CALLBACK(view_screen_changed), NULL);
			view_screen_changed(GTK_WIDGET(florence->icon), NULL, NULL);
		}
		tools_window_move(florence->icon, obj);
		gtk_widget_show(GTK_WIDGET(florence->icon));
	} else view_show(florence->view, obj);
}

/* Called when a widget is focused.
 * Check if the widget is editable and show the keyboard or hide if not. */
void flo_focus_event (const AccessibleEvent *event, void *user_data)
{
	struct florence *florence=(struct florence *)user_data;
	gboolean hide=FALSE;

	if (Accessible_getRole(event->source)==SPI_ROLE_TERMINAL || Accessible_isEditableText(event->source)) {
		if (event->detail1) {
			flo_check_show(florence, event->source);
			Accessible_ref(event->source);
		} else {
			hide=TRUE;
		}
	} else {
		hide=TRUE;
	}
	if (hide) {
		view_hide(florence->view);
		if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
		if (florence->obj) Accessible_unref(florence->obj);
		florence->icon=NULL;
		florence->obj=NULL;
	}
}

/* Shouldn't be used but it seems like we need to traverse accessible widgets when a new window is open to trigger
 * focus events. This is at least the case for gedit. Will need to check how this all work.
 * Can't we get a focus event when the widget is greated focussed? */
void flo_traverse (struct florence *florence, Accessible *obj)
{
	int n_children, i;
	Accessible *child;

	n_children=Accessible_getChildCount(obj);
	if (!Accessible_isTable(obj))
	{
		for (i=0;i<n_children;++i)
		{
			child=Accessible_getChildAtIndex(obj, i);
			if (Accessible_isEditableText(child) &&
				AccessibleStateSet_contains(Accessible_getStateSet(child), SPI_STATE_FOCUSED)) {
				flo_check_show(florence, child);
			} else {
				flo_traverse(florence, child);
				if (child) Accessible_unref(child);
			}
		}
	}
}

/* Called when a window is created */
void flo_window_create_event (const AccessibleEvent *event, gpointer user_data)
{
	/* For some reason, focus state change does happen after traverse 
	 * ==> did I misunderstand? */
	/* TODO: remettre le keyboard au front. Attention: always_on_screen désactive cette fonction */
	flo_traverse((struct florence *)user_data, event->source);
}
#endif

/* Switches between always on screen mode and hidden mode.
 * When in hidden mode, the spi events are registered to monitor focus and show on editable widgets.
 * the events are deregistered when always on screen mode is activated */
void flo_switch_mode (struct florence *florence, gboolean auto_hide)
{
#ifdef ENABLE_AT_SPI
	static AccessibleEventListener *focus_listener=NULL;
	static AccessibleEventListener *window_listener=NULL;
	int i;
	Accessible *obj=NULL;
#endif

	if (auto_hide) {
		if (!status_spi_is_enabled(florence->status)) {
			flo_warn(_("SPI is disabled: Unable to switch auto-hide mode on."));
			return;
		}
#ifdef ENABLE_AT_SPI
		view_hide(florence->view);
		focus_listener=SPI_createAccessibleEventListener (flo_focus_event, (void*)florence);
		SPI_registerGlobalEventListener(focus_listener, "object:state-changed:focused");
		window_listener=SPI_createAccessibleEventListener (flo_window_create_event, (void*)florence);
		SPI_registerGlobalEventListener(window_listener, "window:activate");
		for (i=1;i<=SPI_getDesktopCount();i++) {
			obj=SPI_getDesktop(i);
			if (obj) {
				flo_traverse(florence, obj);
				Accessible_unref(obj);
			}
		}
	} else {
		if (focus_listener) {
			SPI_deregisterGlobalEventListenerAll(focus_listener);
			AccessibleEventListener_unref(focus_listener);
			focus_listener=NULL;
		}
		if (window_listener) {
			SPI_deregisterGlobalEventListenerAll(window_listener);
			AccessibleEventListener_unref(window_listener);
			window_listener=NULL;
		}
		view_show(florence->view, NULL);
		if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
		if (florence->obj) Accessible_unref(obj);
		florence->obj=NULL;
		florence->icon=NULL;
#endif
	}
}

/* load the keyboards from the layout file into the keyboards member of florence */
GSList *flo_keyboards_load(struct florence *florence, struct layout *layout)
{
	GSList *keyboards=NULL;;
	struct keyboard *keyboard=NULL;
	struct keyboard_globaldata global;

	global.status=florence->status;
	florence->status->xkeyboard=xkeyboard_new();
	global.style=florence->style;

	/* read the layout file and create the extensions */
	keyboards=g_slist_append(keyboards, keyboard_new(layout, &global));
#ifdef ENABLE_XRECORD
	status_keys_add(florence->status, ((struct keyboard *)keyboards->data)->keys);
#endif
	while ((keyboard=keyboard_extension_new(layout, &global))) {
		keyboards=g_slist_append(keyboards, keyboard);
#ifdef ENABLE_XRECORD
		status_keys_add(florence->status, keyboard->keys);
#endif
	}

	xkeyboard_client_map_free(florence->status->xkeyboard);
	return keyboards;
}

/* Triggered by gconf when the "auto_hide" parameter is changed. */
void flo_set_auto_hide(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	flo_switch_mode(florence, gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* handles mouse leave events */
gboolean flo_mouse_leave_event (GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	status_focus_set(florence->status, NULL);
	status_timer_stop(florence->status);
	/* As we don't support multitouch yet, and we no longer get button events when the mouse is outside,
	 * we just release any pressed key when the mouse leaves. */
	if (status_get_moving(florence->status)) {
		gtk_window_move(GTK_WINDOW(window), (gint)((GdkEventCrossing*)event)->x_root-florence->xpos,
			(gint)((GdkEventCrossing*)event)->y_root-florence->ypos);
	} else status_pressed_set(florence->status, NULL);
#ifdef ENABLE_RAMBLE
	if (florence->ramble) ramble_reset(florence->ramble, GTK_WIDGET(florence->view->window)->window);
#endif
	return FALSE;
}

/* handles button press events */
gboolean flo_button_press_event (GtkWidget *window, GdkEventButton *event, gpointer user_data)
{
#ifdef ENABLE_RAMBLE
	gchar *val;
#endif
	struct florence *florence=(struct florence *)user_data;
	struct key *key=NULL;
	
	if (event) {
		key=status_hit_get(florence->status, (gint)((GdkEventButton*)event)->x,
#ifdef ENABLE_RAMBLE
			(gint)((GdkEventButton*)event)->y, NULL);
#else
			(gint)((GdkEventButton*)event)->y);
#endif
		/* we don't want double and triple click events */
		if ((event->type==GDK_2BUTTON_PRESS) || (event->type==GDK_3BUTTON_PRESS)) return FALSE;
	} else {
		key=status_focus_get(florence->status);
	}

#ifdef ENABLE_RAMBLE
	val=settings_get_string("behaviour/input_method");
	if (!strcmp("ramble", val)) {
		ramble_start(florence->ramble);
	} else {
#endif
	status_pressed_set(florence->status, key);
	status_timer_stop(florence->status);
#ifdef ENABLE_RAMBLE
	}
	if (val) g_free(val);
#endif
	return FALSE;
}

/* handles button release events */
gboolean flo_button_release_event (GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	status_pressed_set(florence->status, NULL);
	status_timer_stop(florence->status);
#ifdef ENABLE_RAMBLE
	if (ramble_started(florence->ramble) &&
		settings_get_bool("behaviour/ramble_button") &&
		(!strcmp("ramble", settings_get_string("behaviour/input_method")))) {
		ramble_reset(florence->ramble, GTK_WIDGET(florence->view->window)->window);
	}
#endif
	return FALSE;
}

/* update the timer representation: to be called periodically */
gboolean flo_timer_update(gpointer data)
{
	struct florence *florence=(struct florence *)data;
	gboolean ret=TRUE;
	if (status_timer_get(florence->status)>0.0 && status_focus_get(florence->status)) {
		if (status_timer_get(florence->status)>=1.0) {
			flo_button_press_event(NULL, NULL, (void *)florence);
			flo_button_release_event(NULL, NULL, (void *)florence);
			status_timer_start(florence->status, flo_timer_update, (gpointer)florence);
		}
		/* view update */
		status_focus_set(florence->status, status_focus_get(florence->status));
	} else ret=FALSE;
	return ret;
}

/* bring the window back to front: to be calles periodically */
gboolean flo_to_top(gpointer data)
{
	if (GTK_WIDGET_VISIBLE(GTK_WIDGET(data))) gtk_window_present(GTK_WINDOW(data));
	return TRUE;
}

/* handles mouse motion events 
 * update the keyboard key under the mouse */
gboolean flo_mouse_move_event(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
#ifdef ENABLE_RAMBLE
	enum key_hit hit;
	gchar *algo;
#endif
	gchar *val;
	struct florence *florence=(struct florence *)user_data;
	if (status_get_moving(florence->status)) {
		gtk_window_move(GTK_WINDOW(window), (gint)((GdkEventMotion*)event)->x_root-florence->xpos,
			(gint)((GdkEventMotion*)event)->y_root-florence->ypos);
	} else {
		/* Remember mouse position for moving */
		florence->xpos=(gint)((GdkEventMotion*)event)->x;
		florence->ypos=(gint)((GdkEventMotion*)event)->y;
		val=settings_get_string("behaviour/input_method");
#ifdef ENABLE_RAMBLE
		struct key *key=status_hit_get(florence->status, florence->xpos, florence->ypos, &hit);
		if (!strcmp("ramble", val)) {
			florence->view->ramble=florence->ramble;
			algo=settings_get_string("behaviour/ramble_algo");
			if ((hit==KEY_BORDER) &&
				(status_focus_get(florence->status)==key) &&
				(!strcmp("time", algo))) {
				ramble_time_reset(florence->ramble);
				status_focus_set(florence->status, NULL);
			}
			if (algo) g_free(algo);
			if (ramble_started(florence->ramble) &&
				ramble_add(florence->ramble, GTK_WIDGET(florence->view->window)->window,
					florence->xpos, florence->ypos, key)) {
				if (status_focus_get(florence->status)!=key) {
					status_focus_set(florence->status, key);
				}
				status_pressed_set(florence->status, key);
				status_pressed_set(florence->status, NULL);
			}
		} else
#else
		struct key *key=status_hit_get(florence->status, florence->xpos, florence->ypos);
#endif
		if (status_focus_get(florence->status)!=key) {
			if (key && settings_double_get("behaviour/timer")>0.0 &&
				(!strcmp("timer", val))) {
				status_timer_start(florence->status, flo_timer_update, (gpointer)florence);
			} else status_timer_stop(florence->status);
			status_focus_set(florence->status, key);
		}
		if (val) g_free(val);
	}
	return FALSE;
}

/* liberate memory used by the objects of the layout.
 * Those objects are the style object, the keyboards and the keys */
void flo_layout_unload(struct florence *florence)
{
	struct keyboard *keyboard;
	while (florence->keyboards) {
		keyboard=(struct keyboard *)florence->keyboards->data;
		keyboard_free(keyboard);
		florence->keyboards=g_slist_delete_link(florence->keyboards, florence->keyboards);
	}
	if (florence->style) style_free(florence->style);
}

/* loads the layout file
 * create the layour objects: the style, the keyboards and the keys */
void flo_layout_load(struct florence *florence)
{
	struct layout *layout;
	struct layout_infos *infos;
	gchar *layoutname;

	/* get the informations about the layout */
	layoutname=settings_get_string("layout/file");
	layout=layoutreader_new(layoutname,
		DATADIR "/layouts/florence.xml",
		DATADIR "/relaxng/florence.rng");
	layoutreader_element_open(layout, "layout");
	infos=layoutreader_infos_new(layout);
	flo_debug(_("Layout name: \"%s\""), infos->name);
	if (!infos->version || strcmp(infos->version, VERSION))
		flo_warn(_("Layout version %s is different from program version %s"),
			infos->version, VERSION);
	layoutreader_infos_free(infos);

	/* create the style object */
	florence->style=style_new(NULL);

	/* create the keyboard objects */
	florence->keyboards=flo_keyboards_load(florence, layout);
	layoutreader_free(layout);
	g_free(layoutname);
}

/* reloads the layout file */
void flo_layout_reload(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	status_reset(florence->status);
	flo_layout_unload(florence);
	flo_layout_load(florence);
	view_update_layout(florence->view, florence->style, florence->keyboards);
}

#ifdef ENABLE_AT_SPI
/* check if at-spi is enabled in gnome */
gboolean flo_check_at_spi(void)
{
	gboolean ret=FALSE;
	GConfClient *gconfclient=gconf_client_get_default();
	gconf_client_add_dir(gconfclient, "/desktop/gnome/interface",
		GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	if (!gconf_client_get_bool(gconfclient, "/desktop/gnome/interface/accessibility", NULL)) {
		if (GTK_RESPONSE_ACCEPT==tools_dialog(_("Enable accessibility"), NULL, 
			GTK_STOCK_OK, GTK_STOCK_CANCEL, 
			"GNOME Accessibility is disabled.\n"
			"Click OK to enable accessibility and restart GNOME.\n"
			"If you click CANCEL, auto-hide mode will be disabled.\n"
			"Alternatively, you may enable accessibility with gnome-at-properties.")) {
				gconf_client_set_bool(gconfclient,
					"/desktop/gnome/interface/accessibility", TRUE, NULL);
				system("gnome-session-save --kill");
				exit(EXIT_SUCCESS);
			ret=TRUE;
		}
		flo_error(_("at-spi registry daemon is not running. "\
		"Events will be sent with Xtest instead and several "\
		"functions are disabled, including auto-hide."));
		ret=FALSE;
	}
	return ret;
}
#endif

/* create a new instance of florence. */
#ifndef APPLET
struct florence *flo_new(gboolean gnome, const gchar *focus_back)
#else
struct florence *flo_new(gboolean gnome, const gchar *focus_back, PanelApplet *applet)
#endif
{
  struct florence *florence=(struct florence *)g_malloc(sizeof(struct florence));
	if (!florence) flo_fatal(_("Unable to allocate memory for florence"));
	memset(florence, 0, sizeof(struct florence));

#if !GLIB_CHECK_VERSION(2,14,0)
	flo_warn(_("Old GLib version detected. Florence style will be working with a hack."));
#endif

#ifdef ENABLE_RAMBLE
	florence->ramble=ramble_new();
#endif

	florence->status=status_new(focus_back);
#ifdef ENABLE_AT_SPI
	if (0!=SPI_init()) {
		if ((!gnome) || (!flo_check_at_spi())) {
			status_spi_disable(florence->status);
		}
	}
#else
	flo_warn(_("AT-SPI has been disabled at compile time: auto-hide mode is disabled."));
	status_spi_disable(florence->status);
#endif

	flo_layout_load(florence);
#ifdef APPLET
	florence->view=view_new(florence->status, florence->style, florence->keyboards, applet);
#else
	florence->view=view_new(florence->status, florence->style, florence->keyboards);
#endif
	status_view_set(florence->status, florence->view);
	if (settings_get_bool("window/keep_on_top")) g_timeout_add(FLO_TO_TOP_TIMEOUT, flo_to_top, view_window_get(florence->view));

	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "destroy", G_CALLBACK(flo_destroy), NULL);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "motion-notify-event",
		G_CALLBACK(flo_mouse_move_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "leave-notify-event",
		G_CALLBACK(flo_mouse_leave_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "button-press-event",
		G_CALLBACK(flo_button_press_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "button-release-event",
		G_CALLBACK(flo_button_release_event), florence);
#ifndef APPLET
	flo_switch_mode(florence, settings_get_bool("behaviour/auto_hide"));
	florence->trayicon=trayicon_new(florence->view, G_CALLBACK(flo_destroy));
#endif
	settings_changecb_register("behaviour/auto_hide", flo_set_auto_hide, florence);
	/* TODO: just reload the style, no need to reload the whole layout */
	settings_changecb_register("layout/style", flo_layout_reload, florence);
	settings_changecb_register("layout/file", flo_layout_reload, florence);

	return florence;
}

/* liberate all the memory used by florence */
void flo_free(struct florence *florence)
{
#ifdef ENABLE_AT_SPI
	SPI_exit();
#endif
#ifndef APPLET
	trayicon_free(florence->trayicon);
#endif
	flo_layout_unload(florence);
	if (florence->view) view_free(florence->view);
	if (florence->status) status_free(florence->status);
#ifdef ENABLE_RAMBLE
	if (florence->ramble) ramble_free(florence->ramble);
#endif
	g_free(florence);
	xmlCleanupParser();
	xmlMemoryDump();
}

