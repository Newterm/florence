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
void status_pressed_set(struct status *status, gboolean pressed)
{
	gboolean redrawall;
	if (pressed) {
		status->pressed=status->focus;
		if (status->pressed) {
			redrawall=key_get_modifier(status->pressed) || status->globalmod;
			key_press(status->pressed, status);
			view_update(status->view, status->pressed, redrawall);
		}
	} else {
		if (status->pressed) {
			key_release(status->pressed);
			if (!key_is_pressed(status->pressed)) status_release(status, status->pressed);
			view_update(status->view, status->pressed, FALSE);
			status->pressed=NULL;
		}
	}
}

/* returns the keycode currently focussed */
guint status_keycode_get(struct status *status, gint x, gint y)
{
	return view_keycode_get(status->view, x, y);
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
	return status;
}

/* liberate status memory */
void status_free(struct status *status)
{
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

