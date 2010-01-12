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

#include "trayicon.h"
#include "system.h"
#include "trace.h"
#include "settings.h"
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef ENABLE_HELP
#if !GTK_CHECK_VERSION(2,14,0)
	#include <libgnome/gnome-help.h>
#endif
#endif

/* Display the about dialog window */
void trayicon_about(void)
{
	gtk_show_about_dialog(NULL, "program-name", _("Florence Virtual Keyboard"),
		"version", VERSION, "copyright", _("Copyright (C) 2008, 2009 François Agrech"),
		"logo", gdk_pixbuf_new_from_file(ICONDIR "/florence.svg", NULL),
		"website", "http://florence.sourceforge.net",
		"license", _("Copyright (C) 2008, 2009 François Agrech\n\
\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software Foundation,\n\
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA."),
		NULL);
}

#ifdef ENABLE_HELP
/* Open yelp */
void trayicon_help(void)
{
#if GTK_CHECK_VERSION(2,14,0)
	GError *error=NULL;
	gtk_show_uri(NULL, "ghelp:florence", gtk_get_current_event_time(), &error);
	if (error) flo_error(_("Unable to open %s"), "ghelp:florence");
#else
	if (!gnome_help_display_uri("ghelp:florence", NULL)) {
		flo_error(_("Unable to open %s"), "ghelp:florence");
	}
#endif
}
#endif

/* Called when the tray icon is left-clicked
 * Toggles florence window between visible and hidden. */
void trayicon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
	struct trayicon *trayicon=(struct trayicon *)(user_data);
	gtk_window_deiconify(GTK_WINDOW(trayicon->window));
	if (GTK_WIDGET_VISIBLE(trayicon->window)) {
		gtk_window_get_position(GTK_WINDOW(trayicon->window), &(trayicon->x), &(trayicon->y));
		gtk_widget_hide(trayicon->window);
	} else { 
		gtk_window_move(GTK_WINDOW(trayicon->window), trayicon->x, trayicon->y);
		gtk_window_present(GTK_WINDOW(trayicon->window));
	}
}

/* Called when the tray icon is right->clicked
 * Displays the menu. */
void trayicon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu, *about, *config, *quit;
#ifdef ENABLE_HELP
	GtkWidget *help;
#endif
 
	struct trayicon *trayicon=(struct trayicon *)(user_data);
	menu=gtk_menu_new();

	quit=gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(quit),
		gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(quit, "activate", trayicon->trayicon_quit, NULL);

#ifdef ENABLE_HELP
	help=gtk_image_menu_item_new_with_mnemonic(_("_Help"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(help),
		gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
	g_signal_connect(help, "activate", G_CALLBACK(trayicon_help), NULL);
#endif

	about=gtk_image_menu_item_new_with_mnemonic(_("_About"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(about),
		gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));
	g_signal_connect(about, "activate", G_CALLBACK(trayicon_about), NULL);

	config=gtk_image_menu_item_new_with_mnemonic(_("_Preferences"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(config),
		gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
	g_signal_connect(config, "activate", G_CALLBACK(settings), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), config);
#ifdef ENABLE_HELP
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), help);
#endif
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), about);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);
	gtk_widget_show_all(menu);
 
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu,
		status_icon, button, activate_time);
}

#ifdef ENABLE_NOTIFICATION
/* Called to stop showing startup notification. */
void trayicon_notification_stop(NotifyNotification *notification, gchar *action, gpointer userdate)
{
	if (!strcmp(action, "STOP"))
		settings_bool_set("behaviour/startup_notification", FALSE);
}

/* Display startup notification */
gboolean trayicon_notification_start(gpointer userdata)
{
	struct trayicon *trayicon=(struct trayicon *)userdata;
	if (!notify_init(_("Florence"))) flo_warn(_("libnotify failed to initialize"));
	trayicon->notification=notify_notification_new_with_status_icon(
		_("Florence is running"),
		_("Click on Florence smile to show/hide Florence.\n"
		"Right click on it to display menu and get help."),
		GTK_STOCK_INFO, trayicon->tray_icon);
	notify_notification_add_action(trayicon->notification, "STOP",
		_("Do not show again"), trayicon_notification_stop, NULL, NULL);
	notify_notification_set_timeout(trayicon->notification, 5000);
	if (!notify_notification_show(trayicon->notification, NULL))
		flo_warn(_("Notification failed"));
	return FALSE;
}
#endif

/* Deallocate all the memory used bu the trayicon. */
void trayicon_free(struct trayicon *trayicon)
{
#ifdef ENABLE_NOTIFICATION
	if (trayicon->notification) g_object_unref(trayicon->notification);
	notify_uninit();
#endif
	g_object_unref(trayicon->tray_icon);
	g_free(trayicon);
}

/* Creates a new trayicon instance */
struct trayicon *trayicon_new(GtkWidget *window, GCallback quit_cb)
{
	struct trayicon *trayicon;

	trayicon=g_malloc(sizeof(struct trayicon));
	memset(trayicon, 0, sizeof(struct trayicon));

	trayicon->trayicon_quit=quit_cb;
	trayicon->tray_icon=gtk_status_icon_new();
	trayicon->window=window;
	g_signal_connect(G_OBJECT(trayicon->tray_icon), "activate",
		G_CALLBACK(trayicon_on_click), (gpointer)trayicon);
	g_signal_connect(G_OBJECT(trayicon->tray_icon), "popup-menu",
		G_CALLBACK(trayicon_on_menu), (gpointer)trayicon);
	gtk_status_icon_set_from_icon_name(trayicon->tray_icon, "florence");
	gtk_status_icon_set_tooltip(trayicon->tray_icon, _("Florence Virtual Keyboard"));
	gtk_status_icon_set_visible(trayicon->tray_icon, TRUE);

#ifdef ENABLE_NOTIFICATION
	if (settings_get_bool("behaviour/startup_notification"))
		g_timeout_add(2000, trayicon_notification_start, (gpointer)trayicon);
#endif

	return trayicon;
}
