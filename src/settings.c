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

#include <gconf/gconf-client.h>
#include <glade/glade.h>
#include <gtk/gtk.h>

#define FLO_SETTINGS_ROOT "/apps/florence"

GConfClient *gconfclient;
GConfChangeSet *gconfchangeset;

/* private functions */
GdkColor *convert_color(gchar *strcolor)
{
	static GdkColor ret;
	sscanf(strcolor, "#%2x%2x%2x", ret.red, ret.green, ret.blue);
	return &ret;
}

char *settings_get_full_path(char *path)
{
	static char string_buffer[1024];
	strcpy(string_buffer, FLO_SETTINGS_ROOT);
	strcat(string_buffer, "/");
	strcat(string_buffer, path);
	return string_buffer;
}

/* callbacks */
void settings_keys_color(GtkColorButton *button)
{
	GdkColor *color;
	gchar strcolor[8];
	gtk_color_button_get_color(button, color);
	sprintf(strcolor, "#%2x%2x%2x", color->red>>8, color->green>>8, color->blue>>8);
	flo_info("color=%s", strcolor);
	gconf_change_set_set_string(gconfchangeset, settings_get_full_path("colours/key"), strcolor);
}

void settings_always_on(GtkToggleButton *button)
{
	gconf_change_set_set_bool(gconfchangeset, settings_get_full_path("behaviour/always_on_screen"),
		gtk_toggle_button_get_active(button));
}

void settings_rollback(GtkWidget *window, GtkWidget *button)
{
	/* TODO :here a confirmation window */
	settings_close(window, button);
}

void settings_commit(GtkWidget *window, GtkWidget *button)
{
	gconf_engine_commit_change_set(gconf_engine_get_default(), gconfchangeset, TRUE, NULL);
}

void settings_close(GtkWidget *window, gpointer *button)
{
	gconf_change_set_unref(gconfchangeset);
	gtk_object_destroy(GTK_OBJECT(window));
}

/* public functions */
void settings_init(void)
{
	gconfclient=gconf_client_get_default();
	gconf_client_add_dir(gconfclient, FLO_SETTINGS_ROOT, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	glade_init();
}

void settings_exit(void)
{
	g_object_unref(gconfclient);
}

void settings_changecb_register(gchar *name, GCallback cb)
{
	gconf_client_notify_add(gconfclient, settings_get_full_path(name), cb, NULL, NULL, NULL);
}

GConfValue *settings_get_value(gchar *name)
{
	return gconf_client_get(gconfclient, settings_get_full_path(name), NULL);
}

void settings(void)
{
	GladeXML *gladexml;
	gconfchangeset=gconf_change_set_new();
	gladexml=glade_xml_new(DATADIR "/florence.glade", NULL, NULL);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(glade_xml_get_widget(gladexml, "flo_keys")),
		convert_color(gconf_value_get_string(settings_get_value("colours/key"))));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(gladexml, "flo_always_on")),
		gconf_value_get_bool(settings_get_value("behaviour/always_on_screen")));

	glade_xml_signal_autoconnect(gladexml);
}

