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

#ifndef FLO_XKEYBOARD
#define FLO_XKEYBOARD

#ifdef ENABLE_XKB
#include <X11/XKBlib.h>
#endif
#include <glib.h>

/* this structure contains xkb data */
struct xkeyboard {
#ifdef ENABLE_XKB
        XkbDescPtr xkb_desc; /* Keyboard description from XKB */
        XkbStateRec xkb_state; /* Keyboard Status (get from XKB) */
#endif
	GList *groups; /* list of xkb configured groups (layout names) */
};

/* returns the current layout name */
gchar *xkeyboard_current_layout_get(struct xkeyboard *xkeyboard);

/* switch keyboard layout */
void xkeyboard_layout_change(struct xkeyboard *xkeyboard);

/* returns a new allocated structure containing data from xkb */
struct xkeyboard *xkeyboard_new();

/* liberate memory used by the modifier map */
void xkeyboard_client_map_free(struct xkeyboard *xkeyboard);

/* liberate any memory used to record xkb data */
void xkeyboard_free(struct xkeyboard *xkeyboard);

#endif
