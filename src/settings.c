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

/* Note : this is both the viewer and the controller */

#include "system.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <glade/glade.h>
#include <glib/gprintf.h>
#ifdef ENABLE_HELP
#include <libgnome/gnome-help.h>
#include <gdk/gdkkeysyms.h>
#endif
#include "settings.h"
#include "trace.h"
#include "layoutreader.h"
#include "style.h"
#include "tools.h"

#define FLO_SETTINGS_ROOT "/apps/florence"
#if GTK_CHECK_VERSION(2,12,0)
#define FLO_SETTINGS_ICON_CANCEL GTK_STOCK_DISCARD
#else
#define FLO_SETTINGS_ICON_CANCEL GTK_STOCK_CANCEL
#endif

static gchar *settings_nametable[][2] = {
	{ "flo_resizable", "window/resizable" },
	{ "flo_keep_ratio", "window/keep_ratio" },
	{ "flo_decorated", "window/decorated" },
	{ "flo_auto_hide", "behaviour/auto_hide" },
	{ "flo_transparent", "window/transparent" },
	{ "flo_task_bar", "window/task_bar" },
	{ "flo_always_on_top", "window/always_on_top" },
	{ NULL, NULL } };

static GladeXML *gladexml;
static GConfClient *gconfclient=NULL;
static GConfChangeSet *gconfchangeset=NULL;
static GConfChangeSet *rollback=NULL;
static gboolean settings_gtk_exit=FALSE;
static GtkListStore *settings_style_list=NULL;
static guint settings_notify_id=0;

/*********************/
/* private functions */
/*********************/

/* update rollback changeset */
void settings_save(const char *path)
{
	GConfValue *value;
	if (!gconf_change_set_check_value(rollback, path, &value)) {
		value=gconf_client_get(gconfclient, path, NULL);
		gconf_change_set_set(rollback, path, value);
	}
}

/* Returns the absolute gconf path from a path relative to florence root */
char *settings_get_full_path(const char *path)
{
	static char string_buffer[64];
	if ((strlen(FLO_SETTINGS_ROOT)+strlen(path)+2)>64) {
		flo_fatal(_("Settings/get_full_path: buffer overflow : %s/%s"), FLO_SETTINGS_ROOT, path);
	}
	strcpy(string_buffer, FLO_SETTINGS_ROOT);
	strcat(string_buffer, "/");
	strcat(string_buffer, path);
	return string_buffer;
}

/* Returns the gconf name of a glade object option according to the name table */
char *settings_get_gconf_name(GtkWidget *widget)
{
	guint searchidx=0;
	while (strcmp(glade_get_widget_name(widget),
		settings_nametable[searchidx][0]) &&
		settings_nametable[searchidx][0]) {
		searchidx++;
	}
	return settings_nametable[searchidx][1];
}

/* Fills the preview icon view with icons representing the themes */
void settings_preview_build()
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	xmlTextReaderPtr layout;
	struct style *style;
	DIR *dp=opendir(DATADIR "/styles");
	struct dirent *ep;
	gchar *name;

	if (dp!=NULL) {
		if (settings_style_list) {
			gtk_list_store_clear(settings_style_list);
			g_object_unref(G_OBJECT(settings_style_list)); 
		}
		settings_style_list=gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		gtk_icon_view_set_model(GTK_ICON_VIEW(glade_xml_get_widget(gladexml, "flo_preview")),
			GTK_TREE_MODEL(settings_style_list));
		while ((ep=readdir(dp))) {
			if (ep->d_name[0]!='.') {
				name=g_strdup_printf(DATADIR "/styles/%s", ep->d_name);
		        	layout=layoutreader_new(name);
				layoutreader_readinfos(layout, NULL);
				style=style_new(layout, name);
				pixbuf=style_pixbuf_draw(style);
				if (!pixbuf) flo_error(_("Unable to create preview for style %s"), name);
				else {
					gtk_list_store_append(settings_style_list, &iter);
					gtk_list_store_set(settings_style_list, &iter, 0, pixbuf, 1, ep->d_name, -1);
				}
				if (layout) layoutreader_free(layout);
				if (style) style_free(style);
				g_free(name);
				gdk_pixbuf_unref(pixbuf); 
			}
		}
		closedir (dp);
	} else flo_error(_("Couldn't open directory %s"), DATADIR "/styles");
}

GdkColor *settings_convert_color(gchar *strcolor)
{
	static GdkColor ret;
	sscanf(strcolor, "#%02x%02x%02x",
		(unsigned int *)&ret.red,
		(unsigned int *)&ret.green,
		(unsigned int *)&ret.blue);
	ret.red<<=8; ret.green<<=8; ret.blue<<=8;
	return &ret;
}

void settings_color_change(GtkColorButton *button, char *key)
{
	GdkColor color;
	gchar strcolor[8];
	static char string_buffer[32];
	char *fullpath;

	strcpy(string_buffer, "colours/");
	strcat(string_buffer, key);
	gtk_color_button_get_color(button, &color);
	g_sprintf(strcolor, "#%02X%02X%02X", (color.red)>>8, (color.green)>>8, (color.blue)>>8);
	fullpath=settings_get_full_path(string_buffer);
	settings_save(fullpath);
	gconf_client_set_string(gconfclient, fullpath, strcolor, NULL);
}

/* update the window according to gconf */
void settings_update()
{
	gchar **extstrs, **extstr;
	gboolean arrows=FALSE, numpad=FALSE, function_keys=FALSE;
	gchar *color;
	guint searchidx=0;

	while (settings_nametable[searchidx][0]) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml,
				settings_nametable[searchidx][0])),
			settings_get_bool(settings_nametable[searchidx][1]));
			searchidx++;
	}

	color=settings_get_string("colours/key");
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_keys")),
		settings_convert_color(color));
	if (color) g_free(color);
	color=settings_get_string("colours/label");
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_labels")),
		settings_convert_color(color));
	if (color) g_free(color);
	color=settings_get_string("colours/activated");
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_activated")),
		settings_convert_color(color));
	if (color) g_free(color);
	color=settings_get_string("colours/mouseover");
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_mouseover")),
		settings_convert_color(color));
	if (color) g_free(color);
	color=settings_get_string("colours/key");
	gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(gladexml, "flo_auto_click")),
		settings_get_double("behaviour/auto_click"));

	color=settings_get_string("layout/extensions");
	extstrs=extstr=g_strsplit(color, ":", -1);
	while (extstr && *extstr) {
		if (!strcmp(*extstr, "Arrows")) arrows=TRUE;
		if (!strcmp(*extstr, "Numpad")) numpad=TRUE;
		if (!strcmp(*extstr, "Function keys")) function_keys=TRUE;
		extstr++;
	}
	g_strfreev(extstrs);
	if (color) g_free(color);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_arrows")), arrows);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_numpad")), numpad);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_function_keys")),
			function_keys);
}

/*************/
/* callbacks */
/*************/

void settings_help(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
#ifdef ENABLE_HELP
	if (event->keyval==GDK_F1) {
		if (!gnome_help_display_uri("ghelp:florence?config", NULL)) {
			flo_error(_("Unable to open %s"), "ghelp:florence?config");
		}
	}
#endif
}

/* Called when a new style is selected */
void settings_style_change (GtkIconView *iconview, gpointer user_data) 
{
	gchar *path;
	gchar *name;
	GtkTreeIter iter;
	GList *list=gtk_icon_view_get_selected_items(iconview);
	if (list) {
		gtk_tree_model_get_iter(gtk_icon_view_get_model(iconview), &iter, (GtkTreePath *)list->data);
		gtk_tree_model_get(gtk_icon_view_get_model(iconview), &iter, 1, &name, -1);
		path=g_strdup_printf(DATADIR "/styles/%s", name);
		gconf_change_set_set_string(gconfchangeset, settings_get_full_path("layout/style"), path);
		g_list_foreach(list, (GFunc)(gtk_tree_path_free), NULL);
		g_list_free(list);
		g_free(path);
	}
}

void settings_keys_color(GtkColorButton *button)
{
	settings_color_change(button, "key");
	/* update style preview */
	settings_preview_build();
}

void settings_mouseover_color(GtkColorButton *button)
{
	settings_color_change(button, "mouseover");
}

void settings_activated_color(GtkColorButton *button)
{
	settings_color_change(button, "activated");
}

void settings_labels_color(GtkColorButton *button)
{
	settings_color_change(button, "label");
}

void settings_extension(GtkToggleButton *button, gchar *name)
{
        gchar *allextstr=NULL;
        gchar **extstrs=NULL;
        gchar **extstr=NULL;
        gchar **newextstrs=NULL;
        gchar **newextstr=NULL;
	GConfValue *value=NULL;

	/* Get this from change set in case it's not commited */
	if (gconf_change_set_check_value(gconfchangeset, settings_get_full_path("layout/extensions"), &value)) {
		allextstr=(gchar *)gconf_value_get_string(value);
	} else allextstr=settings_get_string("layout/extensions");
	
	if (allextstr) {
                extstrs=g_strsplit(allextstr, ":", -1);
                extstr=extstrs;
		newextstrs=g_malloc(sizeof(gchar *)*(2+g_strv_length(extstrs)));
               	newextstr=newextstrs;
		while (extstr && *extstr) { if (strcmp(*extstr, name)) *(newextstr++)=*(extstr++); else extstr++; }
		if (gtk_toggle_button_get_active(button)) {
			*(newextstr++)=name;
		}
		*newextstr=NULL;
		gconf_change_set_set_string(gconfchangeset, settings_get_full_path("layout/extensions"),
			g_strjoinv(":", newextstrs));
                g_strfreev(extstrs);
                g_free(newextstrs);
		if (!value) g_free(allextstr);
        } else { flo_fatal(_("Can't get gconf value %"), settings_get_full_path("layout/extensions")); }
}

void settings_arrows(GtkToggleButton *button)
{
	settings_extension(button, "Arrows");
}

void settings_numpad(GtkToggleButton *button)
{
	settings_extension(button, "Numpad");
}

void settings_function_keys(GtkToggleButton *button)
{
	settings_extension(button, "Function keys");
}

void settings_auto_click(GtkHScale *scale)
{
	gconf_change_set_set_float(gconfchangeset, settings_get_full_path("behaviour/auto_click"),
		gtk_range_get_value(GTK_RANGE(scale)));
}

/* Set a gconf boolean according to the state of the toggle button.
 * Look for the gconf parameter name in the name table */
void settings_set_bool (GtkToggleButton *button)
{
	gconf_change_set_set_bool(gconfchangeset,
		settings_get_full_path(settings_get_gconf_name(
			GTK_WIDGET(button))),
		gtk_toggle_button_get_active(button));
}

void settings_commit(GtkWidget *window, GtkWidget *button)
{
	gconf_client_commit_change_set(gconfclient, gconfchangeset, TRUE, NULL);
	if (rollback) gconf_change_set_clear(rollback);
}

void settings_rollback(GtkWidget *window, GtkWidget *button)
{
	GConfValue *value;
	gboolean color_changed=gconf_change_set_check_value(rollback,
		settings_get_full_path("colours/key"), &value);
	if (gconfchangeset) {
		if (rollback) gconf_client_commit_change_set(gconfclient, rollback, TRUE, NULL);
		gconf_change_set_clear(gconfchangeset);
	}
	if (color_changed) {
		if (window) settings_preview_build();
	}
}

void settings_close(GtkWidget *window, GtkWidget *button)
{
	static gboolean closed=FALSE;
	if (closed) return;

	gconf_client_remove_dir(gconfclient, FLO_SETTINGS_ROOT, NULL);
	if (settings_notify_id>0) gconf_client_notify_remove(gconfclient, settings_notify_id);
	settings_notify_id=0;
	if ((gconfchangeset && gconf_change_set_size(gconfchangeset)>0)||
		(rollback && gconf_change_set_size(rollback)>0)) {
		if (GTK_RESPONSE_ACCEPT==tools_dialog(_("Corfirm"), GTK_WINDOW(window),
			GTK_STOCK_APPLY, FLO_SETTINGS_ICON_CANCEL, _("Discard changes?"))) {
			settings_commit(window, button);
		} else settings_rollback(NULL, NULL);
	}

	closed=TRUE;
	if (gconfchangeset) gconf_change_set_unref(gconfchangeset);
	if (rollback) gconf_change_set_unref(rollback);
	if (window) gtk_object_destroy(GTK_OBJECT(window));
	if (settings_style_list) g_object_unref(G_OBJECT(settings_style_list)); 
	settings_style_list=NULL;
	if (settings_gtk_exit) gtk_exit(0);
	closed=FALSE;
	gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

void settings_destroy(GtkWidget *window)
{
	if (settings_gtk_exit) gtk_exit(0);
}

/********************/
/* public functions */
/********************/

void settings_init(gboolean exit)
{
	settings_gtk_exit=exit;
	gconfclient=gconf_client_get_default();
	gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	glade_init();
}

void settings_exit(void)
{
	g_object_unref(G_OBJECT(gconfclient));
}

void settings_changecb_register(gchar *name, GConfClientNotifyFunc cb, gpointer user_data)
{
	gconf_client_notify_add(gconfclient, settings_get_full_path(name), cb, user_data, NULL, NULL);
}

gdouble settings_get_double(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	gdouble ret=gconf_client_get_float(gconfclient, fullpath, &err);
	if (err) flo_fatal (_("Incorrect gconf value for %s"), fullpath);
	flo_debug("GCONF:%s=<%f>", fullpath, ret);
	return ret;
}

void settings_set_double(const gchar *name, gdouble value)
{
	gconf_client_remove_dir(gconfclient, FLO_SETTINGS_ROOT, NULL);
	gconf_client_set_float(gconfclient, settings_get_full_path(name), value, NULL);
	gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

gchar *settings_get_string(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	gchar *ret=gconf_client_get_string(gconfclient, fullpath, &err);
	if (err) flo_fatal (_("Incorrect gconf value for %s"), fullpath);
	flo_debug("GCONF:%s=<%s>", fullpath, ret);
	return ret;
}

gboolean settings_get_bool(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	gboolean ret=gconf_client_get_bool(gconfclient, fullpath, &err);
	if (err) flo_fatal (_("Incorrect gconf value for %s"), fullpath);
	flo_debug("GCONF:%s=<%s>", fullpath, ret?"TRUE":"FALSE");
	return ret;
}

/* Create the $HOME/.florence directory */
gboolean settings_mkhomedir()
{ 
	gchar *filename=g_strdup_printf("%s/.florence", g_getenv("HOME"));
	struct stat stat;
	gboolean ret=TRUE;

	/* create the directory if it doesn't exist already */
	if (lstat(filename, &stat)==0) {
		if (!S_ISDIR(stat.st_mode)) {
			flo_warn(_("%s is not a directory"), filename);
			ret=FALSE;
		}
	} else {
		if (0!=mkdir(filename, S_IRUSR|S_IWUSR|S_IXUSR)) {
			flo_warn(_("Unable to create directory %s"), filename);
			ret=FALSE;
		}
	}
	g_free(filename);

	return ret;
}

/* Displays the settings dialog box on the screen and register events */
void settings(void)
{
	GtkWidget *dialog;
	if (settings_notify_id>0) {
		dialog=gtk_message_dialog_new(NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,	GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Another instance of Florence settings dialog is already running"));
		gtk_dialog_run(GTK_DIALOG (dialog));
		gtk_widget_destroy(dialog);
		return;
	}
	gconfchangeset=gconf_change_set_new();
	rollback=gconf_change_set_new();
	gladexml=glade_xml_new(DATADIR "/florence.glade", NULL, NULL);
	settings_update();
	settings_preview_build();
	settings_notify_id=gconf_client_notify_add(gconfclient, FLO_SETTINGS_ROOT,
		(GConfClientNotifyFunc)settings_update, NULL, NULL, NULL);
	glade_xml_signal_autoconnect(gladexml);

	/* set window icon */
	tools_set_icon(GTK_WINDOW(glade_xml_get_widget(gladexml, "flo_config_window")));
}

