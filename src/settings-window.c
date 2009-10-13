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
#include "settings-window.h"

#if GTK_CHECK_VERSION(2,12,0)
#define FLO_SETTINGS_ICON_CANCEL GTK_STOCK_DISCARD
#else
#define FLO_SETTINGS_ICON_CANCEL GTK_STOCK_CANCEL
#endif

static struct settings_window *settings_window=NULL;
void settings_window_extension(GtkToggleButton *button, gchar *name);

/*********************/
/* private functions */
/*********************/

/* update rollback changeset */
void settings_window_save(const char *name)
{
	GConfValue *value;
	if (!gconf_change_set_check_value(settings_window->rollback,
			settings_get_full_path(name), &value)) {
		value=settings_value_get(name);
		gconf_change_set_set(settings_window->rollback,
			settings_get_full_path(name), value);
	}
}

/* Populate layout combobox with available layouts */
void settings_window_layouts_populate()
{
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	DIR *dp=opendir(DATADIR "/layouts");
	struct dirent *ep;
	gchar *name;
	struct layout *layout;
	struct layout_infos *infos;

	if (dp!=NULL) {
		if (settings_window->layout_list) {
			gtk_list_store_clear(settings_window->layout_list);
			g_object_unref(G_OBJECT(settings_window->layout_list)); 
		}
		settings_window->layout_list=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		cell=gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(
			GTK_CELL_LAYOUT(glade_xml_get_widget(settings_window->gladexml, "flo_layouts")),
			cell, FALSE);
		gtk_cell_layout_set_attributes(
			GTK_CELL_LAYOUT(glade_xml_get_widget(settings_window->gladexml, "flo_layouts")),
			cell, "text", 0, NULL);
		while ((ep=readdir(dp))) {
			if (ep->d_name[0]!='.') {
				name=g_strdup_printf(DATADIR "/layouts/%s", ep->d_name);
				layout=layoutreader_new(name, NULL, DATADIR "/relaxng/florence.rng");
				layoutreader_element_open(layout, "layout");
				infos=layoutreader_infos_new(layout);
				gtk_list_store_append(settings_window->layout_list, &iter);
				gtk_list_store_set(settings_window->layout_list, &iter, 0, infos->name, 1, name, -1);
				layoutreader_infos_free(infos);
				layoutreader_free(layout);
				g_free(name);
			}
		}
		closedir(dp);
		gtk_combo_box_set_model(GTK_COMBO_BOX(glade_xml_get_widget(
				settings_window->gladexml, "flo_layouts")),
			GTK_TREE_MODEL(settings_window->layout_list));
	} else flo_error(_("Couldn't open directory %s"), DATADIR "/layouts");
}

/* fills the preview icon view with icons representing the themes */
void settings_window_preview_build()
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	struct style *style;
	DIR *dp=opendir(DATADIR "/styles");
	struct dirent *ep;
	gchar *name;

	if (dp!=NULL) {
		if (settings_window->style_list) {
			gtk_list_store_clear(settings_window->style_list);
			g_object_unref(G_OBJECT(settings_window->style_list)); 
		}
		settings_window->style_list=gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		gtk_icon_view_set_model(GTK_ICON_VIEW(glade_xml_get_widget(
				settings_window->gladexml, "flo_preview")),
			GTK_TREE_MODEL(settings_window->style_list));
		while ((ep=readdir(dp))) {
			if (ep->d_name[0]!='.') {
				name=g_strdup_printf(DATADIR "/styles/%s", ep->d_name);
				style=style_new(name);
				pixbuf=style_pixbuf_draw(style);
				if (!pixbuf) flo_error(_("Unable to create preview for style %s"), name);
				else {
					gtk_list_store_append(settings_window->style_list, &iter);
					gtk_list_store_set(settings_window->style_list, &iter, 0,
						pixbuf, 1, ep->d_name, -1);
				}
				if (style) style_free(style);
				g_free(name);
				gdk_pixbuf_unref(pixbuf); 
			}
		}
		closedir(dp);
	} else flo_error(_("Couldn't open directory %s"), DATADIR "/styles");
}

/* converts a color from string ro gdk */
GdkColor *settings_window_convert_color(gchar *strcolor)
{
	static GdkColor ret;
	sscanf(strcolor, "#%02x%02x%02x",
		(unsigned int *)&ret.red,
		(unsigned int *)&ret.green,
		(unsigned int *)&ret.blue);
	ret.red<<=8; ret.green<<=8; ret.blue<<=8;
	return &ret;
}

/* update the extension check box list according to the layout and gconf */
void settings_window_extensions_update(gchar *layoutname)
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
		GTK_CONTAINER(glade_xml_get_widget(settings_window->gladexml, "flo_extensions")));
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
			G_CALLBACK(settings_window_extension), id);
		gtk_box_pack_start(
			GTK_BOX(glade_xml_get_widget(settings_window->gladexml, "flo_extensions")),
			new, FALSE, FALSE, 0);
		gtk_widget_show(new);
		layoutreader_extension_free(layout, ext);
	}
	layoutreader_free(layout);
}

/* update the layout settings according to gconf */
void settings_window_layout_update()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean out;
	gchar *layout;

	/* update the layout combo box */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(glade_xml_get_widget(
		settings_window->gladexml, "flo_layouts")));
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 1, &layout, -1);
			if ((out=(!strcmp(layout, settings_get_string("layout/file")))))
				gtk_combo_box_set_active_iter(
					GTK_COMBO_BOX(glade_xml_get_widget(
						settings_window->gladexml, "flo_layouts")),
					&iter);
				settings_window_extensions_update(layout);
			if (layout) g_free(layout);
		} while ((!out) && gtk_tree_model_iter_next(model, &iter));
	}
}

/* update the window according to gconf */
void settings_window_update()
{
	gchar *color;
	guint searchidx=0;
	struct settings_param *params=settings_defaults_get();

	while (params[searchidx].glade_name) {
		if (strcmp(params[searchidx].glade_name, SETTINGS_NONE))
			switch (params[searchidx].type) {
				case SETTINGS_BOOL:
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(glade_xml_get_widget(settings_window->gladexml,
							params[searchidx].glade_name)),
						settings_get_bool(params[searchidx].gconf_name));
					break;
				case SETTINGS_COLOR:
					color=settings_get_string(params[searchidx].gconf_name);
					gtk_color_button_set_color(
						GTK_COLOR_BUTTON(glade_xml_get_widget(settings_window->gladexml,
							params[searchidx].glade_name)),
						settings_window_convert_color(color));
					if (color) g_free(color);
					break;
				case SETTINGS_STRING:
					break;
				case SETTINGS_DOUBLE:
					gtk_range_set_value(
						GTK_RANGE(glade_xml_get_widget(settings_window->gladexml,
							params[searchidx].glade_name)),
						settings_double_get(params[searchidx].gconf_name));
					break;
				default:flo_error(_("unknown setting type: %d"),
						params[searchidx].type);
					break;
			}
		searchidx++;
	}

	gtk_widget_set_sensitive(glade_xml_get_widget(settings_window->gladexml,
		"flo_move_to_widget"), settings_get_bool("behaviour/auto_hide"));
	gtk_widget_set_sensitive(glade_xml_get_widget(settings_window->gladexml,
		"flo_intermediate_icon"), settings_get_bool("behaviour/auto_hide"));

	settings_window_layout_update();
}

/*************/
/* callbacks */
/*************/

/* opens yelp when F1 is pressed  */
void settings_window_help(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
#ifdef ENABLE_HELP
	GError *error=NULL;
	if (event->keyval==GDK_F1) {
		gtk_show_uri(NULL, "ghelp:florence?config", gtk_get_current_event_time(), &error);
		if (error) flo_error(_("Unable to open %s: %s"), "ghelp:florence?config",
			error->message);
	}
#endif
}

/* Called when a new style is selected */
void settings_window_style_change (GtkIconView *iconview, gpointer user_data) 
{
	gchar *path;
	gchar *name;
	GtkTreeIter iter;
	GList *list=gtk_icon_view_get_selected_items(iconview);
	if (list) {
		gtk_tree_model_get_iter(gtk_icon_view_get_model(iconview), &iter, (GtkTreePath *)list->data);
		gtk_tree_model_get(gtk_icon_view_get_model(iconview), &iter, 1, &name, -1);
		path=g_strdup_printf(DATADIR "/styles/%s", name);
		settings_window_save("layout/style");
		settings_string_set("layout/style", path);
		g_list_foreach(list, (GFunc)(gtk_tree_path_free), NULL);
		g_list_free(list);
		g_free(path);
	}
}

/* on color change */
void settings_window_change_color(GtkColorButton *button)
{
	GdkColor color;
	gchar strcolor[8];
	char *name=settings_get_gconf_name(GTK_WIDGET(button));
	gtk_color_button_get_color(button, &color);
	g_sprintf(strcolor, "#%02X%02X%02X", (color.red)>>8, (color.green)>>8, (color.blue)>>8);
	settings_window_save(name);
	settings_string_set(name, strcolor);
	/* update style preview */
	if (!strcmp(name, "colours/key")) settings_window_preview_build();
}

/* called on layout change: set gconf layout entry. */
void settings_window_layout(GtkComboBox *combo)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *layout;

	gtk_combo_box_get_active_iter(combo, &iter);
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(
		glade_xml_get_widget(settings_window->gladexml, "flo_layouts")));
	gtk_tree_model_get(model, &iter, 1, &layout, -1);
	gconf_change_set_set_string(settings_window->gconfchangeset,
		settings_get_full_path("layout/file"), layout);
	settings_window_extensions_update(layout);
	g_free(layout);
}

/* called when an extension is activated/deactivated. */
void settings_window_extension(GtkToggleButton *button, gchar *name)
{
        gchar *allextstr=NULL;
        gchar **extstrs=NULL;
        gchar **extstr=NULL;
        gchar **newextstrs=NULL;
        gchar **newextstr=NULL;
	GConfValue *value=NULL;

	/* Get this from change set in case it's not commited */
	if (gconf_change_set_check_value(settings_window->gconfchangeset,
		settings_get_full_path("layout/extensions"), &value)) {
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
		gconf_change_set_set_string(settings_window->gconfchangeset,
			settings_get_full_path("layout/extensions"),
			g_strjoinv(":", newextstrs));
                g_strfreev(extstrs);
                g_free(newextstrs);
		if (!value) g_free(allextstr);
        } else { flo_fatal(_("Can't get gconf value %"), settings_get_full_path("layout/extensions")); }
}

/* Set a gconf double according to the state of the toggle button.
 * Look for the gconf parameter name in the parameters table */
void settings_window_set_double(GtkHScale *scale)
{
	gconf_change_set_set_float(settings_window->gconfchangeset,
		settings_get_full_path(settings_get_gconf_name(GTK_WIDGET(scale))),
		gtk_range_get_value(GTK_RANGE(scale)));
}

/* on opacity change: apply immediately */
void settings_window_opacity(GtkHScale *scale)
{
	settings_window_save("window/opacity");
	settings_double_set("window/opacity", gtk_range_get_value(GTK_RANGE(scale)), TRUE);
}


/* set a gconf boolean according to the state of the toggle button.
 * Look for the gconf parameter name in the parameters table */
void settings_window_set_bool (GtkToggleButton *button)
{
	gconf_change_set_set_bool(settings_window->gconfchangeset,
		settings_get_full_path(settings_get_gconf_name(GTK_WIDGET(button))),
		gtk_toggle_button_get_active(button));
	if (!strcmp(glade_get_widget_name(GTK_WIDGET(button)), "flo_auto_hide")) {
		gtk_widget_set_sensitive(glade_xml_get_widget(settings_window->gladexml,
			"flo_move_to_widget"), gtk_toggle_button_get_active(button));
		gtk_widget_set_sensitive(glade_xml_get_widget(settings_window->gladexml,
			"flo_intermediate_icon"), gtk_toggle_button_get_active(button));
	}
}

/* apply changes: apply the changeset and free the rollback one. */
void settings_window_commit(GtkWidget *window, GtkWidget *button)
{
	settings_commit(settings_window->gconfchangeset);
	if (settings_window->rollback) gconf_change_set_clear(settings_window->rollback);
}

/* discard changes: apply the rollback changeset and clear it. */
void settings_window_rollback(GtkWidget *window, GtkWidget *button)
{
	GConfValue *value;
	gboolean color_changed=gconf_change_set_check_value(settings_window->rollback,
		settings_get_full_path("colours/key"), &value);
	if (settings_window->gconfchangeset) {
		if (settings_window->rollback)
			settings_commit(settings_window->rollback);
		gconf_change_set_clear(settings_window->gconfchangeset);
	}
	if (color_changed) {
		if (window) settings_window_preview_build();
	}
	settings_window_update();
}

/* Called when the 'close' button is pressed:
 * check changes and ask the user if they need commit or discard if any. */
void settings_window_close(GtkWidget *window, GtkWidget *button)
{
	static gboolean closed=FALSE;
	if (closed) return;
	if ((settings_window->gconfchangeset &&
		gconf_change_set_size(settings_window->gconfchangeset)>0)||
		(settings_window->rollback &&
		gconf_change_set_size(settings_window->rollback)>0)) {
		if (GTK_RESPONSE_ACCEPT==tools_dialog(_("Confirm"), GTK_WINDOW(window),
			GTK_STOCK_APPLY, FLO_SETTINGS_ICON_CANCEL, _("Discard changes?")))
			settings_window_commit(NULL, NULL);
		else settings_window_rollback(NULL, NULL);
	}
	if (settings_window->notify_id>0) settings_unregister(settings_window->notify_id);
	settings_window->notify_id=0;

	closed=TRUE;
	if (settings_window->style_list) g_object_unref(G_OBJECT(settings_window->style_list)); 
	settings_window->style_list=NULL;
	if (window) gtk_object_destroy(GTK_OBJECT(window));
	if (settings_window->gconfchangeset) gconf_change_set_unref(settings_window->gconfchangeset);
	if (settings_window->rollback) gconf_change_set_unref(settings_window->rollback);
	if (settings_window->gtk_exit) gtk_exit(0);
	closed=FALSE;
}

/********************/
/* public functions */
/********************/

/* returns true if settings window is open */
gboolean settings_window_open(void)
{
	return (settings_window && (settings_window->notify_id!=0));
}

/* presents the settings window to the user */
void settings_window_present(void)
{
	gtk_window_present(GTK_WINDOW(glade_xml_get_widget(settings_window->gladexml,
		"flo_config_window")));
}

/* opens the settings window */
void settings_window_new(GConfClient *gconfclient, gboolean exit)
{
	settings_window=g_malloc(sizeof(struct settings_window));
	memset(settings_window, 0, sizeof(struct settings_window));

	settings_window->gtk_exit=exit;
	settings_window->gconfclient=gconfclient;
	settings_window->gconfchangeset=gconf_change_set_new();
	settings_window->rollback=gconf_change_set_new();
	settings_window->gladexml=glade_xml_new(DATADIR "/florence.glade", NULL, NULL);

	/* populate fields*/
	settings_window_preview_build();
	settings_window_layouts_populate();

	settings_window_update();
	settings_window->notify_id=settings_register_all(
		(GConfClientNotifyFunc)settings_window_update);

	glade_xml_signal_autoconnect(settings_window->gladexml);

	/* set window icon */
	tools_set_icon(GTK_WINDOW(glade_xml_get_widget(settings_window->gladexml,
		"flo_config_window")));
}

/* liberate memory used by settings window */
void settings_window_free()
{
	if (settings_window) g_free(settings_window);
}

