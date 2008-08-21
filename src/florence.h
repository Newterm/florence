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

#ifndef FLORENCE
#define FLORENCE

#include <X11/XKBlib.h>
#include <gtk/gtk.h>
#include "style.h"
#include "key.h"
#include "keyboard.h"

/* There is one florence structure which contains all global data in florence.c */
struct florence {
	GtkWindow *window; /* GTK window of florence */
	guint width, height; /* dimensions of florence, in pixels */
	gdouble zoom; /* scaling factor of florence window */
	struct key *keys[256]; /* Florence keys sorted by keycode */
	struct key *current; /* focus key (key located under the pointer) or NULL */
	struct key *pressed; /* key currently being pressed or NULL */
	GTimer *timer; /* auto click timer: amount of time the mouse has been over the current key */
	GSList *dirtykeys; /* list of keys that need to be redrawn */
	GdkModifierType globalmod; /* global modifier mask */
	GList *pressedmodkeys; /* list of modifier keys that are pressed not including locker keys */
	GSList *keyboards; /* Main list of keyboard extensions */
	gdouble xoffset, yoffset; /* offset of the main keyboard */
	guchar *hitmap; /* bitmap of key codes: used to know on which key the mouse is over */
	struct style *style; /* Do it with style */
	gboolean composite; /* true if the screen has composite extension */
	/* Xkd data: only used at startup */
	XkbDescPtr xkb; /* Description of the hard keyboard from XKB */
	XkbStateRec state; /* current state of the hard keyboard */
};

/* Launch the virtual keyboard.
 * Returns: 0 on normal exit. */
int florence (void);

#endif

