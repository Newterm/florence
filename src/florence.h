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
#include "key.h"
#include "status.h"

/* There is one florence structure which contains all global data in florence.c */
struct florence {
	struct status *status; /* the status of florence */	
	struct key *keys[256]; /* Florence keys sorted by keycode */
	/* Xkd data: only used at startup */
	XkbDescPtr xkb; /* Description of the hard keyboard from XKB */
	XkbStateRec state; /* current state of the hard keyboard */
};

/* Launch the virtual keyboard.
 * Returns: 0 on normal exit. */
int florence (void);

#endif

