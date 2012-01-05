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
		gdk_pixbuf_unref(icon);
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
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
	gtk_widget_show_all(dialog);
	ret=gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_object_destroy(GTK_OBJECT(dialog));
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
	AtspiComponent *component;
#else
	AccessibleComponent *component;
#endif
	long int x, y, w, h;
	gint screen_width, screen_height;
	gint win_width, win_height;
	GdkRectangle win_rect;

	if (!object) {
	       flo_error(_("NULL accessible object, unable to move window"));
	       return;
	}
#ifdef ENABLE_AT_SPI2
	component=atspi_accessible_get_component(object);
#else
	component=Accessible_getComponent(object);
#endif
	if (component) {
		screen_height=gdk_screen_get_height(gdk_screen_get_default());
		screen_width=gdk_screen_get_width(gdk_screen_get_default());
#ifdef ENABLE_AT_SPI2
		rect=atspi_component_get_extents(component, ATSPI_COORD_TYPE_SCREEN, NULL);
		x=rect->x; y=rect->y; w=rect->width; h=rect->height;
		g_free(rect);
#else
		AccessibleComponent_getExtents(component, &x, &y, &w, &h, SPI_COORD_TYPE_SCREEN);
#endif
		if (gtk_window_get_decorated(window)) {
			gdk_window_get_frame_extents(GTK_WIDGET(window)->window, &win_rect);
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

#if !GLIB_CHECK_VERSION(2,14,0)
/* replace glib 2.14's g_regex_new */
gchar *tools_regex_new(gchar *regex)
{
	START_FUNC
	gchar *result=g_malloc(strlen(regex)+1);
	END_FUNC
	return strcpy(result, regex);
}

/* replace glib 2.14's g_regex_unref */
void tools_regex_free(gchar *regex)
{
	START_FUNC
	g_free(regex);
	END_FUNC
}

/* replace glib 2.14's g_regex_match */
/* WARNING: this is a hack around lack of GRegex from GLib 2.14 */
/* It is best to have proper GRegex with GLib. */
gboolean tools_regex_match(gchar *regex, gchar *text)
{
	START_FUNC
	gboolean match=FALSE;
	if ((!strchr(regex, '(')) && (!strchr(regex, '['))) {
		match=((!strncmp(regex+1, text, strlen(regex)-2))&&
			(!strncmp(regex+1, text, strlen(text))));
	} else if (!strcmp(regex, "^Control_[LR]$"))
		match=(!strcmp(text, "Control_L")) || (!strcmp(text, "Control_R"));
	else if (!strcmp(regex, "^Super_[LR]$"))
		match=(!strcmp(text, "Super_L")) || (!strcmp(text, "Super_R"));
	else if (!strcmp(regex, "^(Alt_[LR]|Meta_L)$"))
		match=(!strcmp(text, "Alt_L")) || (!strcmp(text, "Alt_R")) || (!strcmp(text, "Meta_L"));
	else if (!strcmp(regex, "^(Shift_[LR]|(KP_|)Up)$"))
		match=(!strcmp(text, "Shift_L")) || (!strcmp(text, "Shift_R")) ||
			(!strcmp(text, "KP_Up")) || (!strcmp(text, "Up"));
	else if (!strcmp(regex, "^(KP_|)Insert$"))
		match=(!strcmp(text, "Insert")) || (!strcmp(text, "KP_Insert"));
	else if (!strcmp(regex, "^(KP_|)Home$"))
		match=(!strcmp(text, "Home")) || (!strcmp(text, "KP_Home"));
	else if (!strcmp(regex, "^(KP_|)Page_Up$"))
		match=(!strcmp(text, "Page_Up")) || (!strcmp(text, "KP_Page_Up"));
	else if (!strcmp(regex, "^(KP_|)Page_Down$"))
		match=(!strcmp(text, "Page_Down")) || (!strcmp(text, "KP_Page_Down"));
	else if (!strcmp(regex, "^(KP_|)Delete$"))
		match=(!strcmp(text, "Delete")) || (!strcmp(text, "KP_Delete"));
	else if (!strcmp(regex, "^(KP_|)End$"))
		match=(!strcmp(text, "End")) || (!strcmp(text, "KP_End"));
	else if (!strcmp(regex, "^(KP_|)Down$"))
		match=(!strcmp(text, "Down")) || (!strcmp(text, "KP_Down"));
	else if (!strcmp(regex, "^(KP_|)Left$"))
		match=(!strcmp(text, "Left")) || (!strcmp(text, "KP_Left"));
	else if (!strcmp(regex, "^(KP_|)Right$"))
		match=(!strcmp(text, "Right")) || (!strcmp(text, "KP_Right"));
	END_FUNC
	return match;
}

/* replace glib 2.14's g_regex_replace_literal */
gchar *tools_regex_replace_literal(gchar *old, gchar *source, gchar *new)
{
	START_FUNC
	gchar *match=NULL;
	gchar *result=NULL;
	if ((match=strstr(source, old))) {
		result=g_malloc(strlen(source)-strlen(old)+strlen(new)+1);
		if (!result) flo_error(_("unable to allocate memory for string"));
		result=strncpy(result, source, match-source);
		result[match-source]='\0';
		result=strcat(result, new);
		result=strcat(result, match+strlen(old));
	} else {
		result=g_malloc(strlen(source)+1);
		strcpy(result, source);
	}
	if ((match=strstr(result, old))) {
		match=tools_regex_replace_literal(old, result, new);	
		g_free(result);
		result=match;
	}
	END_FUNC
	return result;
}
#endif
