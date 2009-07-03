/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009 François Agrech

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
#include "status.h"
#include "settings.h"
#ifdef ENABLE_XTST
#include <X11/Xproto.h>
#include <gdk/gdkx.h>
#include <X11/extensions/XTest.h>
#endif

/* Calculate single key status after key is pressed */
void status_key_press_update(struct status *status, struct key *key)
{
	GList *list=status_pressedkeys_get(status);
	struct key *pressed;
	gboolean redrawall=FALSE;
	if (key_get_modifier(key)) {
		key_set_pressed(key, !key->pressed);
	} else { 
		key_set_pressed(key, TRUE);
		while (list) {
			pressed=((struct key *)list->data);
			list=list->next;
			if (key_get_modifier(pressed) && !key_is_locker(pressed)) {
				key_set_pressed(pressed, FALSE);
				status_release(status, pressed);
				redrawall=TRUE;
			}
		}
	}
	if (key_is_pressed(key)) status_press(status, key);
	else status_release(status, key);
	if (!redrawall) redrawall=key_get_modifier(key) || status->globalmod;
	view_update(status->view, key, redrawall);
}

/* Calculate single key status after key is released */
void status_key_release_update(struct status *status, struct key *key)
{
	if (!key_get_modifier(key)) {
		key_set_pressed(key, FALSE);
		status_release(status, key);
		view_update(status->view, key, FALSE);
	}
}

#ifdef ENABLE_XTST
/* Called when a record event is received from XRecord */
void status_record_event (XPointer priv, XRecordInterceptData *hook)
{
	xEvent *event;
	struct status *status=(struct status *)priv;
	struct key *key;
	if (hook->category==XRecordFromServer) {
		event=(xEvent *)hook->data;
		if ((key=status->keys[event->u.u.detail])) {
			if (event->u.u.type==KeyPress) {
				status_key_press_update(status, key);
			} else if (event->u.u.type==KeyRelease) {
				status_key_release_update(status, key);
			}
		}
	}
	if (hook) XRecordFreeData(hook);
}

/* Process record events when idle */
gboolean status_record_process (gpointer data)
{
	struct status *status=(struct status *)data;
	XRecordProcessReplies(status->data_disp);
	return TRUE;
}

/* Record keyboard events */
gpointer status_record_start (gpointer data)
{
	struct status *status=(struct status *)data;
	int major, minor;
	XRecordRange *range;
	XRecordClientSpec client;
	Display *ctrl_disp=(Display *)gdk_x11_drawable_get_xdisplay(gdk_get_default_root_window());

	status->data_disp=XOpenDisplay(NULL);
	if (XRecordQueryVersion(ctrl_disp, &major, &minor)) {
		flo_info(_("XRecord extension found version=%d.%d"), major, minor);
		if (!(range=XRecordAllocRange())) flo_fatal(_("Unable to allocate memory for record range"));
		memset(range, 0, sizeof(XRecordRange));
		range->device_events.first=KeyPress;
		range->device_events.last=KeyRelease;
		range->delivered_events.first=KeyPress;
		range->delivered_events.last=KeyRelease;
		client=XRecordAllClients;
		if ((status->RecordContext=XRecordCreateContext(ctrl_disp, 0, &client, 1, &range, 1))) {
			XSync(ctrl_disp, TRUE);
			if (!XRecordEnableContextAsync(status->data_disp, status->RecordContext, status_record_event,
				(XPointer)status))
				flo_error(_("Unable to record events"));
		} else flo_warn(_("Unable to create xrecord context"));
		XFree(range);
	}
	else flo_warn(_("No XRecord extension found"));
	if (!status->RecordContext) flo_warn(_("Keyboard synchronization is disabled."));
	return data;
}

/* Stop recording keyboard events */
void status_record_stop (struct status *status)
{
	if (status->RecordContext) {
		XRecordDisableContext(status->data_disp, status->RecordContext);
		XRecordFreeContext(status->data_disp, status->RecordContext);
		status->RecordContext=0;
	}
	if (status->data_disp) {
		/* TODO: investigate why this is blocking */
		/* XCloseDisplay(status->data_disp); */
		status->data_disp=NULL;
	}
}

/* Add keys to the keycode indexed keys */
void status_keys_add(struct status *status, GSList *keys)
{
	GSList *list=keys;
	struct key *key;
	while (list) {
		key=(struct key *)list->data;
		status->keys[key->code]=key;
		list=list->next;
	}
}
#endif

/* update the focus key */
void status_focus_set(struct status *status, struct key *focus)
{
	view_update(status->view, status->focus, FALSE);
	status->focus=focus;
	view_update(status->view, status->focus, FALSE);
}

/* return the focus key */
struct key *status_focus_get(struct status *status)
{
	return status->focus;
}

/* update the pressed key */
void status_pressed_set(struct status *status, struct key *pressed)
{
	if (pressed) {
		status->pressed=pressed;
		if (key_is_pressed(status->pressed) && (!key_get_modifier(status->pressed)))
			key_release(status->pressed, status);
		if ((!key_get_modifier(status->pressed)) || key_is_locker(status->pressed))
			key_press(status->pressed, status);
#ifdef ENABLE_XTST
		else status_key_press_update(status, status->pressed); 
		if (!status->RecordContext) status_key_press_update(status, status->pressed);
#else
		status_key_press_update(status, status->pressed);
#endif
	} else {
		if (status->pressed) {
			if ((!key_get_modifier(status->pressed)) || key_is_locker(status->pressed))
				key_release(status->pressed, status);
#ifdef ENABLE_XTST
			else status_key_release_update(status, status->pressed);
			if (!status->RecordContext) status_key_release_update(status, status->pressed);
#else
			status_key_release_update(status, status->pressed);
#endif
			status->pressed=NULL;
		}
	}
}

/* returns the key currently focussed */
struct key *status_hit_get(struct status *status, gint x, gint y)
{
	return view_hit_get(status->view, x, y);
}

/* start the timer */
void status_timer_start(struct status *status, GSourceFunc update, gpointer data)
{
	if (status->timer) g_timer_start(status->timer);
	else {
		status->timer=g_timer_new();
		g_idle_add(update, data);
	}
}

/* stop the timer */
void status_timer_stop(struct status *status)
{
	if (status->timer) {
		g_timer_destroy(status->timer);
		status->timer=NULL;
	}
}

/* get timer value */
gdouble status_timer_get(struct status *status)
{
	gdouble ret=0.0;
	if (status->timer)
		ret=g_timer_elapsed(status->timer, NULL)*1000./settings_get_double("behaviour/auto_click");
	return ret;
}

/* update the global modifier mask */
void status_globalmod_set(struct status *status, GdkModifierType mod)
{
	status->globalmod|=mod;
}

/* update the global modifier mask */
void status_globalmod_unset(struct status *status, GdkModifierType mod)
{
	status->globalmod&=~mod;
}

/* add a key pressed to the list of pressed keys */
void status_press(struct status *status, struct key *key)
{
	if (!g_list_find(status->pressedkeys, key))
		status->pressedkeys=g_list_prepend(status->pressedkeys, key);
	if (key_get_modifier(key) && key_is_pressed(key)) status_globalmod_set(status, key_get_modifier(key));
}

/* remove a key pressed from the list of pressed keys */
void status_release(struct status *status, struct key *key)
{
	GList *found;
	if ((found=g_list_find(status->pressedkeys, key)))
		status->pressedkeys=g_list_delete_link(status->pressedkeys, found);
	if (key_get_modifier(key) && (!key_is_pressed(key))) status_globalmod_unset(status, key_get_modifier(key));
}

/* get the list of pressed keys */
GList *status_pressedkeys_get(struct status *status)
{
	return status->pressedkeys;
}

/* get the global modifier mask */
GdkModifierType status_globalmod_get(struct status *status)
{
	return status->globalmod;
}

/* allocate memory for status */
struct status *status_new()
{
	struct status *status=g_malloc(sizeof(struct status));
	if (!status) flo_fatal(_("Unable to allocate memory for status"));
	memset(status, 0, sizeof(struct status));
#ifdef ENABLE_XTST
	status_record_start(status);
	g_idle_add(status_record_process, (gpointer)status);
#endif
	status->spi=TRUE;
	return status;
}

/* liberate status memory */
void status_free(struct status *status)
{
#ifdef ENABLE_XTST
	status_record_stop(status);
#endif
	if (status->timer) g_timer_destroy(status->timer);
	if (status->pressedkeys) g_list_free(status->pressedkeys);
	if (status) g_free(status);
}

/* reset the status to its original state */
void status_reset(struct status *status)
{
	status->focus=NULL;
	status->pressed=NULL;
	if (status->timer) g_timer_destroy(status->timer);
	status->timer=NULL;
	if (status->pressedkeys) g_list_free(status->pressedkeys);
	status->pressedkeys=NULL;
	status->globalmod=0;
}

/* sets the view to update on status change */
void status_view_set(struct status *status, struct view *view)
{
	status->view=view;
	view_status_set(view, status);
}

/* disable sending of spi events: send xtest events instead */
void status_spi_disable(struct status *status)
{
#ifdef ENABLE_XTST
	int event_base, error_base, major, minor;
	if (!XTestQueryExtension(
		(Display *)gdk_x11_drawable_get_xdisplay(gdk_get_default_root_window()),
		&event_base, &error_base, &major, &minor)) {
		flo_error(_("Neither at-spi nor XTest could be initialized."));
		flo_fatal(_("There is no way we can send keyboard events."));
	} else flo_info(_("XTest extension found: version=%d.%d"), major, minor);
#else
	flo_error(_("Xtest extension not compiled in and at-spi not working"));
	flo_fatal(_("There is no way we can send keyboard events."));
#endif
	status->spi=FALSE;
}

/* tell if spi is enabled */
gboolean status_spi_is_enabled(struct status *status) { return status->spi; }

