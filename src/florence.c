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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cspi/spi.h>

/* Called on destroy event (systray quit or close window) */
void flo_destroy (void)
{
	gtk_exit (0);
}

/* Called when a widget is focused.
 * Check if the widget is editable and show the keyboard or hide if not. */
void flo_focus_event (const AccessibleEvent *event, void *user_data)
{
	struct view *view=(struct view *)user_data;

	if (Accessible_getRole(event->source)==SPI_ROLE_TERMINAL || Accessible_isEditableText(event->source)) {
		if (event->detail1) {
			view_show(view, event->source);
		}
		else {
			view_hide(view);
		}
	}
}

/* Shouldn't be used but it seems like we need to traverse accessible widgets when a new window is open to trigger
 * focus events. This is at least the case for gedit. Will need to check how this all work.
 * Can't we get a focus event when the widget is greated focussed? */
void flo_traverse (struct view *view, Accessible *obj)
{
	int n_children, i;
	Accessible *child;

	n_children=Accessible_getChildCount (obj);
	if (!Accessible_isTable(obj))
	{
		for (i=0;i<n_children;++i)
		{
			child=Accessible_getChildAtIndex(obj, i);
			if (Accessible_isEditableText(child) &&
				AccessibleStateSet_contains(Accessible_getStateSet(child), SPI_STATE_FOCUSED))
				view_show(view, child);
			else flo_traverse(view, child);
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
	flo_traverse((struct view *)user_data, event->source);
}

/* Switches between always on screen mode and hidden mode.
 * When in hidden mode, the spi events are registered to monitor focus and show on editable widgets.
 * the events are deregistered when always on screen mode is activated */
void flo_switch_mode (struct view *view, gboolean auto_hide)
{
	static AccessibleEventListener *focus_listener=NULL;
	static AccessibleEventListener *window_listener=NULL;

	if (auto_hide) {
		view_hide(view);
		focus_listener=SPI_createAccessibleEventListener (flo_focus_event, (void*)view);
		SPI_registerGlobalEventListener(focus_listener, "object:state-changed:focused");
		window_listener=SPI_createAccessibleEventListener (flo_window_create_event, (void*)view);
		SPI_registerGlobalEventListener(window_listener, "window:activate");
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
		view_show(view, NULL);
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
	global.key_table=(struct key **)&(florence->keys);
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
	struct view *view=(struct view *)user_data;
	flo_switch_mode(view, gconf_value_get_bool(gconf_entry_get_value(entry)));
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
	guint code=status_keycode_get(florence->status, (gint)((GdkEventMotion*)event)->x, (gint)((GdkEventMotion*)event)->y);
	if (status_focus_get(florence->status)!=florence->keys[code]) {
		if (florence->keys[code] && settings_get_double("behaviour/auto_click")>0.0) {
			status_timer_start(florence->status, flo_timer_update, (gpointer)florence);
		} else status_timer_stop(florence->status);
		status_focus_set(florence->status, florence->keys[code]);
	}
	return FALSE;
}

/* liberate memory used by the objects of the layout.
 * Those objects are the style object, the keyboards and the keys */
void flo_layout_unload(struct florence *florence)
{
	int i;
	struct keyboard *keyboard;
	while (florence->keyboards) {
		keyboard=(struct keyboard *)florence->keyboards->data;
		keyboard_free(keyboard);
		florence->keyboards=g_slist_delete_link(florence->keyboards, florence->keyboards);
	}
	if (florence->style) style_free(florence->style);
	for (i=0;i<256;i++) {
		if (florence->keys[i]) key_free(florence->keys[i]);
	}
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

	flo_switch_mode(florence->view, settings_get_bool("behaviour/auto_hide"));
	florence->trayicon=trayicon_new(GTK_WIDGET(view_window_get(florence->view)), G_CALLBACK(flo_destroy));

	settings_changecb_register("behaviour/auto_hide", flo_set_auto_hide, florence->view);
	/* TODO: just reload the style, no need to reload the whole layout */
	settings_changecb_register("layout/style", flo_layout_reload, florence);

	SPI_init();

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

