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

#define SETTINGS_NONE "None"

/* callback for registered events */
typedef void (*settings_callback) (GSettings *settings, gchar *key, gpointer user_data);

/* This is a settings catetory */
enum settings_cat {
	SETTINGS_ALL,
	SETTINGS_BEHAVIOUR,
	SETTINGS_WINDOW,
	SETTINGS_COLORS,
	SETTINGS_LAYOUT,
	SETTINGS_STYLE,
	SETTINGS_NUM_CATS
};

/* This is a settings key */
enum settings_item {
	SETTINGS_RESIZABLE,
	SETTINGS_KEEP_RATIO,
	SETTINGS_DECORATED,
	SETTINGS_AUTO_HIDE,
	SETTINGS_HIDE_ON_START,
	SETTINGS_MOVE_TO_WIDGET,
	SETTINGS_INTERMEDIATE_ICON,
	SETTINGS_TRANSPARENT,
	SETTINGS_TASK_BAR,
	SETTINGS_ALWAYS_ON_TOP,
	SETTINGS_KEEP_ON_TOP,
	SETTINGS_STARTUP_NOTIFICATION,
	SETTINGS_KEY,
	SETTINGS_OUTLINE,
	SETTINGS_LABEL,
	SETTINGS_LABEL_OUTLINE,
	SETTINGS_ACTIVATED,
	SETTINGS_MOUSEOVER,
	SETTINGS_LATCHED,
	SETTINGS_RAMBLE,
	SETTINGS_EXTENSIONS,
	SETTINGS_FILE,
	SETTINGS_STYLE_ITEM,
	SETTINGS_INPUT_METHOD,
	SETTINGS_TIMER,
	SETTINGS_RAMBLE_THRESHOLD1,
	SETTINGS_RAMBLE_THRESHOLD2,
	SETTINGS_RAMBLE_TIMER,
	SETTINGS_RAMBLE_BUTTON,
	SETTINGS_RAMBLE_ALGO,
	SETTINGS_OPACITY,
	SETTINGS_SCALEX,
	SETTINGS_SCALEY,
	SETTINGS_FOCUS_ZOOM,
	SETTINGS_SOUNDS,
	SETTINGS_SYSTEM_FONT,
	SETTINGS_FONT,
	SETTINGS_XPOS,
	SETTINGS_YPOS,
	SETTINGS_NUM_ITEMS
};

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
	enum settings_cat cat;
	gchar *builder_name;
	gchar *settings_name;
	enum settings_type type;
	union {
		gboolean vbool;
		gchar *vstring;
		gdouble vdouble;
		gint vinteger;
	} default_value;
};

/* Record key change registrations. */
struct settings_registration {
	enum settings_item item;
	gchar *key;
	gint id;
	settings_callback cb;
	gpointer user_data;
	struct settings_registration *prev;
	struct settings_registration *next;
};

/* general informations related to the settings */
struct settings_info {
	GSettings *settings[SETTINGS_NUM_CATS];
	gboolean transaction; /* TRUE when a transaction is open. */
	gboolean gtk_exit;
	GKeyFile *config;
	gchar *config_file;
	GSList *registrations;
};

/* Initialize the settings module */
void settings_init(gboolean exit, gchar *conf);
/* Liberate memory used by the settings module */
void settings_exit(void);
/* Open the settings window */
void settings(void);

/* register for settings changes */
void settings_changecb_register(enum settings_item item, settings_callback cb, gpointer user_data);
/* register all events */
guint settings_register_all(settings_callback cb);
/* unregister events */
void settings_unregister(guint notify_id);

/* create a transaction */
void settings_transaction();
/* commit buffered changes */
void settings_commit();
/* revert buffered changes */
void settings_rollback();
/* check if there is any uncommited change */
gboolean settings_dirty();

/* get a value from GSettings */
GVariant *settings_get_value(enum settings_item item);
/* get a GSettings double */
gdouble settings_get_double(enum settings_item item);
/* set a GSettings double */
void settings_set_double(enum settings_item item, gdouble value, gboolean notify);
/* get an integer from GSettings */
gint settings_get_int(enum settings_item item);
/* set a GSettings integer */
void settings_set_int(enum settings_item item, gint value);
/* get a GSettings string */
gchar *settings_get_string(enum settings_item item);
/* set a GSettings string */
void settings_set_string(enum settings_item item, const gchar *value);
/* get a GSettings boolean */
gboolean settings_get_bool(enum settings_item item);
/* set a GSettings boolean */
void settings_set_bool(enum settings_item item, gboolean value);

/* get parameters table */
const struct settings_param *settings_defaults_get(void);
/* Returns the GSettings name of a builder object option according to the name table */
enum settings_item settings_get_settings_name(GtkWidget *widget);

