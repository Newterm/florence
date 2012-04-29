/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2012 François Agrech

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
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>
#include "settings.h"
#include "settings-window.h"
#include "trace.h"

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

/* GSettings category names */
static const gchar *settings_cat_names[]={
	"org.florence",
	"org.florence.behaviour",
	"org.florence.window",
	"org.florence.colours",
	"org.florence.layout",
	"org.florence.style",
};

/* settings defaults. { "gtk builder name", "settings name", "type", "default value" } */
/* C99 */
static const struct settings_param settings_defaults[] = {
	{ SETTINGS_WINDOW, "flo_resizable", "resizable", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_WINDOW, "flo_keep_ratio", "keep-ratio", SETTINGS_BOOL, { .vbool = FALSE } },
	{ SETTINGS_WINDOW, "flo_decorated", "decorated", SETTINGS_BOOL, { .vbool = FALSE } },
	{ SETTINGS_BEHAVIOUR, "flo_auto_hide", "auto-hide", SETTINGS_BOOL, { .vbool = FALSE } },
	{ SETTINGS_BEHAVIOUR, SETTINGS_NONE, "hide-on-start", SETTINGS_BOOL, { .vbool = FALSE } },
	{ SETTINGS_BEHAVIOUR, "flo_move_to_widget", "move-to-widget", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_BEHAVIOUR, "flo_intermediate_icon", "intermediate-icon", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_WINDOW, "flo_transparent", "transparent", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_WINDOW, "flo_task_bar", "task-bar", SETTINGS_BOOL, { .vbool = FALSE } },
	{ SETTINGS_WINDOW, "flo_always_on_top", "always-on-top", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_WINDOW, SETTINGS_NONE, "keep-on-top", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_BEHAVIOUR, SETTINGS_NONE, "startup-notification", SETTINGS_BOOL, { .vbool = FALSE } },
	{ SETTINGS_COLORS, "flo_keys", "key", SETTINGS_COLOR, { .vstring = "#000000" } },
	{ SETTINGS_COLORS, SETTINGS_NONE, "outline", SETTINGS_COLOR, { .vstring = "#808080" } },
	{ SETTINGS_COLORS, "flo_labels", "label", SETTINGS_COLOR, { .vstring = "#FFFFFF" } },
	{ SETTINGS_COLORS, "flo_label_outline", "label-outline", SETTINGS_COLOR, { .vstring = "#000000" } },
	{ SETTINGS_COLORS, "flo_activated", "activated", SETTINGS_COLOR, { .vstring = "#FF0000" } },
	{ SETTINGS_COLORS, "flo_mouseover", "mouseover", SETTINGS_COLOR, { .vstring = "#0000FF" } },
	{ SETTINGS_COLORS, "flo_latched", "latched", SETTINGS_COLOR, { .vstring = "#00FF00" } },
	{ SETTINGS_COLORS, SETTINGS_NONE, "ramble", SETTINGS_COLOR, { .vstring = "#FF00FFAA" } },
	{ SETTINGS_LAYOUT, "flo_extensions", "extensions", SETTINGS_STRING, { .vstring = "" } },
	{ SETTINGS_LAYOUT, "flo_layouts", "file", SETTINGS_STRING, { .vstring = DATADIR "/layouts/florence.xml" } },
	{ SETTINGS_LAYOUT, "flo_preview", "style", SETTINGS_STRING, { .vstring = DATADIR "/styles/default/florence.style" } },
	{ SETTINGS_BEHAVIOUR, "input_method_combo", "input-method", SETTINGS_STRING, { .vstring = "button" } },
	{ SETTINGS_BEHAVIOUR, "flo_timer", "timer", SETTINGS_DOUBLE, { .vdouble = 1300. } },
	{ SETTINGS_BEHAVIOUR, "ramble_threshold1", "ramble-threshold1", SETTINGS_DOUBLE, { .vdouble = 1.3 } },
	{ SETTINGS_BEHAVIOUR, "ramble_threshold2", "ramble-threshold2", SETTINGS_DOUBLE, { .vdouble = 3.0 } },
	{ SETTINGS_BEHAVIOUR, "ramble_timer", "ramble-timer", SETTINGS_DOUBLE, { .vdouble = 300.0 } },
	{ SETTINGS_BEHAVIOUR, "ramble_button", "ramble-button", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_BEHAVIOUR, "ramble_algo", "ramble-algo", SETTINGS_STRING, { .vstring = "distance" } },
	{ SETTINGS_WINDOW, "flo_opacity", "opacity", SETTINGS_DOUBLE, { .vdouble = 100. } },
	{ SETTINGS_WINDOW, SETTINGS_NONE, "scalex", SETTINGS_DOUBLE, { .vdouble = 20. } },
	{ SETTINGS_WINDOW, SETTINGS_NONE, "scaley", SETTINGS_DOUBLE, { .vdouble = 20. } },
	{ SETTINGS_STYLE, "flo_focus_zoom", "focus-zoom", SETTINGS_DOUBLE, { .vdouble = 1.3 } },
	{ SETTINGS_STYLE, "flo_sounds", "sounds", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_STYLE, "flo_system_font", "system-font", SETTINGS_BOOL, { .vbool = TRUE } },
	{ SETTINGS_STYLE, "flo_font", "font", SETTINGS_STRING, { .vstring = "sans 10" } },
	{ SETTINGS_WINDOW, SETTINGS_NONE, "xpos", SETTINGS_INTEGER, { .vinteger = 0 } },
	{ SETTINGS_WINDOW, SETTINGS_NONE, "ypos", SETTINGS_INTEGER, { .vinteger = 0 } },
	{ 0, NULL } };

static struct settings_info *settings_infos=NULL;

/*********************/
/* private functions */
/*********************/

/* liberate memory for a registration */
void settings_registration_free(gpointer data, gpointer userdata)
{
	START_FUNC
	struct settings_registration *registration=(struct settings_registration *)data;
	if (registration->key) g_free(registration->key);
	END_FUNC
}

/* get a registration record from key name */
struct settings_registration *settings_registration_get(enum settings_item key)
{
	GSList *list;
	struct settings_registration *ret=NULL;
	if (settings_infos) {
		list=settings_infos->registrations;
		while (list) {
			if (((struct settings_registration *)list->data)->item==key) {
				ret=(struct settings_registration *)list->data;
				break;
			}
			list=list->next;
		}
	}
	if (ret) {
		while (ret->prev) ret=ret->prev;
	}
	return ret;
}

/* Create a new GSettings object */
GSettings *settings_new_object(gchar *file, const gchar *cat)
{
	GSettingsBackend *backend;
	GSettings *ret;
	if (file) {
		backend=g_keyfile_settings_backend_new(file, cat, NULL);
		ret=g_settings_new_with_backend(cat, backend);
		flo_info(_("Using configuration file %s"), file);
	} else {
		ret=g_settings_new(cat);
	}
	return ret;
}

/********************/
/* public functions */
/********************/

/* must be called before calling any settings function */
void settings_init(gboolean exit, gchar *conf)
{
	START_FUNC
	enum settings_cat cat;
	settings_infos=g_malloc(sizeof(struct settings_info));
	memset(settings_infos, 0, sizeof(struct settings_info));
	settings_infos->gtk_exit=exit;
	for (cat=0; cat<SETTINGS_NUM_CATS; cat++) {
		settings_infos->settings[cat]=settings_new_object(conf, settings_cat_names[cat]);
	}
	END_FUNC
}

/* liberate all settings memory */
void settings_exit(void)
{
	START_FUNC
	GError *err=NULL;
	gsize len;
	gchar *data=NULL;
	enum settings_cat cat;
	if (settings_infos) {
		for (cat=0; cat<SETTINGS_NUM_CATS; cat++)
			if (settings_infos->settings[cat])
				g_object_unref(G_OBJECT(settings_infos->settings[cat]));
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
		g_slist_foreach(settings_infos->registrations, settings_registration_free, NULL);
		g_slist_free(settings_infos->registrations);
		g_free(settings_infos);
	}
	settings_window_free();
	END_FUNC
}

/* get parameters table */
const struct settings_param *settings_defaults_get(void)
{
	START_FUNC
	END_FUNC
	return settings_defaults;
}

/* Get the settings item from gtkbuilder name */
enum settings_item settings_get_settings_name(GtkWidget *widget)
{
	START_FUNC
	const gchar* widget_name=gtk_buildable_get_name(GTK_BUILDABLE(widget));
	enum settings_item item=0;
	while (settings_defaults[item].builder_name &&
		strcmp(widget_name,
		settings_defaults[item].builder_name)) {
		item++;
	}
	END_FUNC
	return item;
}

/* register for settings changes */
void settings_changecb_register(enum settings_item item, settings_callback cb, gpointer user_data)
{
	START_FUNC
	struct settings_registration *registration, *similar;
	if (settings_infos->settings) {
		registration=g_malloc(sizeof(struct settings_registration));
		memset(registration, 0, sizeof(struct settings_registration));
		registration->item=item;
		registration->key=g_strdup_printf("changed::%s", settings_defaults[item].settings_name);
		registration->id=g_signal_connect(G_OBJECT(settings_infos->settings[settings_defaults[item].cat]),
			registration->key, G_CALLBACK(cb), user_data);
		registration->cb=cb;
		registration->user_data=user_data;
		similar=settings_registration_get(item);
		if (similar) {
			similar->prev=registration;
			registration->next=similar;
		}
		settings_infos->registrations=g_slist_append(settings_infos->registrations, registration);
	}
	END_FUNC
}

/* register all events */
guint settings_register_all(settings_callback cb)
{
	START_FUNC
	guint ret;
	if (settings_infos->settings[SETTINGS_ALL]) {
		g_signal_connect(G_OBJECT(settings_infos->settings[SETTINGS_ALL]), "changed", G_CALLBACK(cb), NULL);
	} else { ret=1; }
	END_FUNC
	return ret;
}

/* unregister events */
void settings_unregister(guint notify_id)
{
	START_FUNC
	if (settings_infos->settings[SETTINGS_ALL]) {
		g_signal_handler_disconnect(settings_infos->settings[SETTINGS_ALL], notify_id);
	}
	END_FUNC
}

/* create a transaction */
void settings_transaction()
{
	START_FUNC
	enum settings_cat cat;
	for (cat=0; cat<SETTINGS_NUM_CATS; cat++)
		if (settings_infos->settings[cat]) {
			g_settings_delay(settings_infos->settings[cat]);
		}
	settings_infos->transaction=TRUE;
	END_FUNC
}

/* commit buffered changes */
void settings_commit()
{
	START_FUNC
	enum settings_cat cat;
	for (cat=0; cat<SETTINGS_NUM_CATS; cat++)
		if (settings_infos->settings[cat]) {
			g_settings_apply(settings_infos->settings[cat]);
		}
	settings_infos->transaction=FALSE;
	END_FUNC
}

/* revert buffered changes */
void settings_rollback()
{
	START_FUNC
	enum settings_cat cat;
	for (cat=0; cat<SETTINGS_NUM_CATS; cat++)
		if (settings_infos->settings[cat]) {
			g_settings_revert(settings_infos->settings[cat]);
		}
	settings_infos->transaction=FALSE;
	END_FUNC
}

/* check if there is any uncommited change */
gboolean settings_dirty()
{
	START_FUNC
	enum settings_cat cat;
	gboolean ret=FALSE;
	for (cat=0; cat<SETTINGS_NUM_CATS; cat++)
		ret |= (settings_infos->settings[cat] &&
			g_settings_get_has_unapplied(settings_infos->settings[cat]));
	END_FUNC
	return ret;
}

/* get a value from gsettings */
GVariant *settings_value_get(enum settings_item item)
{
	START_FUNC
	GVariant *ret;
	gchar *str=NULL;
	gchar *name=settings_defaults[item].settings_name;

	if (settings_infos->settings) {
		ret=g_settings_get_value(settings_infos->settings[settings_defaults[item].cat], name);
	}
	if (ret) {
		str=g_variant_print(ret, TRUE);
		flo_debug_distinct(TRACE_DEBUG, "CONF:%s=<%s>", name, str);
		if (str) g_free(str);
	} else {
 		flo_warn_distinct(_("No GSettings value for %s. Using default."), name);
	}
	END_FUNC
	return ret;
}

/* get an integer from gsettings */
gint settings_get_int(enum settings_item item)
{
	START_FUNC
	gint ret=0;
	GVariant *val=settings_value_get(item);
	if (!val) {
		ret=settings_defaults[item].default_value.vinteger;
	} else {
		ret=(gint)g_variant_get_int32(val);
		g_variant_unref(val);
	}
	END_FUNC
	return ret;
}

/* set a gsettings integer */
void settings_set_int(enum settings_item item, gint value)
{
	START_FUNC
	gchar *name=settings_defaults[item].settings_name;
	enum settings_cat cat=settings_defaults[item].cat;
	struct settings_registration *registration, *list;
	if (settings_infos->settings) {
		registration=settings_registration_get(item);
		list=registration;
		while (list) {
			g_signal_handler_disconnect(settings_infos->settings[cat], list->id);
			list=list->next;
		}
		g_settings_set_int(settings_infos->settings[cat], name, value);
		list=registration;
		while (list) {
			list->id=g_signal_connect(G_OBJECT(settings_infos->settings[cat]),
				list->key, G_CALLBACK(list->cb), list->user_data);
			list=list->next;
		}
	}
	END_FUNC
}

/* get a double from gsettings */
gdouble settings_get_double(enum settings_item item)
{
	START_FUNC
	gdouble ret=0.0;
	GVariant *val=settings_value_get(item);
	if (!val) {
		ret=settings_defaults[item].default_value.vdouble;
	} else {
		ret=g_variant_get_double(val);
		g_variant_unref(val);
	}
	END_FUNC
	return ret;
}

/* set a gsettings double */
void settings_set_double(enum settings_item item, double value, gboolean b)
{
	START_FUNC
	gchar *name=settings_defaults[item].settings_name;
	enum settings_cat cat=settings_defaults[item].cat;
	struct settings_registration *registration, *list;
	if (settings_infos->settings) {
		registration=settings_registration_get(item);
		list=registration;
		while (!b && list) {
			g_signal_handler_disconnect(settings_infos->settings[cat], list->id);
			list=list->next;
		}
		g_settings_set_double(settings_infos->settings[cat], name, value);
		list=registration;
		while (!b && list) {
			list->id=g_signal_connect(G_OBJECT(settings_infos->settings[cat]),
				list->key, G_CALLBACK(list->cb), list->user_data);
			list=list->next;
		}
	}
	END_FUNC
}

/* get a string from gsettings */
gchar *settings_get_string(enum settings_item item)
{
	START_FUNC
	gchar *ret=NULL;
	GVariant *val=settings_value_get(item);
	if (!val) {
		ret=g_strdup(settings_defaults[item].default_value.vstring);
	} else {
		ret=g_strdup(g_variant_get_string(val, NULL));
		g_variant_unref(val);
	}
	END_FUNC
	return ret;
}

/* set a gsettings string */
void settings_set_string(enum settings_item item, const gchar *value)
{
	START_FUNC
	gchar *name=settings_defaults[item].settings_name;
	if (settings_infos->settings) {
		g_settings_set_string(settings_infos->settings[settings_defaults[item].cat],
			name, value);
	}
	END_FUNC
}

/* get a boolean from gsettings */
gboolean settings_get_bool(enum settings_item item)
{
	START_FUNC
	gboolean ret=FALSE;
	GVariant *val=settings_value_get(item);
	if (!val) {
		ret=settings_defaults[item].default_value.vbool;
	} else {
		ret=g_variant_get_boolean(val);
		g_variant_unref(val);
	}
	END_FUNC
	return ret;
}

/* set a gsettings boolean */
void settings_set_bool(enum settings_item item, gboolean value)
{
	START_FUNC
	gchar *name=settings_defaults[item].settings_name;
	if (settings_infos->settings) {
		g_settings_set_boolean(settings_infos->settings[settings_defaults[item].cat],
			name, value);
	}
	END_FUNC
}

/* Displays the settings dialog box on the screen and register events */
void settings(void)
{
	START_FUNC
	if (settings_window_open()) {
		settings_window_present();
	} else {
		settings_window_new(settings_infos->gtk_exit);
	}
	END_FUNC
}

