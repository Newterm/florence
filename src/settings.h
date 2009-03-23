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

#include <gconf/gconf-client.h>
#include <gtk/gtk.h>

void settings_init(gboolean exit);
void settings_exit(void);
void settings(void);
void settings_changecb_register(gchar *name, GConfClientNotifyFunc cb, gpointer user_data);

/* get a gconf double */
gdouble settings_get_double(const gchar *name);
/* set a gconf double */
void settings_set_double(const gchar *name, gdouble value);
/* get an integer from gconf */
gint settings_get_int(const gchar *name);
/* set a gconf integer */
void settings_set_int(const gchar *name, gint value);
/* get a gconf string */
gchar *settings_get_string(const gchar *name);
/* get a gconf boolean */
gboolean settings_get_bool(const gchar *name);
/* set a gconf boolean */
void settings_bool_set(const gchar *name, gboolean value);

/* Create the $HOME/.florence directory */
gboolean settings_mkhomedir();

