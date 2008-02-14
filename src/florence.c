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

#include "keyboard.h"
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <cspi/spi.h>

#define FLO_SPI_TIME_INTERVAL 100
guint flo_width=640;
guint flo_height=240;

void fatal (char *s)
{
	fprintf(stderr, (_("FATAL ERROR :\n")));
	fprintf(stderr, (_("%s\n")), s);
	exit(1);
}

void destroy (void)
{
	gtk_exit (0);
}

void focus_event (const AccessibleEvent *event, void *user_data)
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

void traverse (Accessible *obj)
{
	int n_children, i;
	Accessible *child;

        n_children = Accessible_getChildCount (obj);
        if (!Accessible_isTable (obj))
        {
                for (i = 0; i < n_children; ++i)
                {
                        child=Accessible_getChildAtIndex (obj, i);
                        traverse(child);
                        Accessible_unref(child);
                }
	}
}

void window_create_event (const AccessibleEvent *event, gpointer user_data)
{
	/* For some reason, focus state change does happen after traverse 
	 * ==> did I misunderstand? */
	traverse(event->source);
}

gboolean spi_event_check (gpointer data)
{
	AccessibleEvent *event;
	while (event=SPI_nextEvent(FALSE)) {}
	return TRUE;
}

void flo_set_mask(GdkWindow *window)
{
	int x, y, o;
	guchar *kbmap=keyboard_get_map();
	GdkBitmap *mask;
	guchar *data=g_malloc(sizeof(guchar)*flo_width*flo_height/8);
	guchar byte;

	for (y=0;y<flo_height;y++) {
		for (x=0;x<(flo_width/8);x++) {
			byte=0;
			/* here with have a big/little endian problem
			 * ==> TODO : use autotools' copnfig.h */
			for(o=7;o>=0;o--) {
				byte<<=1;
				if (kbmap[(8*x)+(y*flo_width)+o]) byte|=1; else byte&=0xfe;
			}
			data[x+(y*flo_width/8)]=byte;
		}
	}
	mask=gdk_bitmap_create_from_data(window, data, flo_width, flo_height);
	gdk_window_shape_combine_mask(window, mask, 0, 0);

	g_object_unref(G_OBJECT(mask));
	g_free(data);
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
	gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy), NULL);
	gtk_container_border_width(GTK_CONTAINER(window), 10);
	
	gtk_window_set_keep_above((GtkWindow *)window, TRUE);
	gtk_window_set_accept_focus((GtkWindow *)window, FALSE);
	gtk_window_set_skip_taskbar_hint((GtkWindow *)window, TRUE);

	printf((_("Using config file %s\n")),config);
        gkf=g_key_file_new();
        if (!g_key_file_load_from_file(gkf, config, G_KEY_FILE_NONE, NULL))
                fatal ("Unable to load config file");
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
		gtk_window_set_default_size((GtkWindow *)window, flo_width, flo_height);
		gtk_widget_show(window);
		flo_set_mask(gtk_widget_get_parent_window(GTK_WIDGET(canvas)));
	}
	else {
		borderWidth=gtk_container_get_border_width(GTK_CONTAINER(window));
		flo_width+=2*borderWidth;
		flo_height+=2*borderWidth;
		gtk_window_set_default_size((GtkWindow *)window, flo_width, flo_height);
	}

	g_key_file_free(gkf);

	SPI_init ();
	if (showOnFocus) {
		gtk_widget_hide(window);

		/* SPI binding */
		focus_listener=SPI_createAccessibleEventListener (focus_event, (void*)window);
		SPI_registerGlobalEventListener(focus_listener, "object:state-changed:focused");
		window_listener=SPI_createAccessibleEventListener (window_create_event, NULL);
		SPI_registerGlobalEventListener(window_listener, "window:create");
		g_timeout_add(FLO_SPI_TIME_INTERVAL, spi_event_check, NULL);
	} else {
		gtk_widget_show(window);
	}

	printf((_("Florence is ready.\n")));
	fflush(stdout);
	gtk_main();

	keyboard_exit();
	return SPI_exit();
}

