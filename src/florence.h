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

#include "keyboard.h"
#include "layoutreader.h"

/* An extension is a set of keys that can be show or hidden together
 * Examples: the numpad or the arrows. The main extension can is the trunk of the keyboard */
struct extension {
	struct keyboard *keyboard; /* Extension data */
	char *name;    /* NULL for main keyboard, "Arrows" or "Numpad" */
	int is_active; /* TRUE if the extension is visible ; ignored for main */
	enum layout_placement placement; /* Position of the extension relative to main */
};

/* Launches the virtual keyboard.
 * Returns: 0 on normal exit. */
int florence (void);

