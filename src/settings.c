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

struct settings_boolean {
	gchar *glade_name;
	gchar *gconf_name;
	gboolean default_value;
};

/* settings booleans. { "glade name", "gconf name", "default value" } */
struct settings_boolean settings_nametable[] = {
	{ "flo_resizable", "window/resizable", TRUE },
	{ "flo_keep_ratio", "window/keep_ratio", FALSE },
	{ "flo_decorated", "window/decorated", TRUE },
	{ "flo_auto_hide", "behaviour/auto_hide", FALSE },
	{ "flo_move_to_widget", "behaviour/move_to_widget", TRUE },
	{ "flo_intermediate_icon", "behaviour/intermediate_icon", TRUE },
	{ "flo_transparent", "window/transparent", FALSE },
	{ "flo_task_bar", "window/task_bar", FALSE },
	{ "flo_always_on_top", "window/always_on_top", TRUE },
	{ NULL } };

static GladeXML *gladexml;
static GConfClient *gconfclient=NULL;
static GConfChangeSet *gconfchangeset=NULL;
static GConfChangeSet *rollback=NULL;
static gboolean settings_gtk_exit=FALSE;
static GtkListStore *settings_style_list=NULL;
static GtkListStore *settings_layout_list=NULL;
static guint settings_notify_id=0;

void settings_extension(GtkToggleButton *button, gchar *name);

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
	while (settings_nametable[searchidx].glade_name &&
		strcmp(glade_get_widget_name(widget),
		settings_nametable[searchidx].glade_name)) {
		searchidx++;
	}
	return settings_nametable[searchidx].gconf_name;
}

/* Populate layout combobox with available layouts */
void settings_layouts_populate()
{
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	DIR *dp=opendir(DATADIR "/layouts");
	struct dirent *ep;
	gchar *name;
	struct layout *layout;
	struct layout_infos *infos;

	if (dp!=NULL) {
		if (settings_layout_list) {
			gtk_list_store_clear(settings_layout_list);
			g_object_unref(G_OBJECT(settings_layout_list)); 
		}
		settings_layout_list=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		cell=gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(
			GTK_CELL_LAYOUT(glade_xml_get_widget(gladexml, "flo_layouts")),
			cell, FALSE);
		gtk_cell_layout_set_attributes(
			GTK_CELL_LAYOUT(glade_xml_get_widget(gladexml, "flo_layouts")),
			cell, "text", 0, NULL);
		while ((ep=readdir(dp))) {
			if (ep->d_name[0]!='.') {
				name=g_strdup_printf(DATADIR "/layouts/%s", ep->d_name);
				layout=layoutreader_new(name, NULL, DATADIR "/relaxng/florence.rng");
				layoutreader_element_open(layout, "layout");
				infos=layoutreader_infos_new(layout);
				gtk_list_store_append(settings_layout_list, &iter);
				gtk_list_store_set(settings_layout_list, &iter, 0, infos->name, 1, name, -1);
				layoutreader_infos_free(infos);
				layoutreader_free(layout);
				g_free(name);
			}
		}
		closedir(dp);
		gtk_combo_box_set_model(GTK_COMBO_BOX(glade_xml_get_widget(gladexml, "flo_layouts")),
			GTK_TREE_MODEL(settings_layout_list));
	} else flo_error(_("Couldn't open directory %s"), DATADIR "/layouts");
}

/* Fills the preview icon view with icons representing the themes */
void settings_preview_build()
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
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
				style=style_new(name);
				pixbuf=style_pixbuf_draw(style);
				if (!pixbuf) flo_error(_("Unable to create preview for style %s"), name);
				else {
					gtk_list_store_append(settings_style_list, &iter);
					gtk_list_store_set(settings_style_list, &iter, 0, pixbuf, 1, ep->d_name, -1);
				}
				if (style) style_free(style);
				g_free(name);
				gdk_pixbuf_unref(pixbuf); 
			}
		}
		closedir(dp);
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

/* update the extension check box list according to the layout and gconf */
void settings_extensions_update(gchar *layoutname)
{
	GList *extensions;
	GtkWidget *extension;
	struct layout *layout;
	struct layout_extension *ext=NULL;
	GtkWidget *new;
	gchar **extstrs, **extstr;
	gchar *id;
	gchar *temp;

	extensions=gtk_container_get_children(
		GTK_CONTAINER(glade_xml_get_widget(gladexml, "flo_extensions")));
	while (extensions) {
		extension=(GTK_WIDGET(extensions->data));
		extensions=extensions->next;
		/* TODO: g_free(id) */
		gtk_widget_destroy(extension);
	}
	layout=layoutreader_new(layoutname, NULL, DATADIR "/relaxng/florence.rng");
	layoutreader_element_open(layout, "layout");
	while ((ext=layoutreader_extension_new(layout))) {
		new=gtk_check_button_new_with_label(ext->name);
		id=g_strdup(ext->identifiant);
		temp=settings_get_string("layout/extensions");
		extstrs=extstr=g_strsplit(temp, ":", -1);
		while (extstr && *extstr && strcmp(*extstr, id)) {
			extstr++;
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new), extstr && *extstr);
		g_strfreev(extstrs);
		if (temp) g_free(temp);
		g_signal_connect(G_OBJECT(new), "toggled",
			G_CALLBACK(settings_extension), id);
		gtk_box_pack_start(
			GTK_BOX(glade_xml_get_widget(gladexml, "flo_extensions")),
			new, FALSE, FALSE, 0);
		gtk_widget_show(new);
		layoutreader_extension_free(layout, ext);
	}
	layoutreader_free(layout);
}

/* update the layout settings according to gconf */
void settings_layout_update()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean out;
	gchar *layout;

	/* update the layout combo box */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(glade_xml_get_widget(gladexml, "flo_layouts")));
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 1, &layout, -1);
			if ((out=(!strcmp(layout, settings_get_string("layout/file")))))
				gtk_combo_box_set_active_iter(
					GTK_COMBO_BOX(glade_xml_get_widget(gladexml, "flo_layouts")),
					&iter);
				settings_extensions_update(layout);
			if (layout) g_free(layout);
		} while ((!out) && gtk_tree_model_iter_next(model, &iter));
	}
}

/* update the window according to gconf */
void settings_update()
{
	gchar *color;
	guint searchidx=0;

	while (settings_nametable[searchidx].glade_name) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml,
				settings_nametable[searchidx].glade_name)),
			settings_get_bool(settings_nametable[searchidx].gconf_name));
			searchidx++;
	}
	gtk_widget_set_sensitive(glade_xml_get_widget(gladexml,
		"flo_move_to_widget"), settings_get_bool("behaviour/auto_hide"));
	gtk_widget_set_sensitive(glade_xml_get_widget(gladexml,
		"flo_intermediate_icon"), settings_get_bool("behaviour/auto_hide"));

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
	gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(gladexml, "flo_opacity")),
		settings_get_double("window/opacity"));

	settings_layout_update();
}

/*************/
/* callbacks */
/*************/

void settings_help(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
#ifdef ENABLE_HELP
	GError *error=NULL;
	if (event->keyval==GDK_F1) {
		gtk_show_uri(NULL, "ghelp:florence?config", gtk_get_current_event_time(), &error);
		if (error) flo_error(_("Unable to open %s"), "ghelp:florence?config");
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
		settings_save(settings_get_full_path("layout/style"));
		gconf_client_set_string(gconfclient, settings_get_full_path("layout/style"), path, NULL);
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

/* called on layout change: set gconf layout entry. */
void settings_layout(GtkComboBox *combo)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *layout;

	gtk_combo_box_get_active_iter(combo, &iter);
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(glade_xml_get_widget(gladexml, "flo_layouts")));
	gtk_tree_model_get(model, &iter, 1, &layout, -1);
	gconf_change_set_set_string(gconfchangeset, settings_get_full_path("layout/file"),
		layout);
	settings_extensions_update(layout);
	g_free(layout);
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
	
	/* TODO: delete unknown extensions */
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

void settings_auto_click(GtkHScale *scale)
{
	gconf_change_set_set_float(gconfchangeset, settings_get_full_path("behaviour/auto_click"),
		gtk_range_get_value(GTK_RANGE(scale)));
}

void settings_opacity(GtkHScale *scale)
{
	settings_save(settings_get_full_path("window/opacity"));
	gconf_client_set_float(gconfclient, settings_get_full_path("window/opacity"),
		gtk_range_get_value(GTK_RANGE(scale)), NULL);
}

/* Set a gconf boolean according to the state of the toggle button.
 * Look for the gconf parameter name in the name table */
void settings_set_bool (GtkToggleButton *button)
{
	gconf_change_set_set_bool(gconfchangeset,
		settings_get_full_path(settings_get_gconf_name(
			GTK_WIDGET(button))),
		gtk_toggle_button_get_active(button));
	if (!strcmp(glade_get_widget_name(GTK_WIDGET(button)), "flo_auto_hide")) {
		gtk_widget_set_sensitive(glade_xml_get_widget(gladexml,
			"flo_move_to_widget"), gtk_toggle_button_get_active(button));
		gtk_widget_set_sensitive(glade_xml_get_widget(gladexml,
			"flo_intermediate_icon"), gtk_toggle_button_get_active(button));
	}
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

/* Called when the 'close' button is pressed:
 * check changes and ask the user if they need commit or discard if any. */
void settings_close(GtkWidget *window, GtkWidget *button)
{
	static gboolean closed=FALSE;
	if (closed) return;
	if ((gconfchangeset && gconf_change_set_size(gconfchangeset)>0)||
		(rollback && gconf_change_set_size(rollback)>0)) {
		if (GTK_RESPONSE_ACCEPT==tools_dialog(_("Corfirm"), GTK_WINDOW(window),
			GTK_STOCK_APPLY, FLO_SETTINGS_ICON_CANCEL, _("Discard changes?")))
			settings_commit(NULL, NULL);
		else settings_rollback(NULL, NULL);
	}
	if (settings_notify_id>0) gconf_client_notify_remove(gconfclient, settings_notify_id);
	settings_notify_id=0;

	closed=TRUE;
	if (settings_style_list) g_object_unref(G_OBJECT(settings_style_list)); 
	settings_style_list=NULL;
	if (window) gtk_object_destroy(GTK_OBJECT(window));
	if (gconfchangeset) gconf_change_set_unref(gconfchangeset);
	if (rollback) gconf_change_set_unref(rollback);
	if (settings_gtk_exit) gtk_exit(0);
	closed=FALSE;
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

/* get an integer from gconf */
gint settings_get_int(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	gint ret=gconf_client_get_int(gconfclient, fullpath, &err);
	if (err) flo_fatal (_("Incorrect gconf value for %s"), fullpath);
	flo_debug("GCONF:%s=<%d>", fullpath, ret);
	return ret;
}

/* set a gconf integer */
void settings_set_int(const gchar *name, gint value)
{
	gconf_client_remove_dir(gconfclient, FLO_SETTINGS_ROOT, NULL);
	gconf_client_set_int(gconfclient, settings_get_full_path(name), value, NULL);
	gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

/* get a double from gconf */
gdouble settings_get_double(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	gdouble ret=gconf_client_get_float(gconfclient, fullpath, &err);
	if (err) flo_fatal (_("Incorrect gconf value for %s"), fullpath);
	flo_debug("GCONF:%s=<%f>", fullpath, ret);
	return ret;
}

/* set a gconf double */
void settings_set_double(const gchar *name, gdouble value, gboolean notify)
{
	if (!notify) gconf_client_remove_dir(gconfclient, FLO_SETTINGS_ROOT, NULL);
	gconf_client_set_float(gconfclient, settings_get_full_path(name), value, NULL);
	if (!notify) gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

/* get a gconf string */
gchar *settings_get_string(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	gchar *ret=gconf_client_get_string(gconfclient, fullpath, &err);
	if (err) flo_fatal (_("Incorrect gconf value for %s"), fullpath);
	flo_debug("GCONF:%s=<%s>", fullpath, ret);
	return ret;
}

/* get a gconf boolean */
gboolean settings_get_bool(const gchar *name)
{
	GError *err=NULL;
	char *fullpath=settings_get_full_path(name);
	guint searchidx=0;
	gboolean ret=gconf_client_get_bool(gconfclient, fullpath, &err);
	if (err) {
		while (settings_nametable[searchidx].glade_name && strcmp(name,
			settings_nametable[searchidx].gconf_name)) {
			searchidx++;
		}
		ret=settings_nametable[searchidx].default_value;
		flo_error (_("Incorrect gconf value for %s. Using default value %s"),
			fullpath, ret?_("TRUE"):_("FALSE"));
	}
	flo_debug("GCONF:%s=<%s>", fullpath, ret?"TRUE":"FALSE");
	return ret;
}

/* set a gconf boolean */
void settings_bool_set(const gchar *name, gboolean value)
{
	gconf_client_set_bool(gconfclient, settings_get_full_path(name), value, NULL);
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
	settings_preview_build();
	/* populate layout combobox */
	settings_layouts_populate();
	settings_update();
	settings_notify_id=gconf_client_notify_add(gconfclient, FLO_SETTINGS_ROOT,
		(GConfClientNotifyFunc)settings_update, NULL, NULL, NULL);
	glade_xml_signal_autoconnect(gladexml);

	/* set window icon */
	tools_set_icon(GTK_WINDOW(glade_xml_get_widget(gladexml, "flo_config_window")));
}

