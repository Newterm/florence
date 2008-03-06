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

struct keyboard {
	struct key *keys[256];
	GnomeCanvasGroup *canvas_group;
	GnomeCanvas *canvas;
	guchar *map;
	gdouble dwidth, dheight, zoom;
	guint width, height;
	GdkModifierType modstatus;
	guint shift;
	guint control;
	struct key *current;
	struct key *pressed;
	gdouble timer;
	gdouble timer_step;
};

struct keyboard *keyboard_new (GnomeCanvas *canvas);
void keyboard_free (struct keyboard *keyboard);
GnomeCanvas *keyboard_get_canvas(struct keyboard *keyboard);
guint keyboard_get_width(struct keyboard *keyboard);
guint keyboard_get_height(struct keyboard *keyboard);
guchar *keyboard_get_map(struct keyboard *keyboard);
gdouble keyboard_get_zoom(struct keyboard *keyboard);
void keyboard_resize(struct keyboard *keyboard, gdouble zoom);

