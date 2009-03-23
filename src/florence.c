/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008, 2009 François Agrech

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
#include "settings.h"
#include "layoutreader.h"
#include "keyboard.h"
#include "tools.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cspi/spi.h>

/* Called on destroy event (systray quit or close window) */
void flo_destroy (void)
{
	gtk_exit (0);
}

/* Calles to destroy the icon */
void flo_icon_destroy (GtkWidget *widget, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
	if (florence->obj) Accessible_unref(florence->obj);
	florence->icon=NULL;
}

/* on button-press events: destroy the icon and show the actual keyboard */
void flo_icon_press (GtkWidget *window, GdkEventButton *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	if (florence->icon) gtk_object_destroy(GTK_OBJECT(florence->icon));
	florence->icon=NULL;
	view_show(florence->view, florence->obj);
	Accessible_unref(florence->obj);
	florence->obj=NULL;
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
		style_render_svg(context, handle, settings_get_double("window/zoom")*2,
			settings_get_double("window/zoom")*2, TRUE, NULL);
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
			gtk_widget_set_size_request(GTK_WIDGET(florence->icon), settings_get_double("window/zoom")*2,
				settings_get_double("window/zoom")*2);
			gtk_container_set_border_width(GTK_CONTAINER(florence->icon), 0);
			gtk_window_set_decorated(florence->icon, FALSE);
			gtk_window_set_position(florence->icon, GTK_WIN_POS_MOUSE);
			gtk_window_set_accept_focus(florence->icon, FALSE);
			gtk_widget_set_events(GTK_WIDGET(florence->icon), GDK_ALL_EVENTS_MASK);
			g_signal_connect(G_OBJECT(florence->icon), "expose-event", G_CALLBACK(flo_icon_expose), NULL);
			g_signal_connect(G_OBJECT(florence->icon), "button-press-event",
				G_CALLBACK(flo_icon_press), florence);
			g_signal_connect(G_OBJECT(florence->view->window), "show",
				G_CALLBACK(flo_icon_destroy), florence);
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
				Accessible_unref(child);
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

/* Switches between always on screen mode and hidden mode.
 * When in hidden mode, the spi events are registered to monitor focus and show on editable widgets.
 * the events are deregistered when always on screen mode is activated */
void flo_switch_mode (struct florence *florence, gboolean auto_hide)
{
	static AccessibleEventListener *focus_listener=NULL;
	static AccessibleEventListener *window_listener=NULL;
	int i;
	Accessible *obj;

	if (auto_hide) {
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
	}
}

/* load the keyboards from the layout file into the keyboards member of florence */
GSList *flo_keyboards_load(struct florence *florence, struct layout *layout)
{
	int maj = XkbMajorVersion;
	int min = XkbMinorVersion;
	int opcode_rtrn=0, event_rtrn=0, error_rtrn=0;
	GSList *keyboards=NULL;;
	struct keyboard *keyboard=NULL;
	struct keyboard_globaldata global;
	struct layout_extension *extension=NULL;

	/* Check XKB Version */
	if (!XkbLibraryVersion(&maj, &min) ||
		!XkbQueryExtension((Display *)gdk_x11_drawable_get_xdisplay(gdk_get_default_root_window()),
		&opcode_rtrn, &event_rtrn, &error_rtrn, &maj, &min)) {
		flo_fatal(_("XKB version mismatch"));
	}
	/* get the modifier map from xkb */
	global.xkb_desc=XkbGetMap((Display *)gdk_x11_drawable_get_xdisplay(gdk_get_default_root_window()),
	XkbKeyActionsMask|XkbModifierMapMask, XkbUseCoreKbd);
	/* get global modifiers state */
	XkbGetState((Display *)gdk_x11_drawable_get_xdisplay(gdk_get_default_root_window()),
		XkbUseCoreKbd, &(global.xkb_state));
	global.status=florence->status;

	/* initialize global data */
	global.style=florence->style;

	/* read the layout file and create the extensions */
	keyboards=g_slist_append(keyboards,
		       keyboard_new(layout, florence->style, NULL, NULL, LAYOUT_VOID, &global));
	while ((extension=layoutreader_extension_new(layout))) {
		flo_debug(_("[new extension] name=%s id=%s"), extension->name, extension->identifiant);
		keyboard=keyboard_new(layout, florence->style, extension->identifiant, extension->name,
			extension->placement, &global);
		keyboards=g_slist_append(keyboards, keyboard);
		layoutreader_extension_free(layout, extension);
	}

	/* Free the modifiers map */
	XkbFreeClientMap(global.xkb_desc, XkbKeyActionsMask|XkbModifierMapMask, True);

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
	status_pressed_set(florence->status, FALSE);
	return FALSE;
}

/* handles button press events */
gboolean flo_button_press_event (GtkWidget *window, GdkEventButton *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	/* we don't want double and triple click events */
	if (event && ((event->type==GDK_2BUTTON_PRESS) || (event->type==GDK_3BUTTON_PRESS))) return FALSE;

	/* means 2 consecutive button press and no release, but we don't support multi-touch, yet. */
	/* so we just release any pressed key */
	status_pressed_set(florence->status, FALSE);
	status_pressed_set(florence->status, TRUE);
	status_timer_stop(florence->status);
	return FALSE;
}

/* handles button release events */
gboolean flo_button_release_event (GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	status_pressed_set(florence->status, FALSE);
	status_timer_stop(florence->status);
	return FALSE;
}

/* update the timer representation: to be called when idle */
gboolean flo_timer_update(gpointer data)
{
	struct florence *florence=(struct florence *)data;
	if (status_timer_get(florence->status)>0.0 && status_focus_get(florence->status)) {
		if (status_timer_get(florence->status)>=1.0) {
			flo_button_press_event(NULL, NULL, (void *)florence);
			flo_button_release_event(NULL, NULL, (void *)florence);
		}
		/* view update */
		status_focus_set(florence->status, status_focus_get(florence->status));
		return TRUE;
	} else return FALSE;
}

/* handles mouse motion events 
 * update the keyboard key under the mouse */
gboolean flo_mouse_move_event(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	struct florence *florence=(struct florence *)user_data;
	struct key *key=status_hit_get(florence->status, (gint)((GdkEventMotion*)event)->x,
		(gint)((GdkEventMotion*)event)->y);
	if (status_focus_get(florence->status)!=key) {
		if (key && settings_get_double("behaviour/auto_click")>0.0) {
			status_timer_start(florence->status, flo_timer_update, (gpointer)florence);
		} else status_timer_stop(florence->status);
		status_focus_set(florence->status, key);
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

	/* get the informations about the layout */
	layout=layoutreader_new(settings_get_string("layout/file"),
		DATADIR "/florence.xml",
		DATADIR "/relaxng/florence.rng");
	layoutreader_element_open(layout, "layout");
	infos=layoutreader_infos_new(layout);
	flo_info(_("Layout name: \"%s\""), infos->name);
	if (!infos->version || strcmp(infos->version, VERSION))
		flo_warn(_("Layout version %s is different from program version %s"),
			infos->version, VERSION);
	layoutreader_infos_free(infos);

	/* create the style object */
	florence->style=style_new(NULL);

	/* create the keyboard objects */
	florence->keyboards=flo_keyboards_load(florence, layout);
	layoutreader_free(layout);
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

/* create a new instance of florence. */
struct florence *flo_new(void)
{
	struct florence *florence;

	florence=g_malloc(sizeof(struct florence));
	if (!florence) flo_fatal(_("Unable to allocate memory for florence"));
	memset(florence, 0, sizeof(struct florence));

	florence->status=status_new();
	flo_layout_load(florence);
	florence->view=view_new(florence->style, florence->keyboards);
	status_view_set(florence->status, florence->view);

	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "destroy", G_CALLBACK(flo_destroy), NULL);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "motion-notify-event",
		G_CALLBACK(flo_mouse_move_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "leave-notify-event",
		G_CALLBACK(flo_mouse_leave_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "button-press-event",
		G_CALLBACK(flo_button_press_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(florence->view)), "button-release-event",
		G_CALLBACK(flo_button_release_event), florence);

	SPI_init();
	flo_switch_mode(florence, settings_get_bool("behaviour/auto_hide"));
	florence->trayicon=trayicon_new(GTK_WIDGET(view_window_get(florence->view)), G_CALLBACK(flo_destroy));

	settings_changecb_register("behaviour/auto_hide", flo_set_auto_hide, florence);
	/* TODO: just reload the style, no need to reload the whole layout */
	settings_changecb_register("layout/style", flo_layout_reload, florence);

	return florence;
}

/* liberate all the memory used by florence */
void flo_free(struct florence *florence)
{
	SPI_exit();
	trayicon_free(florence->trayicon);
	flo_layout_unload(florence);
	if (florence->view) view_free(florence->view);
	if (florence->status) status_free(florence->status);
	g_free(florence);
}

