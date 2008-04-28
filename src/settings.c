/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008 François Agrech

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

#include <glade/glade.h>
#include "system.h"
#include "settings.h"

#define FLO_SETTINGS_ROOT "/apps/florence"

GConfClient *gconfclient=NULL;
GConfChangeSet *gconfchangeset=NULL;
gboolean settings_gtk_exit=FALSE;

/* private functions */
char *settings_get_full_path(const char *path)
{
	static char string_buffer[64];
	strcpy(string_buffer, FLO_SETTINGS_ROOT);
	strcat(string_buffer, "/");
	strcat(string_buffer, path);
	return string_buffer;
}

GdkColor *settings_convert_color(gchar *strcolor)
{
	static GdkColor ret;
	sscanf(strcolor, "#%02x%02x%02x", &ret.red, &ret.green, &ret.blue);
	ret.red<<=8; ret.green<<=8; ret.blue<<=8;
	return &ret;
}

void settings_color_change(GtkColorButton *button, char *key)
{
	GdkColor color;
	gchar strcolor[8];
	static char string_buffer[32];

	strcpy(string_buffer, "colours/");
	strcat(string_buffer, key);
	gtk_color_button_get_color(button, &color);
	sprintf(strcolor, "#%02X%02X%02X", (color.red)>>8, (color.green)>>8, (color.blue)>>8);
	gconf_change_set_set_string(gconfchangeset, settings_get_full_path(string_buffer), strcolor);
}

/* callbacks */
void settings_keys_color(GtkColorButton *button)
{
	settings_color_change(button, "key");
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

void settings_always_on(GtkToggleButton *button)
{
	gboolean always_on_screen=gtk_toggle_button_get_active(button);
	gconf_change_set_set_bool(gconfchangeset, settings_get_full_path("window/decorated"),
		always_on_screen);
	gconf_change_set_set_bool(gconfchangeset, settings_get_full_path("window/shaped"),
		!always_on_screen);
	gconf_change_set_set_bool(gconfchangeset, settings_get_full_path("behaviour/always_on_screen"),
		always_on_screen);
}

void settings_extension(GtkToggleButton *button, gchar *name)
{
        gchar *allextstr=NULL;
        gchar **extstrs=NULL;
        gchar **extstr=NULL;
        gchar **newextstrs=NULL;
        gchar **newextstr=NULL;
	GConfValue *value;

	/* Get this from change set in case it's not commited */
	if (gconf_change_set_check_value(gconfchangeset, settings_get_full_path("layout/extensions"), &value)) {
		allextstr=(gchar *)gconf_value_get_string(value);
	} else value=NULL;
	
	if (value || (allextstr=settings_get_string("layout/extensions"))) {
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

void settings_zoom(GtkHScale *scale)
{
	gconf_change_set_set_float(gconfchangeset, settings_get_full_path("window/zoom"),
		gtk_range_get_value(GTK_RANGE(scale)));
}

void settings_auto_click(GtkHScale *scale)
{
	gconf_change_set_set_float(gconfchangeset, settings_get_full_path("behaviour/auto_click"),
		gtk_range_get_value(GTK_RANGE(scale)));
}

void settings_commit(GtkWidget *window, GtkWidget *button)
{
	gconf_client_commit_change_set(gconfclient, gconfchangeset, TRUE, NULL);
}

void settings_rollback(GtkWidget *window, GtkWidget *button)
{
	if (gconfchangeset) {
		gconf_change_set_clear(gconfchangeset);
		gconf_change_set_unref(gconfchangeset);
	}
	if (window) gtk_object_destroy(GTK_OBJECT(window));
	if (settings_gtk_exit) gtk_exit(0);
}

void settings_close(GtkWidget *window, GtkWidget *button)
{
	GtkWidget *dialog, *label;
	gint result;
	if (gconf_change_set_size(gconfchangeset)) {
		dialog=gtk_dialog_new_with_buttons("Corfirm", GTK_WINDOW(window),
			GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_APPLY,
			GTK_RESPONSE_ACCEPT, GTK_STOCK_DISCARD, GTK_RESPONSE_REJECT, NULL);
		label=gtk_label_new(_("Discard changes?"));
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
		gtk_widget_show_all(dialog);
		result=gtk_dialog_run(GTK_DIALOG(dialog));
		if (result==GTK_RESPONSE_ACCEPT) {
			settings_commit(window, button);
		}
	}
	settings_rollback(window, button);
}

void settings_destroy(GtkWidget *window)
{
	if (settings_gtk_exit) gtk_exit(0);
}

/* public functions */
void settings_init(gboolean exit)
{
	settings_gtk_exit=exit;
	gconfclient=gconf_client_get_default();
	gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	glade_init();
}

void settings_exit(void)
{
	g_object_unref(gconfclient);
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

void settings(void)
{
	GladeXML *gladexml;
	gconfchangeset=gconf_change_set_new();
	gladexml=glade_xml_new(DATADIR "/florence.glade", NULL, NULL);
	gchar **extstrs, **extstr;
	gboolean arrows=FALSE, numpad=FALSE, function_keys=FALSE;

	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_keys")),
		settings_convert_color(settings_get_string("colours/key")));
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_labels")),
		settings_convert_color(settings_get_string("colours/label")));
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_activated")),
		settings_convert_color(settings_get_string("colours/activated")));
	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_mouseover")),
		settings_convert_color(settings_get_string("colours/mouseover")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_always_on")),
		settings_get_bool("behaviour/always_on_screen"));
	gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(gladexml, "flo_zoom")),
		settings_get_double("window/zoom"));
	gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(gladexml, "flo_auto_click")),
		settings_get_double("behaviour/auto_click"));

	extstrs=extstr=g_strsplit(settings_get_string("layout/extensions"), ":", -1);
	while (extstr && *extstr) {
		if (!strcmp(*extstr, "Arrows")) arrows=TRUE;
		if (!strcmp(*extstr, "Numpad")) numpad=TRUE;
		if (!strcmp(*extstr, "Function keys")) function_keys=TRUE;
		extstr++;
	}
	g_strfreev(extstrs);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_arrows")), arrows);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_numpad")), numpad);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_function_keys")),
		function_keys);

	glade_xml_signal_autoconnect(gladexml);
}

