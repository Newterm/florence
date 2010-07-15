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

#include <gtk/gtk.h>
#include "system.h"
#ifdef ENABLE_NOTIFICATION
	#include <libnotify/notify.h>
#endif
#include "view.h"

/* A trayicon object is holding the informations about the tray icon */
struct trayicon {
	GtkStatusIcon *tray_icon; /* GTK representation of the tray icon */
	GCallback trayicon_quit; /* Callback called to quit the applications (when the Quit menu is selected) */
	struct view *view; /* View shown or hidden on left click on the tray icon */
#ifdef ENABLE_NOTIFICATION
	NotifyNotification *notification; /* startup notification */
#endif
};

/* Display the about dialog window */
void trayicon_about(void);
/* Open yelp */
void trayicon_help(void);

/* Creates a new trayicon instance */
struct trayicon *trayicon_new(struct view *view, GCallback quit_cb);
/* Deallocate all the memory used bu the trayicon. */
void trayicon_free(struct trayicon *trayicon);

