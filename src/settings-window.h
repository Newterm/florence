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

/* all the informations related to the settings window */
struct settings_window {
	GtkBuilder *gtkbuilder;
	gboolean gtk_exit;
	GtkListStore *style_list;
	GtkListStore *layout_list;
	GtkListStore *input_method_list;
	guint notify_id;
	GSList *extensions;
};

/* returns true if settings window is open */
gboolean settings_window_open(void);
/* presents the settings window to the user */
void settings_window_present(void);
/* opens the settings window */
void settings_window_new(gboolean exit);
/* liberate memory used by settings window */
void settings_window_free();

