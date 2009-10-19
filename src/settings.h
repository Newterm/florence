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

#define SETTINGS_NONE "None"

/* This is a settings parameter type */
enum settings_type {
	SETTINGS_BOOL,
	SETTINGS_COLOR,
	SETTINGS_STRING,
	SETTINGS_DOUBLE,
	SETTINGS_INTEGER
};

/* This is a settings parameter. */
struct settings_param {
	gchar *glade_name;
	gchar *gconf_name;
	enum settings_type type;
	union {
		gboolean vbool;
		gchar *vstring;
		gdouble vdouble;
		gint vinteger;
	} default_value;
};

/* general informations related to the settings */
struct settings_info {
	GConfClient *gconfclient;
	gboolean gtk_exit;
	GKeyFile *config;
	gchar *config_file;
	gchar buffer[64];
};

/* Returns the absolute gconf path from a path relative to florence root */
/* ! not thread safe */
char *settings_get_full_path(const char *path);
void settings_init(gboolean exit, gchar *conf);
void settings_exit(void);
void settings(void);
void settings_changecb_register(gchar *name, GConfClientNotifyFunc cb, gpointer user_data);
/* register all events */
guint settings_register_all(GConfClientNotifyFunc cb);
/* unregister events */
void settings_unregister(guint notify_id);
/* commit a changeset */
void settings_commit(GConfChangeSet *cs);

/* get a value from gconf */
GConfValue *settings_value_get(const gchar *name);
/* get a gconf double */
gdouble settings_double_get(const gchar *name);
/* set a gconf double */
void settings_double_set(const gchar *name, gdouble value, gboolean notify);
/* get an integer from gconf */
gint settings_get_int(const gchar *name);
/* set a gconf integer */
void settings_set_int(const gchar *name, gint value);
/* get a gconf string */
gchar *settings_get_string(const gchar *name);
/* set a gconf string */
void settings_string_set(const gchar *name, const gchar *value);
/* get a gconf boolean */
gboolean settings_get_bool(const gchar *name);
/* set a gconf boolean */
void settings_bool_set(const gchar *name, gboolean value);

/* get parameters table */
struct settings_param *settings_defaults_get(void);
/* Returns the gconf name of a glade object option according to the name table */
char *settings_get_gconf_name(GtkWidget *widget);

