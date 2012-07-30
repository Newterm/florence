/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2012 François Agrech

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#include "system.h"
#include "tools.h"
#include "trace.h"
#include <string.h>
#include <glib.h>

/* sets the window icon to florence.svg */
void tools_set_icon (GtkWindow *window)
{
	START_FUNC
	GError *error=NULL;
	GdkPixbuf *icon;
	icon=gdk_pixbuf_new_from_file_at_size(ICONDIR "/florence.svg", 64, 64, &error);
	if (error) flo_warn(_("Error loading icon file: %s (%s)"),
		ICONDIR "/florence.svg", error->message);
	else {
		gtk_window_set_icon(window, icon);
		g_object_unref(G_OBJECT(icon));
	}
	END_FUNC
}

/* open a YES/NO dialog window and return the user response */
gint tools_dialog(const gchar *title, GtkWindow *parent,
	const gchar *accept, const gchar *reject, const gchar *text)
{
	START_FUNC
	gint ret;
        GtkWidget *dialog, *label;
	dialog=gtk_dialog_new_with_buttons(title, parent, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
		accept, GTK_RESPONSE_ACCEPT, reject, GTK_RESPONSE_REJECT, NULL);
	label=gtk_label_new(text);
	tools_set_icon(GTK_WINDOW(dialog));
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
	gtk_widget_show_all(dialog);
	ret=gtk_dialog_run(GTK_DIALOG(dialog));
	g_object_unref(G_OBJECT(dialog));
	END_FUNC
	return ret;
}

#ifdef AT_SPI
/* position a window near the specified object */
#ifdef ENABLE_AT_SPI2
void tools_window_move(GtkWindow *window, AtspiAccessible *object)
#else
void tools_window_move(GtkWindow *window, Accessible *object)
#endif
{
	START_FUNC
#ifdef ENABLE_AT_SPI2
	AtspiRect *rect;
	AtspiComponent *component=NULL;
#else
	AccessibleComponent *component=NULL;
#endif
	long int x, y, h;
	gint screen_width, screen_height;
	gint win_width, win_height;
	GdkRectangle win_rect;

	if (!object) {
	       flo_error(_("NULL accessible object, unable to move window"));
	       END_FUNC
	       return;
	}
#ifdef ENABLE_AT_SPI2
	component=atspi_accessible_get_component(object);
#else
	if (Accessible_isComponent(object)) component=Accessible_getComponent(object);
#endif
	if (component) {
		screen_height=gdk_screen_get_height(gdk_screen_get_default());
		screen_width=gdk_screen_get_width(gdk_screen_get_default());
#ifdef ENABLE_AT_SPI2
		rect=atspi_component_get_extents(component, ATSPI_COORD_TYPE_SCREEN, NULL);
		x=rect->x; y=rect->y; h=rect->height;
		g_free(rect);
#else
		AccessibleComponent_getExtents(component, &x, &y, NULL, &h, SPI_COORD_TYPE_SCREEN);
#endif
		if (gtk_window_get_decorated(window)) {
			gdk_window_get_frame_extents(gtk_widget_get_window(GTK_WIDGET(window)), &win_rect);
			win_width=win_rect.width;
			win_height=win_rect.height;
		} else gtk_window_get_size(window, &win_width, &win_height);

		if (x<0) x=0;
		else if (win_width>(screen_width-x)) x=screen_width-win_width;

		gtk_window_set_gravity(window, GDK_GRAVITY_NORTH_WEST);
		if (win_height<(screen_height-y-h)) gtk_window_move(window, x, y+h);
		else if (y>win_height) gtk_window_move(window, x, y-win_height);
		else gtk_window_move(window, x, screen_height-win_height);
	} else flo_warn(_("Unable to get component from accessible object"));
	END_FUNC
}
#endif

