/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008, 2009 François Agrech

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

/* sets the window icon to florence.svg */
void tools_set_icon (GtkWindow *window)
{
	GError *error=NULL;
	GdkPixbuf *icon;
	icon=gdk_pixbuf_new_from_file_at_size(ICONDIR "/florence.svg", 64, 64, &error);
	if (error) flo_warn(_("Error loading icon file: %s (%s)"),
		ICONDIR "/florence.svg", error->message);
	else gtk_window_set_icon(window, icon);
}

/* open a YES/NO dialog window and return the user response */
gint tools_dialog(const gchar *title, GtkWindow *parent,
	const gchar *accept, const gchar *reject, const gchar *text)
{
        GtkWidget *dialog, *label;
	dialog=gtk_dialog_new_with_buttons(title, parent, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
		accept, GTK_RESPONSE_ACCEPT, reject, GTK_RESPONSE_REJECT, NULL);
	label=gtk_label_new(text);
	tools_set_icon(GTK_WINDOW(dialog));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
	gtk_widget_show_all(dialog);
	return gtk_dialog_run(GTK_DIALOG(dialog));
}

