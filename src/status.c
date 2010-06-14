/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009, 2010 François Agrech

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

#include "trace.h"
#include "status.h"
#include "settings.h"
#include <X11/Xproto.h>

/* check for record events every 1/10th of a second */
#define STATUS_EVENTCHECK_INTERVAL 100
/* animate keyboard every 1/50th of a second */
#define STATUS_ANIMATION_INTERVAL 20

/* action for the fsm table */
typedef void (*status_action) (struct status *, struct key *, enum status_event);

/* what to do when state changes */
struct status_change {
	enum key_state new_state; /* new state after the change */
	status_action *actions; /* actions to do to switch state (NULL terminated list) */
};

/* all FSM actions */
void status_send (struct status *, struct key *, enum status_event);
void status_send_latched (struct status *, struct key *, enum status_event);
void status_latch (struct status *, struct key *, enum status_event);
void status_unlatch (struct status *, struct key *, enum status_event);
void status_unlatch_all (struct status *, struct key *, enum status_event);
void status_lock (struct status *, struct key *, enum status_event);
void status_unlock (struct status *, struct key *, enum status_event);
void status_update_view (struct status *, struct key *, enum status_event);
void status_error (struct status *, struct key *, enum status_event);

/* FSM action groups */
static status_action status_action_s[]={ status_send, NULL };
static status_action status_action_la[]={ status_latch, status_update_view, NULL };
static status_action status_action_ulalo[]={ status_unlatch, status_lock, status_update_view, NULL };
static status_action status_action_lo[]={ status_lock, status_update_view, NULL };
static status_action status_action_ul[]={ status_unlock, status_update_view, NULL };
static status_action status_action_slas[]={ status_send_latched, status_send, NULL };
static status_action status_action_sslaulaa[]={ status_send, status_send_latched, status_unlatch_all, status_update_view, NULL };
static status_action status_action_uv[]={ status_update_view, NULL };
static status_action status_action_err[]={ status_error, NULL };

/* status FSM - simulate sticky keys
 * changes by type, event and state */
static struct status_change status_fsm[STATUS_EVENT_NUM][STATUS_KEY_TYPE_NUM][KEY_STATE_NUM]={
	{ /* PRESS event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, status_action_slas }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_LATCHED, status_action_la }, /* RELEASED state */
			{ KEY_RELEASED, status_action_ul }, /* LOCKED state */
			{ KEY_LOCKED, status_action_ulalo } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_RELEASED, status_action_s }, /* RELEASED state */
			{ KEY_LOCKED, status_action_s } /* LOCKED state */
		}
	}, { /* RELEASE event */
		{ /* NORMAL key */
			{ KEY_PRESSED, status_action_sslaulaa }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_RELEASED, status_action_s }, /* RELEASED state */
			{ KEY_LOCKED, status_action_s } /* LOCKED state */
		}
	}, { /* PRESSED event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_PRESSED, status_action_uv } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_PRESSED, status_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_LOCKED, status_action_lo }, /* RELEASED state */
			{ KEY_RELEASED, status_action_ul } /* LOCKED state */
		}
	}, { /* RELEASED event */
		{ /* NORMAL key */
			{ KEY_RELEASED, status_action_uv }, /* PRESSED state */
			{ KEY_RELEASED, NULL } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, status_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL } /* LOCKED state */
		}
	}
};

/* handle X11 errors */
int status_error_handler(Display *my_dpy, XErrorEvent *event)
{
	flo_warn(_("Unable to focus window."));
	return 0;
}

/* switch focus to focus window */
void status_focus_window(struct status *status)
{
	int (*old_handler)(Display *, XErrorEvent *);
	if (status->w_focus) {
		old_handler=XSetErrorHandler(status_error_handler);
		XSetInputFocus(gdk_x11_get_default_xdisplay(), status->w_focus->w,
				status->w_focus->revert_to, CurrentTime);
		XSync(gdk_x11_get_default_xdisplay(), FALSE);
		XSetErrorHandler(old_handler);
	}
}

/* process the fsm actions and state change */
void status_fsm_process(struct status *status, struct key *key, enum status_event event)
{
	enum key_state state;
	enum status_key_type type;
	guint idx;
	if (key) {
		state=key->state;
		type=(key_get_modifier(key)?(key_is_locker(key)?
			STATUS_KEY_LOCKER:STATUS_KEY_MODIFIER):STATUS_KEY_NORMAL);
		/* switch state */
		key_state_set(key, status_fsm[event][type][state].new_state);
		/* execute actions */
		status_focus_window(status);
		flo_debug(_("Event %d received :type %d. Switching from state %d to %d"),
			event, type, state, status_fsm[event][type][state].new_state);
		if (status_fsm[event][type][state].actions)
			for (idx=0;status_fsm[event][type][state].actions[idx];idx++)
				status_fsm[event][type][state].actions[idx](status, key, event);
	}
}

/* update the global modifier mask */
void status_globalmod_set(struct status *status, GdkModifierType mod)
{
	status->globalmod|=mod;
}

#ifdef ENABLE_XRECORD
/* Called when a record event is received from XRecord */
void status_record_event (XPointer priv, XRecordInterceptData *hook)
{
	xEvent *event;
	struct status *status=(struct status *)priv;
	struct key *key;
	if (hook->category==XRecordFromServer) {
		event=(xEvent *)hook->data;
		if ((key=status->keys[event->u.u.detail])) {
			if (event->u.u.type==KeyPress)
				status_fsm_process(status, key, STATUS_PRESSED);
			else if (event->u.u.type==KeyRelease)
				status_fsm_process(status, key, STATUS_RELEASED);
		}
	}
	if (hook) XRecordFreeData(hook);
}

/* Process record events (every 1/10th of a second) */
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
	Display *ctrl_disp=(Display *)gdk_x11_get_default_xdisplay();

	status->data_disp=XOpenDisplay(NULL);
	if (XRecordQueryVersion(ctrl_disp, &major, &minor)) {
		flo_info(_("XRecord extension found version=%d.%d"), major, minor);
		if (!(range=XRecordAllocRange())) flo_fatal(_("Unable to allocate memory for record range"));
		memset(range, 0, sizeof(XRecordRange));
		range->device_events.first=KeyPress;
		range->device_events.last=KeyRelease;
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
		if (((struct key_mod *)key->mods)->type==KEY_CODE) status->keys[key->code]=key;
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

/* return the currently pressed key */
struct key *status_pressed_get(struct status *status) { return status->pressed; }

/* update the pressed key: send the press event and update the view 
 * if pressed is NULL, then release the last pressed key.
 * WARNING: not multi-touch safe! */
void status_pressed_set(struct status *status, struct key *pressed)
{
	enum status_event event;
	/* find actions in fsm table */
	if (pressed) {
		event=STATUS_PRESS;
		status->pressed=pressed;
	} else event=STATUS_RELEASE;
	status_fsm_process(status, status->pressed, event);
	status->pressed=pressed;
}

/****************************/
/* FSM state change actions */
/****************************/

/* update the view according to the change */
void status_update_view (struct status *status, struct key *key, enum status_event event)
{
	/* update view */
	if (status->view) view_update(status->view, key, key_get_modifier(key));
}

/* send the event: press or release the key */
void status_send (struct status *status, struct key *key, enum status_event event)
{
	flo_debug(_("sending event %d"), event);
	switch(event) {
		case STATUS_PRESS:
			key_press(key, status);
#ifdef ENABLE_XRECORD
			if (key->actions)
#endif
			status_fsm_process(status, key, STATUS_PRESSED);
			break;
		case STATUS_RELEASE:
			key_release(key, status);
#ifdef ENABLE_XRECORD
			if (key->actions)
#endif
			status_fsm_process(status, key, STATUS_RELEASED);
			break;
		default: flo_warn(_("Unknown event type received: %d"), event); break;
	}
#ifdef ENABLE_XRECORD
	status_record_process(status);
#endif
}

/* press all latched keys */
void status_send_latched (struct status *status, struct key *key, enum status_event event)
{
	struct key *latched;
	GList *list=status->latched_keys;
	while (list) {
		latched=((struct key *)list->data);
		status_send(status, latched, event);
		list=list->next;
	}
	/* send "locked" modifier keys that are not lockers */
	list=status->locked_keys;
	while (list) {
		latched=((struct key *)list->data);
		if (!key_is_locker(latched)) {
			status_send(status, latched, event);
		}
		list=list->next;
	}
}

/* calculate globalmod according to latched and locked list */
void status_globalmod_calc(struct status *status)
{
	GdkModifierType globalmod=0;
	GList *list=status->latched_keys;
	while (list) {
		globalmod|=key_get_modifier((struct key *)list->data);
		list=list->next;
	}
	list=status->locked_keys;
	while (list) {
		globalmod|=key_get_modifier((struct key *)list->data);
		list=list->next;
	}
	status->globalmod=globalmod;
}

/* latch or lock a key (depending on state) */
void status_latchorlock (struct status *status, struct key *key, enum status_event event,
	enum key_state state)
{
	GList **list=(state==KEY_LATCHED?&(status->latched_keys):&(status->locked_keys));
	*list=g_list_append(*list, key);
	/* update globalmod */
	status_globalmod_set(status, key_get_modifier(key));
}

/* unlatch or unlock a key (depending on state) */
void status_unlatchorlock (struct status *status, struct key *key,
	enum status_event event, enum key_state state)
{
	GList **list=(state==KEY_LATCHED?&(status->latched_keys):&(status->locked_keys));
	GList *found;
	if ((found=g_list_find(*list, key)))
		*list=g_list_delete_link(*list, found);
	status_globalmod_calc(status);
}

/* latch a key */
void status_latch (struct status *status, struct key *key, enum status_event event)
{
	status_latchorlock(status, key, event, KEY_LATCHED);
}

/* unlatch a key */
void status_unlatch (struct status *status, struct key *key, enum status_event event)
{
	status_unlatchorlock(status, key, event, KEY_LATCHED);
}

/* unlatch all latched keys */
void status_unlatch_all (struct status *status, struct key *key, enum status_event event)
{
	struct key *latched;
	while(status->latched_keys) {
		latched=(struct key *)(g_list_first(status->latched_keys)->data);
		latched->state=KEY_RELEASED;
		status->latched_keys=g_list_delete_link(status->latched_keys,
			g_list_first(status->latched_keys));
		if (status->view) view_update(status->view, latched, TRUE);
	}
	status_globalmod_calc(status);
}

/* lock a key*/
void status_lock (struct status *status, struct key *key, enum status_event event)
{
	status_latchorlock(status, key, event, KEY_LOCKED);
}

/* unlock a key */
void status_unlock (struct status *status, struct key *key, enum status_event event)
{
	status_unlatchorlock(status, key, event, KEY_LOCKED);
}

/* print a status error */
void status_error (struct status *status, struct key *key, enum status_event event)
{
	flo_error(_("FSM state error. event=%d ; state=%d"), event, key->state);
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
		g_timeout_add(STATUS_ANIMATION_INTERVAL, update, data);
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
		ret=g_timer_elapsed(status->timer, NULL)*1000./settings_double_get("behaviour/auto_click");
	return ret;
}

/* get the list of latched keys */
GList *status_list_latched(struct status *status) { return status->latched_keys; }

/* get the list of locked keys */
GList *status_list_locked(struct status *status) { return status->locked_keys; }

/* get the global modifier mask */
GdkModifierType status_globalmod_get(struct status *status) { return status->globalmod; }

/* find a child window of win to focus by its name */
struct status_focus *status_find_subwin(Window parent, const gchar *win)
{
	Window root_return, parent_return;
	Window *children;
	guint nchildren, idx;
	gchar *name;
	struct status_focus *focus=NULL;
	XWindowAttributes attrs;
	if (XQueryTree(gdk_x11_get_default_xdisplay(), parent,
		&root_return, &parent_return, &children, &nchildren)) {
		for (idx=0;(idx<nchildren) && (!focus);idx++) {
			XFetchName(gdk_x11_get_default_xdisplay(), children[idx], &name);
			XGetWindowAttributes(gdk_x11_get_default_xdisplay(), children[idx], &attrs);

			if (attrs.map_state==IsViewable && name && (!strcmp(name, win))) {
				focus=g_malloc(sizeof(struct status_focus));
				if (!focus) flo_fatal(_("Unable to allocate memory for status focus"));
				focus->w=children[idx];
				focus->revert_to=RevertToPointerRoot;
				flo_info(_("Found window %s (ID=%ld)"), win, children[idx]);
			} else {
				focus=status_find_subwin(children[idx], win);
			}
			if (name) XFree(name);
		}
	} else {
		flo_warn(_("XQueryTree failed."));
	}
	return focus;
}

/* find a window to focus by its name */
/* TODO: wait for window to exist */
struct status_focus *status_find_window(const gchar *win)
{
	gchar *name;
	struct status_focus *focus=NULL;
	if (win && win[0]) {
		focus=status_find_subwin(gdk_x11_get_default_root_xwindow(), win);
	}
	if (!focus) {
		focus=g_malloc(sizeof(struct status_focus));
		if (!focus) flo_fatal(_("Unable to allocate memory for status focus"));
		XGetInputFocus(gdk_x11_get_default_xdisplay(), &(focus->w),
			&(focus->revert_to));
		XFetchName(gdk_x11_get_default_xdisplay(), focus->w, &name);
		if (win[0]) {
			flo_warn(_("Window not found: %s, using last focused window: %s (ID=%d)"),
				win, name, focus->w);
		} else {
			flo_info(_("Focussing window %s (ID=%d)"), name, focus->w);
		}
		if (name) XFree(name);
	}
	return focus;
}

/* allocate memory for status */
struct status *status_new(const gchar *focus_back)
{
	struct status *status=g_malloc(sizeof(struct status));
	if (!status) flo_fatal(_("Unable to allocate memory for status"));
	memset(status, 0, sizeof(struct status));
#ifdef ENABLE_XRECORD
	status_record_start(status);
	g_timeout_add(STATUS_EVENTCHECK_INTERVAL, status_record_process, (gpointer)status);
#endif
	status->spi=TRUE;
	if (focus_back) {
		status->w_focus=status_find_window(focus_back);
	}
	return status;
}

/* liberate status memory */
void status_free(struct status *status)
{
#ifdef ENABLE_XRECORD
	status_record_stop(status);
#endif
	if (status->xkeyboard) xkeyboard_free(status->xkeyboard);
	if (status->timer) g_timer_destroy(status->timer);
	if (status->latched_keys) g_list_free(status->latched_keys);
	if (status->locked_keys) g_list_free(status->locked_keys);
	if (status->w_focus) g_free(status->w_focus);
	if (status) g_free(status);
}

/* reset the status to its original state */
void status_reset(struct status *status)
{
	status->focus=NULL;
	status->pressed=NULL;
	if (status->timer) g_timer_destroy(status->timer);
	status->timer=NULL;
	if (status->latched_keys) g_list_free(status->latched_keys);
	if (status->locked_keys) g_list_free(status->locked_keys);
	status->latched_keys=NULL;
	status->locked_keys=NULL;
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
		(Display *)gdk_x11_get_default_xdisplay(),
		&event_base, &error_base, &major, &minor)) {
		flo_error(_("Neither at-spi nor XTest could be initialized."));
		flo_fatal(_("There is no way we can send keyboard events."));
	} else {
		flo_info(_("At-spi registry daemon is not running. "
			"XTest extension found: version=%d.%d; "
			"It will be used instead of at-spi."), major, minor);
	}
#else
	flo_error(_("Xtest extension not compiled in and at-spi not working"));
	flo_fatal(_("There is no way we can send keyboard events."));
#endif
	status->spi=FALSE;
}

/* tell if spi is enabled */
gboolean status_spi_is_enabled(struct status *status) { return status->spi; }

/* set/get moving status */
void status_set_moving(struct status *status, gboolean moving) { status->moving=moving; }
gboolean status_get_moving(struct status *status) { return status->moving; }

/* zoom the focused key */
void status_focus_zoom_set(struct status *status, gboolean focus_zoom) { status->focus_zoom=focus_zoom; }
gboolean status_focus_zoom_get(struct status *status) { return status->focus_zoom; }

