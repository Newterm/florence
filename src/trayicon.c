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

#include "system.h"
#include "config.h"
#include "settings.h"
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

GCallback trayicon_quit;

void trayicon_about(void)
{
	/* TODO: the icon doesn't seem to work */
	GtkAboutDialog *about=GTK_ABOUT_DIALOG(gtk_about_dialog_new());
	gtk_show_about_dialog(NULL, "program-name", _("Florence Virtual Keyboard"),
		"version", VERSION, "copyright", _("Copyright (C) 2008 François Agrech"),
		"logo", gdk_pixbuf_new_from_file(ICONDIR "/florence.svg", NULL),
		"website", "http://florence.sourceforge.net",
		"license", _("Copyright (C) 2008 François Agrech\n\
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

void trayicon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
	static gint x=0;
	static gint y=0;
	GtkWidget *window=GTK_WIDGET(user_data);
	gtk_window_deiconify(window);
	if (GTK_WIDGET_VISIBLE(window)) {
		gtk_window_get_position(GTK_WINDOW(window), &x, &y);
		gtk_widget_hide(window);
	} else { 
		gtk_window_move(GTK_WINDOW(window), x, y);
		gtk_window_present(window);
	}
}

void trayicon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu, *about, *config, *quit;
 
	menu = gtk_menu_new();

	quit = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(quit), gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(quit, "activate", trayicon_quit, NULL);

	about = gtk_image_menu_item_new_with_mnemonic(_("_About"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(about), gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));
	g_signal_connect(about, "activate", G_CALLBACK(trayicon_about), NULL);

	config = gtk_image_menu_item_new_with_mnemonic(_("_Preferences"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(config), gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
	g_signal_connect(config, "activate", G_CALLBACK(settings), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), config);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), about);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);
	gtk_widget_show_all(menu);
 
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,gtk_status_icon_position_menu, status_icon,button, activate_time);
}

void trayicon_create(GtkWidget *window, GCallback *quit_cb)
{
	GtkStatusIcon *tray_icon;

	trayicon_quit=quit_cb;
	tray_icon = gtk_status_icon_new();
	g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(trayicon_on_click), (gpointer)window);
	g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(trayicon_on_menu), NULL);
	/* TODO: this doesn't work!!! */
	gtk_status_icon_set_from_icon_name(tray_icon, "florence");
	gtk_status_icon_set_tooltip(tray_icon, _("Florence Virtual Keyboard"));
	gtk_status_icon_set_visible(tray_icon, TRUE);
}
