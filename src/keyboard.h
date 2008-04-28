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

#include "key.h"
#include <libgnomecanvas/libgnomecanvas.h>
#include <libxml/xmlreader.h>

/* A keyboard is a set of keys logically grouped together */
/* Examples: the main keyboard, the numpad or the function keys */
struct keyboard {
	struct key **keys; /* key code indexed list of keys */
	GnomeCanvasGroup *canvas_group; /* GnomeCanvasGroup used for drowing the keyboard */
	GnomeCanvas *canvas; /* Canvas of the keyboard (parent of the canvas group) */
	guchar *map; /* byte map representing the keyboard. the byte value is equal to the keycode. */
	gdouble dwidth, dheight, zoom; /* logical with and height and pixels per unit */
	guint width, height; /* width and height in pixels (width=dwidth*zoom) */
	GdkModifierType modstatus; /* modifier mask representing the modifier state of the keyboard */
	guint shift; /* number of shift keys pressed */
	guint control; /* number of control keys pressed */
	struct key *current; /* key under the mouse (NULL if the mouse is not over a key) */
	struct key *pressed; /* key currently being pressed */
	gdouble timer; /* counts the number of ms the mouse is over a key (for autoclickr) */
	gdouble timer_step; /* number of ms to add to timer at each period */
};

/* create a keyboard: the layout is passed as a text reader */
struct keyboard *keyboard_new (GnomeCanvas *canvas, struct key **keys, xmlTextReaderPtr reader, int level);
/* delete a keyboard */
void keyboard_free (struct keyboard *keyboard);
/* Returns Keyboard canvas */
GnomeCanvas *keyboard_get_canvas(struct keyboard *keyboard);
/* Returns Keyboard width in pixels */
guint keyboard_get_width(struct keyboard *keyboard);
/* Returns Keyboard height in pixels */
guint keyboard_get_height(struct keyboard *keyboard);
/* Returns the keyboard byte map (one byte=one pixel=one keycode) */
guchar *keyboard_get_map(struct keyboard *keyboard);
/* Returns the zoom factor (1/2 of a pixel per unit) of the keyboard
 * A zoom of 10 means 20 pixels per unit */
gdouble keyboard_get_zoom(struct keyboard *keyboard);
/* Resize the keyboard according to the new scaling factor */
void keyboard_resize(struct keyboard *keyboard, gdouble zoom);

