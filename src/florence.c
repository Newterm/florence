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
void flo_switch_mode (struct view *view, gboolean on_screen)
{
	static AccessibleEventListener *focus_listener=NULL;
	static AccessibleEventListener *window_listener=NULL;

	if (!on_screen) {
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

/* Callback called by the layour reader while parsing the layout file. Provides informations about the layout */
void flo_layout_infos(char *name, char *version)
{
	flo_info("Layout name: \"%s\"", name);
	if (strcmp(version, VERSION)) {
		flo_warn(_("Layout version %s is different from program version %s"), version, VERSION);
	}
}

/* load the keyboards from the layout file into the keyboards member of florence */
GSList *flo_keyboards_load(struct florence *florence, xmlTextReaderPtr layout, struct style *style)
{
	int maj = XkbMajorVersion;
	int min = XkbMinorVersion;
	int opcode_rtrn=0, event_rtrn=0, error_rtrn=0;
	GSList *keyboards=NULL;;
	struct keyboard *keyboard=NULL;
	struct keyboard_globaldata global;

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
	XkbGetState((Display *)gdk_x11_drawable_get_xdisplay(gdk_get_default_root_window()), XkbUseCoreKbd, &(global.xkb_state));
	global.status=florence->status;

	/* initialize global data */
	global.key_table=(struct key **)&(florence->keys);
	global.style=style;

	/* read the layout file and create the extensions */
	keyboards=g_slist_append(keyboards, (gpointer)keyboard_new(layout, 1, NULL, LAYOUT_VOID, &global));
	while ((keyboard=(struct keyboard *)layoutreader_readextension(layout,
		(layoutreader_keyboardprocess)keyboard_new, (void *)&global))) {
		keyboards=g_slist_append(keyboards, keyboard);
	}
	layoutreader_free(layout);

	/* Free the modifiers map */
	XkbFreeClientMap(global.xkb_desc, XkbKeyActionsMask|XkbModifierMapMask, True);

	return keyboards;
}

/* Triggered by gconf when the "always_on_screen" parameter is changed. */
void flo_set_show_on_focus(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
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

/* liberate all the memory used by florence */
void flo_free(struct florence *florence)
{
	int i;
	for (i=0;i<256;i++) {
		if (florence->keys[i]) key_free(florence->keys[i]);
	}
	if (florence->status) status_free(florence->status);
	g_free(florence);
}

/* This is the main function.
 * Creates the hitmap, the window, combine the mask and draw the keyboard
 * Registers the event callbacks.
 * Call the event loop.
 * Cleans up at exit. */
int florence (void)
{
	xmlTextReaderPtr layout;
	struct florence *florence;
	struct style *style;
	struct view *view;
	GSList *keyboards;
	struct keyboard *keyboard;
	const char *modules;

	settings_init(FALSE);

	florence=g_malloc(sizeof(struct florence));
	if (!florence) flo_fatal(_("Unable to allocate memory for florence"));
	memset(florence, 0, sizeof(struct florence));

	layout=layoutreader_new();
	layoutreader_readinfos(layout, flo_layout_infos);
	style=style_new(layout);
	florence->status=status_new(florence->keys);
	keyboards=flo_keyboards_load(florence, layout, style);

	view=view_new(style, keyboards);
	status_view_set(florence->status, view);
	g_signal_connect(G_OBJECT(view_window_get(view)), "destroy", G_CALLBACK(flo_destroy), NULL);
	g_signal_connect(G_OBJECT(view_window_get(view)), "motion-notify-event", G_CALLBACK(flo_mouse_move_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(view)), "leave-notify-event", G_CALLBACK(flo_mouse_leave_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(view)), "button-press-event", G_CALLBACK(flo_button_press_event), florence);
	g_signal_connect(G_OBJECT(view_window_get(view)), "button-release-event", G_CALLBACK(flo_button_release_event), florence);

        modules = g_getenv("GTK_MODULES");
	if (!modules||modules[0]=='\0')
		putenv("GTK_MODULES=gail:atk-bridge");
	SPI_init();
	flo_switch_mode(view, settings_get_bool("behaviour/always_on_screen"));

	trayicon_create(GTK_WIDGET(view_window_get(view)), G_CALLBACK(flo_destroy));
	settings_changecb_register("behaviour/always_on_screen", flo_set_show_on_focus, view);
	gtk_main();

	settings_exit();

	while (keyboards) {
		keyboard=(struct keyboard *)keyboards->data;
		keyboard_free(keyboard);
		keyboards=g_slist_delete_link(keyboards, keyboards);
	}
	if (view) view_free(view);
	flo_free(florence);
	SPI_exit();
	putenv("AT_BRIDGE_SHUTDOWN=1");
	return 0;
}

