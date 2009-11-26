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

#include "system.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <glade/glade.h>
#include "settings.h"
#include "settings-window.h"
#include "trace.h"

#define FLO_SETTINGS_ROOT "/apps/florence"

/* used to split key into group and key for gKeyFile */
struct settings_key {
	gchar *group;
	gchar *key;
};

/* settings defaults. { "glade name", "gconf name", "type", "default value" } */
/* C99 */
static struct settings_param settings_defaults[] = {
	{ "flo_resizable", "window/resizable", SETTINGS_BOOL, { .vbool = TRUE } },
	{ "flo_keep_ratio", "window/keep_ratio", SETTINGS_BOOL, { .vbool = FALSE } },
	{ "flo_decorated", "window/decorated", SETTINGS_BOOL, { .vbool = FALSE } },
	{ "flo_auto_hide", "behaviour/auto_hide", SETTINGS_BOOL, { .vbool = FALSE } },
	{ "flo_move_to_widget", "behaviour/move_to_widget", SETTINGS_BOOL, { .vbool = TRUE } },
	{ "flo_intermediate_icon", "behaviour/intermediate_icon", SETTINGS_BOOL, { .vbool = TRUE } },
	{ "flo_transparent", "window/transparent", SETTINGS_BOOL, { .vbool = TRUE } },
	{ "flo_task_bar", "window/task_bar", SETTINGS_BOOL, { .vbool = FALSE } },
	{ "flo_always_on_top", "window/always_on_top", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_NONE, "window/keep_on_top", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_NONE, "behaviour/startup_notification", SETTINGS_BOOL, { .vbool = FALSE } },
	{ "flo_keys", "colours/key", SETTINGS_COLOR, { .vstring = "#000000" } },
	{ SETTINGS_NONE, "colours/outline", SETTINGS_COLOR, { .vstring = "#808080" } },
	{ "flo_labels", "colours/label", SETTINGS_COLOR, { .vstring = "#FFFFFF" } },
	{ "flo_activated", "colours/activated", SETTINGS_COLOR, { .vstring = "#FF0000" } },
	{ "flo_mouseover", "colours/mouseover", SETTINGS_COLOR, { .vstring = "#0000FF" } },
	{ "flo_latched", "colours/latched", SETTINGS_COLOR, { .vstring = "#00FF00" } },
	{ "flo_extensions", "layout/extensions", SETTINGS_STRING, { .vstring = "" } },
	{ "flo_layouts", "layout/file", SETTINGS_STRING, { .vstring = DATADIR "/layouts/florence.xml" } },
	{ "flo_preview", "layout/style", SETTINGS_STRING, { .vstring = DATADIR "/styles/default/florence.style" } },
	{ "flo_auto_click", "behaviour/auto_click", SETTINGS_DOUBLE, { .vdouble = 0. } },
	{ "flo_opacity", "window/opacity", SETTINGS_DOUBLE, { .vdouble = 100. } },
	{ SETTINGS_NONE, "window/zoom", SETTINGS_DOUBLE, { .vdouble = 20. } },
	{ "flo_focus_zoom", "style/focus_zoom", SETTINGS_DOUBLE, { .vdouble = 1.3 } },
	{ SETTINGS_NONE, "window/xpos", SETTINGS_INTEGER, { .vinteger = 0 } },
	{ SETTINGS_NONE, "window/ypos", SETTINGS_INTEGER, { .vinteger = 0 } },
	{ NULL } };

static struct settings_info *settings_infos=NULL;

/*********************/
/* private functions */
/*********************/

/* returns the defaults table's index for gconf name */
guint settings_default_idx(const gchar *gconf_name)
{
	guint ret=0;
	while (settings_defaults[ret].glade_name &&
		strcmp(gconf_name, settings_defaults[ret].gconf_name)) {
		ret++;
	}
	return ret;
}

/* split a key into group and key for gKeyFile */
/* ! not thread safe */
struct settings_key *settings_split(const gchar *key)
{
	static struct settings_key ret;
	if (strlen(key+1)>64) {
		flo_fatal(_("Settings/split: buffer overflow : %s"), key);
	}
	strcpy(settings_infos->buffer, key);
	ret.group=settings_infos->buffer;
	ret.key=g_strrstr(settings_infos->buffer, "/");
	*(ret.key++)='\0';
	return &ret;
}

/* called by settings_commit on each record of the changeset */
void settings_value_set (GConfChangeSet *cs, const gchar *name, GConfValue *value, gpointer user_data)
{
	struct settings_key *key;
	key=settings_split(name+strlen(FLO_SETTINGS_ROOT)+1);
	switch(value->type) {
		case GCONF_VALUE_STRING:
			g_key_file_set_string(settings_infos->config, key->group, key->key, 
				gconf_value_get_string(value));
			break;
		case GCONF_VALUE_INT:
			g_key_file_set_integer(settings_infos->config, key->group, key->key, 
				gconf_value_get_int(value));
			break;
		case GCONF_VALUE_FLOAT:
			g_key_file_set_double(settings_infos->config, key->group, key->key,
				gconf_value_get_float(value));
			break;
		case GCONF_VALUE_BOOL:
			g_key_file_set_boolean(settings_infos->config, key->group, key->key,
				gconf_value_get_bool(value));
			break;
		default:flo_warn(_("Unknown value type: %d"), value->type);
			break;
	}
}

/********************/
/* public functions */
/********************/

/* Returns the absolute gconf path from a path relative to florence root */
/* ! not thread safe */
char *settings_get_full_path(const char *path)
{
	if ((strlen(FLO_SETTINGS_ROOT)+strlen(path)+2)>64) {
		flo_fatal(_("Settings/get_full_path: buffer overflow : %s/%s"), FLO_SETTINGS_ROOT, path);
	}
	strcpy(settings_infos->buffer, FLO_SETTINGS_ROOT);
	strcat(settings_infos->buffer, "/");
	strcat(settings_infos->buffer, path);
	return settings_infos->buffer;
}

/* must be called before calling any settings function */
void settings_init(gboolean exit, gchar *conf)
{
	settings_infos=g_malloc(sizeof(struct settings_info));
	memset(settings_infos, 0, sizeof(struct settings_info));
	settings_infos->gtk_exit=exit;
	if (conf) {
		settings_infos->config=g_key_file_new();
		settings_infos->config_file=conf;
		if (!g_key_file_load_from_file(settings_infos->config, conf,
			G_KEY_FILE_KEEP_COMMENTS, NULL)) {
			flo_fatal(_("Unable to open file %s"), conf);
		}
		flo_info(_("Using configuration file %s"), conf);
	} else {
		settings_infos->gconfclient=gconf_client_get_default();
		gconf_client_add_dir(settings_infos->gconfclient, FLO_SETTINGS_ROOT,
			GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	}
	glade_init();
}

/* liberate all settings memory */
void settings_exit(void)
{
	GError *err=NULL;
	gsize len;
	gchar *data=NULL;
	if (settings_infos) {
		if (settings_infos->gconfclient) g_object_unref(G_OBJECT(settings_infos->gconfclient));
		if (settings_infos->config) {
			data=g_key_file_to_data(settings_infos->config, &len, NULL);
			if (data) {
				if (!g_file_set_contents(settings_infos->config_file, data, len, &err)) {
					flo_error(_("Unable to save configuration to %s: %s"),
						settings_infos->config_file, err->message);
				}
				g_free(data);
			}
			g_key_file_free(settings_infos->config);
		}
		g_free(settings_infos);
	}
	settings_window_free();
}

/* get parameters table */
struct settings_param *settings_defaults_get(void)
{
	return settings_defaults;
}

/* Returns the gconf name of a glade object option according to the name table */
char *settings_get_gconf_name(GtkWidget *widget)
{
	guint searchidx=0;
	while (settings_defaults[searchidx].glade_name &&
		strcmp(glade_get_widget_name(widget),
		settings_defaults[searchidx].glade_name)) {
		searchidx++;
	}
	return settings_defaults[searchidx].gconf_name;
}

/* register for gconf events */
void settings_changecb_register(gchar *name, GConfClientNotifyFunc cb, gpointer user_data)
{
	if (settings_infos->gconfclient) {
		gconf_client_notify_add(settings_infos->gconfclient,
			settings_get_full_path(name), cb, user_data, NULL, NULL);
	}
	/* TODO: propagate key_file events? */
}

/* register all events */
guint settings_register_all(GConfClientNotifyFunc cb)
{
	guint ret;
	if (settings_infos->gconfclient) {
		ret=gconf_client_notify_add(settings_infos->gconfclient, FLO_SETTINGS_ROOT,
			cb, NULL, NULL, NULL);
	} else { ret=1; }
	return ret;
}

/* unregister events */
void settings_unregister(guint notify_id)
{
	if (settings_infos->gconfclient) {
		gconf_client_notify_remove(settings_infos->gconfclient, notify_id);
	}
}

/* commit a changeset */
void settings_commit(GConfChangeSet *cs)
{
	if (settings_infos->gconfclient) {	
		gconf_client_commit_change_set(settings_infos->gconfclient, cs, TRUE, NULL);
	} else {
		gconf_change_set_foreach(cs, (GConfChangeSetForeachFunc)settings_value_set, NULL);
	}
	gconf_change_set_clear(cs);
}

/* get a value from gconf */
GConfValue *settings_value_get(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	GConfValue *ret;
	gchar *str=NULL;
	struct settings_key *key;

	if (settings_infos->gconfclient) {
		ret=gconf_client_get(settings_infos->gconfclient, fullpath, &err);
		if (err) {
			flo_error (_("gconf error reading key %s"), fullpath, err->message);
		}
	} else {
		key=settings_split(name);
		switch(settings_defaults[settings_default_idx(name)].type) {
			case SETTINGS_BOOL:
				ret=gconf_value_new(GCONF_VALUE_BOOL);
				gconf_value_set_bool(ret, g_key_file_get_boolean(settings_infos->config,
					key->group, key->key, &err));
				break;
			case SETTINGS_COLOR:
			case SETTINGS_STRING:
				ret=gconf_value_new(GCONF_VALUE_STRING);
				gconf_value_set_string(ret, g_key_file_get_string(settings_infos->config,
					key->group, key->key, &err));
				break;
			case SETTINGS_DOUBLE:
				ret=gconf_value_new(GCONF_VALUE_FLOAT);
				gconf_value_set_float(ret, g_key_file_get_double(settings_infos->config,
					key->group, key->key, &err));
				break;
			case SETTINGS_INTEGER:
				ret=gconf_value_new(GCONF_VALUE_INT);
				gconf_value_set_int(ret, g_key_file_get_integer(settings_infos->config,
					key->group, key->key, &err));
				break;
			default:
				flo_error(_("Unknown value type: %d"),
					settings_defaults[settings_default_idx(name)].type);
				break;
		}
		if (err) flo_fatal (_("Error reading value for %s: %s"), name, err->message);
	}
	if (ret) {
		str=gconf_value_to_string(ret);
		flo_debug_distinct("CONF:%s=<%s>", fullpath, str);
		if (str) g_free(str);
	} else {
 		flo_warn_distinct(_("No gconf value for %s. Using default."), fullpath);
	}
	return ret;
}

/* get an integer from gconf */
gint settings_get_int(const gchar *name)
{
	gint ret=0;
	GConfValue *val=settings_value_get(name);;
	if (!val) {
		ret=settings_defaults[settings_default_idx(name)].default_value.vdouble;
	} else {
		ret=gconf_value_get_int(val);
		gconf_value_free(val);
	}
	return ret;
}

/* set a gconf integer */
void settings_set_int(const gchar *name, gint value)
{
	struct settings_key *key;
	if (settings_infos->gconfclient) {
		gconf_client_remove_dir(settings_infos->gconfclient, FLO_SETTINGS_ROOT, NULL);
		gconf_client_set_int(settings_infos->gconfclient, settings_get_full_path(name), value, NULL);
		gconf_client_add_dir(settings_infos->gconfclient, FLO_SETTINGS_ROOT,
			GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	} else {
		key=settings_split(name);
		g_key_file_set_integer(settings_infos->config, key->group, key->key, value);
	}
}

/* get a double from gconf */
gdouble settings_double_get(const gchar *name)
{
	gdouble ret=0.0;
	GConfValue *val=settings_value_get(name);;
	if (!val) {
		ret=settings_defaults[settings_default_idx(name)].default_value.vdouble;
	} else {
		ret=gconf_value_get_float(val);
		gconf_value_free(val);
	}
	return ret;
}

/* set a gconf double */
void settings_double_set(const gchar *name, gdouble value, gboolean b)
{
	struct settings_key *key;
	if (settings_infos->gconfclient) {
		if (!b) gconf_client_remove_dir(settings_infos->gconfclient, FLO_SETTINGS_ROOT, NULL);
		gconf_client_set_float(settings_infos->gconfclient, settings_get_full_path(name), value, NULL);
		if (!b) gconf_client_add_dir(settings_infos->gconfclient, FLO_SETTINGS_ROOT,
			GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	} else {
		key=settings_split(name);
		g_key_file_set_double(settings_infos->config, key->group, key->key, value);
	}
}

/* get a gconf string */
gchar *settings_get_string(const gchar *name)
{
	gchar *ret=NULL;
	GConfValue *val=settings_value_get(name);;
	if (!val) {
		ret=g_strdup(settings_defaults[settings_default_idx(name)].default_value.vstring);
	} else {
		ret=g_strdup(gconf_value_get_string(val));
		gconf_value_free(val);
	}
	return ret;
}

/* set a gconf string */
void settings_string_set(const gchar *name, const gchar *value)
{
	struct settings_key *key;
	if (settings_infos->gconfclient) {
		gconf_client_set_string(settings_infos->gconfclient, settings_get_full_path(name), value, NULL);
	} else {
		key=settings_split(name);
		g_key_file_set_string(settings_infos->config, key->group, key->key, value);
	}
}

/* get a gconf boolean */
gboolean settings_get_bool(const gchar *name)
{
	gboolean ret=FALSE;
	GConfValue *val=settings_value_get(name);;
	if (!val) {
		ret=settings_defaults[settings_default_idx(name)].default_value.vbool;
	} else {
		ret=gconf_value_get_bool(val);
		gconf_value_free(val);
	}
	return ret;
}

/* set a gconf boolean */
void settings_bool_set(const gchar *name, gboolean value)
{
	struct settings_key *key;
	if (settings_infos->gconfclient) {
		gconf_client_set_bool(settings_infos->gconfclient, settings_get_full_path(name), value, NULL);
	} else {
		key=settings_split(name);
		g_key_file_set_boolean(settings_infos->config, key->group, key->key, value);
	}
}

/* Displays the settings dialog box on the screen and register events */
void settings(void)
{
	if (settings_window_open()) {
		settings_window_present();
	} else {
		settings_window_new(settings_infos->gconfclient, settings_infos->gtk_exit);
	}
}

