/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2012 François Agrech

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
#ifdef ENABLE_AT_SPI2
#define AT_SPI
#include <dbus/dbus.h>
#include <atspi/atspi.h>
#endif
#ifdef ENABLE_AT_SPI
#define AT_SPI
#include <cspi/spi.h>
#endif

/* bring the window back to front every seconds */
#define FLO_TO_TOP_TIMEOUT 1000

/* exit signal */
static int flo_exit=FALSE;

/* Called on destroy event (systray quit or close window) */
void flo_destroy (void)
{
	START_FUNC
#ifndef APPLET
	gtk_main_quit();
#endif
	//gtk_exit (0);
	END_FUNC
}

#ifdef AT_SPI
/* Called to destroy the icon */
void flo_icon_destroy (GtkWidget *widget, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
	if (florence->obj) {
#ifdef ENABLE_AT_SPI2
		g_object_unref(florence->obj);
#else
		Accessible_unref(florence->obj);
#endif
		florence->obj=NULL;
	}
	florence->icon=NULL;
	END_FUNC
}

/* on button-press events: destroy the icon and show the actual keyboard */
void flo_icon_press (GtkWidget *window, GdkEventButton *event, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
	florence->icon=NULL;
	view_show(florence->view, florence->obj);
	if (florence->obj) {
#ifdef ENABLE_AT_SPI2
		g_object_unref(florence->obj);
#else
		Accessible_unref(florence->obj);
#endif
		florence->obj=NULL;
	}
	END_FUNC
}

/* on expose event: display florence icon */
void flo_icon_expose (GtkWidget *window, GdkEventExpose* pExpose, void *userdata)
{
	START_FUNC
	cairo_t *context, *mask_context;
	RsvgHandle *handle;
	GError *error=NULL;
	gdouble w, h;
	GdkBitmap *mask=NULL;

	context=gdk_cairo_create(window->window);
	cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);

	handle=rsvg_handle_new_from_file(ICONDIR "/florence.svg", &error);
	if (error) flo_error(_("Error loading florence icon: %s"), error->message);
	else {
		w=settings_double_get("window/scalex")*2;
		h=settings_double_get("window/scaley")*2;
		if (!(mask=(GdkBitmap*)gdk_pixmap_new(NULL, w, h, 1)))
			flo_fatal(_("Unable to create mask"));
		mask_context=gdk_cairo_create(mask);
		cairo_set_source_rgba(mask_context, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(mask_context, CAIRO_OPERATOR_SOURCE);
		cairo_paint(mask_context);
		cairo_set_operator(mask_context, CAIRO_OPERATOR_OVER);
		style_render_svg(mask_context, handle, w, h, TRUE, NULL);
		gdk_window_shape_combine_mask(window->window, mask, 0, 0);
		cairo_set_source_rgba(context, 0.0, 0.0, 0.0, 100.0);
		cairo_set_operator(context, CAIRO_OPERATOR_DEST_OUT);
		cairo_paint(context);
		cairo_set_operator(context, CAIRO_OPERATOR_OVER);
		style_render_svg(context, handle, w, h, TRUE, NULL);
		cairo_destroy(mask_context);
		g_object_unref(G_OBJECT(mask));
		rsvg_handle_free(handle);
	}

	cairo_destroy(context);
	END_FUNC
}

/* Show an intermediate icon before showing the keyboard (if intermediate_icon is activated) 
 * otherwise, directly show the keyboard */
#ifdef ENABLE_AT_SPI2
void flo_check_show (struct florence *florence, AtspiAccessible *obj)
#else
void flo_check_show (struct florence *florence, Accessible *obj)
#endif
{
	START_FUNC
	if (flo_exit || (!florence->view)) return;
	if (GTK_WIDGET_VISIBLE(florence->view->window)) view_hide(florence->view);
	if (settings_get_bool("behaviour/intermediate_icon")) {
#ifdef ENABLE_AT_SPI2
		if (florence->obj) g_object_unref(florence->obj);
#else
		if (florence->obj) Accessible_unref(florence->obj);
#endif
		florence->obj=obj;
		if (!florence->icon) {
			florence->icon=GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
			gtk_window_set_keep_above(florence->icon, TRUE);
			gtk_window_set_skip_taskbar_hint(florence->icon, TRUE);
			gtk_widget_set_size_request(GTK_WIDGET(florence->icon), settings_double_get("window/scalex")*2,
				settings_double_get("window/scaley")*2);
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
	END_FUNC
}

/* Called when a widget is focused.
 * Check if the widget is editable and show the keyboard or hide if not. */
#ifdef ENABLE_AT_SPI2
void flo_focus_event (const AtspiEvent *event, void *user_data)
#else
void flo_focus_event (const AccessibleEvent *event, void *user_data)
#endif
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	gboolean hide=FALSE;

#ifdef ENABLE_AT_SPI2
	if (atspi_accessible_get_role(event->source, NULL)==ATSPI_ROLE_TERMINAL ||
		(((atspi_accessible_get_role(event->source, NULL)==ATSPI_ROLE_TEXT) ||
		(atspi_accessible_get_role(event->source, NULL)==ATSPI_ROLE_PASSWORD_TEXT)) &&
		 atspi_state_set_contains(atspi_accessible_get_state_set(event->source), ATSPI_STATE_EDITABLE))) {
#else
	if (Accessible_getRole(event->source)==SPI_ROLE_TERMINAL || Accessible_isEditableText(event->source)) {
#endif
		if (event->detail1) {
			flo_check_show(florence, event->source);
#ifdef ENABLE_AT_SPI2
			g_object_ref(event->source);
#else
			Accessible_ref(event->source);
#endif
		} else {
			hide=TRUE;
		}
	} else {
		hide=TRUE;
	}
	if (hide) {
		view_hide(florence->view);
		if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
#ifdef ENABLE_AT_SPI2
		if (florence->obj) g_object_unref(florence->obj);
#else
		if (florence->obj) Accessible_unref(florence->obj);
#endif
		florence->icon=NULL;
		florence->obj=NULL;
	}
	END_FUNC
}
#endif

#ifdef AT_SPI
/* Shouldn't be used but it seems like we need to traverse accessible widgets when a new window is open to trigger
 * focus events. This is at least the case for gedit. Will need to check how this all work.
 * Can't we get a focus event when the widget is greated focussed? */
#ifdef ENABLE_AT_SPI2
void flo_traverse (struct florence *florence, AtspiAccessible *obj)
#else
void flo_traverse (struct florence *florence, Accessible *obj)
#endif
{
	START_FUNC
	int n_children, i;
#ifdef ENABLE_AT_SPI2
	AtspiAccessible *child;
	n_children=atspi_accessible_get_child_count(obj, NULL);

	if (!atspi_accessible_get_table(obj)) {
#else
	Accessible *child;
	n_children=Accessible_getChildCount(obj);

	if (!Accessible_isTable(obj)) {
#endif
		for (i=0;i<n_children;++i)
		{
#ifdef ENABLE_AT_SPI2
			child=atspi_accessible_get_child_at_index(obj, i, NULL);
			if (atspi_state_set_contains(atspi_accessible_get_state_set(child), ATSPI_STATE_FOCUSED) &&
				(atspi_accessible_get_role(child, NULL)==ATSPI_ROLE_TERMINAL ||
				(((atspi_accessible_get_role(child, NULL)==ATSPI_ROLE_TEXT) ||
				(atspi_accessible_get_role(child, NULL)==ATSPI_ROLE_PASSWORD_TEXT)) &&
				 atspi_state_set_contains(atspi_accessible_get_state_set(child), ATSPI_STATE_EDITABLE)))) {
#else
			child=Accessible_getChildAtIndex(obj, i);
			if (Accessible_isEditableText(child) &&
				AccessibleStateSet_contains(Accessible_getStateSet(child), SPI_STATE_FOCUSED)) {
#endif
				flo_check_show(florence, child);
			} else {
				flo_traverse(florence, child);
#ifdef ENABLE_AT_SPI2
				if (child) g_object_unref(child);
#else
				if (child) Accessible_unref(child);
#endif
			}
		}
	}
	END_FUNC
}

/* Called when a window is created */
#ifdef ENABLE_AT_SPI2
void flo_window_create_event (const AtspiEvent *event, gpointer user_data)
#else
void flo_window_create_event (const AccessibleEvent *event, gpointer user_data)
#endif
{
	START_FUNC
	/* For some reason, focus state change does happen after traverse 
	 * ==> did I misunderstand? */
	/* TODO: remettre le keyboard au front. Attention: always_on_screen désactive cette fonction */
	flo_traverse((struct florence *)user_data, event->source);
	END_FUNC
}
#endif

/* Switches between always on screen mode and hidden mode.
 * When in hidden mode, the spi events are registered to monitor focus and show on editable widgets.
 * the events are deregistered when always on screen mode is activated */
void flo_switch_mode (struct florence *florence, gboolean auto_hide)
{
	START_FUNC
#ifdef ENABLE_AT_SPI
	static AccessibleEventListener *focus_listener=NULL;
	static AccessibleEventListener *window_listener=NULL;
	Accessible *obj=NULL;
#endif
#ifdef AT_SPI
	int i;
#endif

	if (auto_hide) {
		if (!status_spi_is_enabled(florence->status)) {
			flo_warn(_("SPI is disabled: Unable to switch auto-hide mode on."));
			return;
		}
#ifdef AT_SPI
		view_hide(florence->view);
#ifdef ENABLE_AT_SPI2
		atspi_event_listener_register_from_callback(flo_focus_event, (void*)florence, NULL, "object:state-changed:focused", NULL);
		atspi_event_listener_register_from_callback(flo_window_create_event, (void*)florence, NULL, "window:activate", NULL);
#else
		focus_listener=SPI_createAccessibleEventListener(flo_focus_event, (void*)florence);
		SPI_registerGlobalEventListener(focus_listener, "object:state-changed:focused");
		window_listener=SPI_createAccessibleEventListener(flo_window_create_event, (void*)florence);
		SPI_registerGlobalEventListener(window_listener, "window:activate");
		for (i=1;i<=SPI_getDesktopCount();i++) {
			obj=SPI_getDesktop(i);
			if (obj) {
				flo_traverse(florence, obj);
				Accessible_unref(obj);
			}
		}
#endif
	} else {
#ifdef ENABLE_AT_SPI2
		if (!atspi_event_listener_deregister_from_callback(flo_focus_event, (void*)florence, "object:state-changed:focused", NULL)) {
			flo_warn(_("AT SPI: problem deregistering focus listener"));
		}
		if (!atspi_event_listener_deregister_from_callback(flo_window_create_event, (void*)florence, "window:activate", NULL)) {
			flo_warn(_("AT SPI: problem deregistering window listener"));
		}
#endif
#ifdef ENABLE_AT_SPI
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
#endif
		view_show(florence->view, NULL);
		if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
#ifdef ENABLE_AT_SPI
		if (florence->obj) Accessible_unref(obj);
#endif
		florence->obj=NULL;
		florence->icon=NULL;
#endif
	}
	END_FUNC
}

/* load the keyboards from the layout file into the keyboards member of florence */
GSList *flo_keyboards_load(struct florence *florence, struct layout *layout)
{
	START_FUNC
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
	END_FUNC
	return keyboards;
}

/* Triggered by gconf when the "auto_hide" parameter is changed. */
void flo_set_auto_hide(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	flo_switch_mode(florence, gconf_value_get_bool(gconf_entry_get_value(entry)));
	END_FUNC
}

/* handles mouse leave events */
gboolean flo_mouse_leave_event (GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	status_focus_set(florence->status, NULL);
	status_timer_stop(florence->status);
	/* As we don't support multitouch yet, and we no longer get button events when the mouse is outside,
	 * we just release any pressed key when the mouse leaves. */
	if (status_get_moving(florence->status)) {
		gtk_window_move(GTK_WINDOW(window), (gint)((GdkEventCrossing*)event)->x_root-florence->xpos,
			(gint)((GdkEventCrossing*)event)->y_root-florence->ypos);
	} else {
		status_pressed_set(florence->status, NULL);
		status_press_latched(florence->status, NULL);
	}
#ifdef ENABLE_RAMBLE
	if (florence->ramble) ramble_reset(florence->ramble, GTK_WIDGET(florence->view->window)->window);
#endif
	END_FUNC
	return FALSE;
}

/* handles mouse enter events */
gboolean flo_mouse_enter_event (GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	status_release_latched(florence->status, NULL);
	END_FUNC
	return FALSE;
}

/* handles button press events */
gboolean flo_button_press_event (GtkWidget *window, GdkEventButton *event, gpointer user_data)
{
	START_FUNC
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
	if (status_im_get(florence->status)==STATUS_IM_RAMBLE) {
		ramble_start(florence->ramble);
	} else {
#endif
	status_pressed_set(florence->status, key);
	status_timer_stop(florence->status);
#ifdef ENABLE_RAMBLE
	}
#endif
	END_FUNC
	return FALSE;
}

/* handles button release events */
gboolean flo_button_release_event (GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	status_pressed_set(florence->status, NULL);
	status_timer_stop(florence->status);
#ifdef ENABLE_RAMBLE
	if (ramble_started(florence->ramble) &&
		status_im_get(florence->status)==STATUS_IM_RAMBLE &&
		settings_get_bool("behaviour/ramble_button")) {
		ramble_reset(florence->ramble, GTK_WIDGET(florence->view->window)->window);
	}
#endif
	END_FUNC
	return FALSE;
}

/* update the timer representation: to be called periodically */
gboolean flo_timer_update(gpointer data)
{
	START_FUNC
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
	END_FUNC
	return ret;
}

/* bring the window back to front: to be calles periodically */
gboolean flo_to_top(gpointer data)
{
	START_FUNC
	struct florence *florence=data;
	GtkWindow *window=GTK_WINDOW(view_window_get(florence->view));
	if (!settings_get_bool("window/keep_on_top")) return FALSE;
	if (GTK_WIDGET_VISIBLE(GTK_WIDGET(window))) gtk_window_present(window);
	END_FUNC
	return TRUE;
}
/* start keeping the keyboard back to front every second */
void flo_start_keep_on_top(struct florence *florence, gboolean keep_on_top)
{
	START_FUNC
	if (settings_get_bool("window/keep_on_top")) {
		g_timeout_add(FLO_TO_TOP_TIMEOUT, flo_to_top, florence);
	}
	END_FUNC
}

/* Triggered by gconf when the "keep_on_top" parameter is changed. */
void flo_set_keep_on_top(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	START_FUNC
	struct florence *florence=user_data;
	flo_start_keep_on_top(florence, gconf_value_get_bool(gconf_entry_get_value(entry)));
	END_FUNC
}

/* handles mouse motion events 
 * update the keyboard key under the mouse */
gboolean flo_mouse_move_event(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	START_FUNC
#ifdef ENABLE_RAMBLE
	enum key_hit hit;
	gchar *algo;
#endif
	struct florence *florence=(struct florence *)user_data;
	if (status_get_moving(florence->status)) {
		gtk_window_move(GTK_WINDOW(window), (gint)((GdkEventMotion*)event)->x_root-florence->xpos,
			(gint)((GdkEventMotion*)event)->y_root-florence->ypos);
	} else {
		/* Remember mouse position for moving */
		florence->xpos=(gint)((GdkEventMotion*)event)->x;
		florence->ypos=(gint)((GdkEventMotion*)event)->y;
#ifdef ENABLE_RAMBLE
		struct key *key=status_hit_get(florence->status, florence->xpos, florence->ypos, &hit);
		if (status_im_get(florence->status)==STATUS_IM_RAMBLE) {
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
				status_im_get(florence->status)==STATUS_IM_TIMER) {
				status_timer_start(florence->status, flo_timer_update, (gpointer)florence);
			} else status_timer_stop(florence->status);
			status_focus_set(florence->status, key);
		}
	}
	END_FUNC
	return FALSE;
}

/* liberate memory used by the objects of the layout.
 * Those objects are the style object, the keyboards and the keys */
void flo_layout_unload(struct florence *florence)
{
	START_FUNC
	struct keyboard *keyboard;
	while (florence->keyboards) {
		keyboard=(struct keyboard *)florence->keyboards->data;
		keyboard_free(keyboard);
		florence->keyboards=g_slist_delete_link(florence->keyboards, florence->keyboards);
	}
	if (florence->style) style_free(florence->style);
	END_FUNC
}

/* loads the layout file
 * create the layour objects: the style, the keyboards and the keys */
void flo_layout_load(struct florence *florence)
{
	START_FUNC
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
	flo_debug(TRACE_DEBUG, _("Layout name: \"%s\""), infos->name);
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
	END_FUNC
}

/* reloads the layout file */
void flo_layout_reload(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	START_FUNC
	struct florence *florence=(struct florence *)user_data;
	status_reset(florence->status);
	flo_layout_unload(florence);
	flo_layout_load(florence);
	view_update_layout(florence->view, florence->style, florence->keyboards);
	END_FUNC
}

#ifdef ENABLE_AT_SPI
/* check if at-spi is enabled in gnome */
gboolean flo_check_at_spi(void)
{
	START_FUNC
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
	END_FUNC
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
	START_FUNC
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
#ifdef AT_SPI
#ifdef ENABLE_AT_SPI2
	if (atspi_init()) {
#else
	if (SPI_init() && ((!gnome) || (!flo_check_at_spi()))) {
#endif
			status_spi_disable(florence->status);
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
	flo_start_keep_on_top(florence, settings_get_bool("window/keep_on_top"));

	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "destroy", G_CALLBACK(flo_destroy), NULL);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "motion-notify-event",
		G_CALLBACK(flo_mouse_move_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "leave-notify-event",
		G_CALLBACK(flo_mouse_leave_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "enter-notify-event",
		G_CALLBACK(flo_mouse_enter_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "button-press-event",
		G_CALLBACK(flo_button_press_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "button-release-event",
		G_CALLBACK(flo_button_release_event), florence);
#ifndef APPLET
	if (settings_get_bool("behaviour/hide_on_start") && (!settings_get_bool("behaviour/auto_hide"))) view_hide(florence->view);
	else flo_switch_mode(florence, settings_get_bool("behaviour/auto_hide"));
	florence->trayicon=trayicon_new(florence->view, G_CALLBACK(flo_destroy));
#endif
	settings_changecb_register("behaviour/auto_hide", flo_set_auto_hide, florence);
	settings_changecb_register("window/keep_on_top", flo_set_keep_on_top, florence);
	/* TODO: just reload the style, no need to reload the whole layout */
	settings_changecb_register("layout/style", flo_layout_reload, florence);
	settings_changecb_register("layout/file", flo_layout_reload, florence);

	END_FUNC
	return florence;
}

/* liberate all the memory used by florence */
void flo_free(struct florence *florence)
{
	START_FUNC
	flo_exit=TRUE;
#ifdef AT_SPI
#ifdef ENABLE_AT_SPI2
	atspi_exit();
#else
	SPI_exit();
#endif
#endif
#ifndef APPLET
	trayicon_free(florence->trayicon);
	florence->trayicon=NULL;
#endif
	flo_layout_unload(florence);
	if (florence->view) view_free(florence->view);
	florence->view=NULL;
	if (florence->status) status_free(florence->status);
	florence->status=NULL;
#ifdef ENABLE_RAMBLE
	if (florence->ramble) ramble_free(florence->ramble);
	florence->ramble=NULL;
#endif
	g_free(florence);
	xmlCleanupParser();
	xmlMemoryDump();
	END_FUNC
}

