/* 
   florence - Florence is a simple virtual keyboard for Gnome.

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
#include "trace.h"
#include "keyboard.h"
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <cspi/spi.h>

#define FLO_SPI_TIME_INTERVAL 100
guint flo_width=640;
guint flo_height=240;

void flo_destroy (void)
{
	gtk_exit (0);
}

void flo_focus_event (const AccessibleEvent *event, void *user_data)
{
	GtkWindow *window=(GtkWindow *)user_data;
	AccessibleComponent *component;
	long int x, y, w, h;
	gint screen_width, screen_height;

	if (Accessible_isEditableText(event->source)) {
		if (event->detail1) {
			/* positionnement intelligent */
			component=Accessible_getComponent(event->source);
			if (component) {
				screen_height=gdk_screen_get_height(gdk_screen_get_default());
				screen_width=gdk_screen_get_width(gdk_screen_get_default());
				AccessibleComponent_getExtents(component, &x, &y, &w, &h, SPI_COORD_TYPE_SCREEN);
				if (x<0) x=0;
				else if (flo_width<(screen_width-x-w)) x=screen_width-flo_width;
				if (flo_height<(screen_height-y-h)) {
					gtk_window_move(window, x, y+h);
				} else if (y>flo_height) {
					gtk_window_move(window, x, y-flo_height);
				} else {
					gtk_window_move(window, x, screen_height-flo_height);
				}
			}
			gtk_widget_show(GTK_WIDGET(window));
			/* Some winwow managers forget it */
			gtk_window_set_keep_above(window, TRUE);
		}
		else {
			gtk_widget_hide(GTK_WIDGET(window)); 
		}
	}
}

void flo_traverse (Accessible *obj)
{
	int n_children, i;
	Accessible *child;

	n_children = Accessible_getChildCount (obj);
	if (!Accessible_isTable (obj))
	{
		for (i = 0; i < n_children; ++i)
		{
			child=Accessible_getChildAtIndex (obj, i);
			flo_traverse(child);
			Accessible_unref(child);
		}
	}
}

void flo_window_create_event (const AccessibleEvent *event, gpointer user_data)
{
	/* For some reason, focus state change does happen after traverse 
	 * ==> did I misunderstand? */
	flo_traverse(event->source);
}

gboolean flo_spi_event_check (gpointer data)
{
	AccessibleEvent *event;
	while (event=SPI_nextEvent(FALSE)) {}
	return TRUE;
}

void flo_about()
{
	GtkAboutDialog *about=GTK_ABOUT_DIALOG(gtk_about_dialog_new());
	gtk_show_about_dialog(NULL, "program-name", _("Florence Virtual Keyboard"),
		"version", VERSION, "copyright", _("Copyright (C) 2008 François Agrech"),
		"logo-icon-name", "florence.svg", "website", "http://florence.sourceforge.net",
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

void flo_set_mask(GdkWindow *window)
{
	int x, y, o;
	guchar *kbmap=keyboard_get_map();
	GdkBitmap *mask;
	guint width=(flo_width&0xFFFFFFF8)+(flo_width&0x7?8:0);
	guchar *data=g_malloc((sizeof(guchar)*width*flo_height)>>3);
	guchar byte;

	for (y=0;y<flo_height;y++) {
		for (x=0;x<(width>>3);x++) {
			byte=0;
			/* here with may have a big/little endian problem
			 * unless gdk or xlib handles it? 
			 * ==> TODO : check and use autotools' config.h if necessary */
			for(o=7;o>=0;o--) {
				byte<<=1;
				if (kbmap[(x<<3)+(y*flo_width)+o]) byte|=1; else byte&=0xfe;
			}
			data[x+((y*width)>>3)]=byte;
		}
	}
	mask=gdk_bitmap_create_from_data(window, data, flo_width, flo_height);
	gdk_window_shape_combine_mask(window, mask, 0, 0);

	g_object_unref(G_OBJECT(mask));
	g_free(data);
}

void flo_tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data)
{
	static gint x=0;
	static gint y=0;
	GtkWidget *window=GTK_WIDGET(user_data);
	if (GTK_WIDGET_VISIBLE(window)) {
		gtk_window_get_position(GTK_WINDOW(window), &x, &y);
		gtk_widget_hide(window);
	} else { 
		gtk_window_move(GTK_WINDOW(window), x, y);
		gtk_widget_show(window);
	}
}

void flo_tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu, *about, *quit;
 
	menu = gtk_menu_new();

	quit = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(quit), gtk_image_new_from_stock(GTK_STOCK_QUIT,GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(quit, "activate", G_CALLBACK(flo_destroy), NULL);

	about = gtk_image_menu_item_new_with_mnemonic(_("_About"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(about), gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU));
	g_signal_connect(about, "activate", G_CALLBACK(flo_about), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), about);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit);
	gtk_widget_show_all(menu);
 
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,gtk_status_icon_position_menu, status_icon,button, activate_time);
}

void flo_create_tray_icon(GtkWidget *window)
{
	GtkStatusIcon *tray_icon;

	tray_icon = gtk_status_icon_new();
	g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(flo_tray_icon_on_click), (gpointer)window);
	g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(flo_tray_icon_on_menu), NULL);
	gtk_status_icon_set_from_icon_name(tray_icon, "florence.svg");
	gtk_status_icon_set_tooltip(tray_icon, _("Florence Virtual Keyboard"));
	gtk_status_icon_set_visible(tray_icon, TRUE);
}

int florence (char *config)
{
	AccessibleEventListener *focus_listener;
	AccessibleEventListener *window_listener;
	AccessibleKeystrokeListener *keystroke_listener;
	GtkWidget *window;
	GtkWidget *canvas;
	GKeyFile *gkf;
	gboolean showOnFocus;
	guint borderWidth;

	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(flo_destroy), NULL);

	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

	printf(_("Using config file %s\n"),config);
	gkf=g_key_file_new();
	if (!g_key_file_load_from_file(gkf, config, G_KEY_FILE_NONE, NULL))
		flo_fatal ("Unable to load config file");
	showOnFocus=g_key_file_get_boolean(gkf, "Settings", "ShowOnEditable", NULL);

	canvas=gnome_canvas_new_aa();
	keyboard_init(gkf, (GnomeCanvas *)canvas);
	gtk_container_add(GTK_CONTAINER(window), canvas);
	gtk_widget_show(canvas);

	gnome_canvas_w2c(GNOME_CANVAS(canvas), 2.0*g_key_file_get_double(gkf, "Window", "width", NULL),
		2.0*g_key_file_get_double(gkf, "Window", "height", NULL), &flo_width, &flo_height);
	if (g_key_file_get_boolean(gkf, "Window", "Shaped", NULL)) {
		gtk_window_set_decorated((GtkWindow *)window, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(window), 0);
		gtk_widget_set_size_request(GTK_WIDGET(window), flo_width, flo_height);
		gtk_widget_show(window);
		flo_set_mask(gtk_widget_get_parent_window(GTK_WIDGET(canvas)));
	}
	else {
		borderWidth=gtk_container_get_border_width(GTK_CONTAINER(window));
		flo_width+=2*borderWidth;
		flo_height+=2*borderWidth;
		gtk_widget_set_size_request(GTK_WIDGET(window), flo_width, flo_height);
	}
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	g_key_file_free(gkf);

	SPI_init ();
	if (showOnFocus) {
		gtk_widget_hide(window);

		/* SPI binding */
		focus_listener=SPI_createAccessibleEventListener (flo_focus_event, (void*)window);
		SPI_registerGlobalEventListener(focus_listener, "object:state-changed:focused");
		window_listener=SPI_createAccessibleEventListener (flo_window_create_event, NULL);
		SPI_registerGlobalEventListener(window_listener, "window:create");
		g_timeout_add(FLO_SPI_TIME_INTERVAL, flo_spi_event_check, NULL);
	} else {
		gtk_widget_show(window);
	}

	flo_create_tray_icon(window);
	gtk_main();

	keyboard_exit();
	return SPI_exit();
}

