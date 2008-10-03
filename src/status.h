/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

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

#ifndef FLO_STATUS
#define FLO_STATUS

#include <gtk/gtk.h>
#include "key.h"
#include "view.h"

struct view;

/* This represents the status of florence */
struct status {
	struct key *focus; /* focus key (key located under the pointer) or NULL */
	GTimer *timer; /* auto click timer: amount of time the mouse has been over the current key */
	struct key *pressed; /* key currently being pressed or NULL */
	GList *pressedkeys; /* the list of all currently pressed keys */
	GdkModifierType globalmod; /* global modifier mask */
	struct view *view; /* view to update on status change */
};

/* update the focus key */
void status_focus_set(struct status *status, struct key *focus);
/* return the focus key */
struct key *status_focus_get(struct status *status);
/* update the pressed key */
void status_pressed_set(struct status *status, gboolean pressed);
/* returns the keycode currently focussed */
guint status_keycode_get(struct status *status, gint x, gint y);

/* start the timer */
void status_timer_start(struct status *status, GSourceFunc update, gpointer data);
/* stop the timer */
void status_timer_stop(struct status *status);
/* get timer value */
gdouble status_timer_get(struct status *status);

/* add a key pressed to the list of pressed keys */
void status_press(struct status *status, struct key *key);
/* remove a key pressed from the list of pressed keys */
void status_release(struct status *status, struct key *key);
/* get the list of pressed keys */
GList *status_pressedkeys_get(struct status *status);

/* get the global modifier mask */
GdkModifierType status_globalmod_get(struct status *status);

/* allocate memory for status */
struct status *status_new(struct key *keys[]);
/* liberate status memory */
void status_free(struct status *status);

/* sets the view to update on status change */
void status_view_set(struct status *status, struct view *view);

#endif


