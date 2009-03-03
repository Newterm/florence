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
#include "view.h"
#include "trace.h"
#include "settings.h"
#include "keyboard.h"
#include "tools.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

/* Show the view next to the accessible object if specified. */
void view_show (struct view *view, Accessible *object)
{
	AccessibleComponent *component;
	long int x, y, w, h;
	gint screen_width, screen_height;

	/* positionnement intelligent */
	if (object) {
		component=Accessible_getComponent(object);
		if (component) {
			screen_height=gdk_screen_get_height(gdk_screen_get_default());
			screen_width=gdk_screen_get_width(gdk_screen_get_default());
			AccessibleComponent_getExtents(component, &x, &y, &w, &h, SPI_COORD_TYPE_SCREEN);
			if (x<0) x=0;
			else if (view->width<(screen_width-x-w)) x=screen_width-view->width;
			if (view->height<(screen_height-y-h)) gtk_window_move(view->window, x, y+h);
			else if (y>view->height) gtk_window_move(view->window, x, y-view->height);
			else gtk_window_move(view->window, x, screen_height-view->height);
		}
	}
	gtk_widget_show(GTK_WIDGET(view->window));
	/* Some winwow managers forget it */
	gtk_window_set_keep_above(view->window, TRUE);
}

/* Hides the view */
void view_hide (struct view *view)
{
	gtk_widget_hide(GTK_WIDGET(view->window));
}

/* resize the window */
void view_resize (struct view *view)
{
	GdkGeometry hints;
	if (settings_get_bool("window/resizable")) {
		hints.min_aspect=view->vwidth/view->vheight;
		hints.max_aspect=view->vwidth/view->vheight;
		gtk_window_set_resizable(view->window, TRUE);
		gtk_window_set_geometry_hints(view->window, NULL, &hints,
			GDK_HINT_ASPECT);
		gtk_window_resize(view->window, view->width, view->height);
	} else {
		gtk_window_set_geometry_hints(view->window, NULL, &hints, 0);
		gtk_window_set_resizable(view->window, FALSE);
		gtk_widget_set_size_request(GTK_WIDGET(view->window),
			view->width, view->height);
	}
}

/* draws the background of florence */
void view_draw (struct view *view, cairo_t *cairoctx, cairo_surface_t **surface, enum style_class class)
{
	GSList *list=view->keyboards;
	struct keyboard *keyboard;
	cairo_t *offscreen;

	/* create the surface */
	if (!*surface) *surface=cairo_surface_create_similar(cairo_get_target(cairoctx),
		CAIRO_CONTENT_COLOR_ALPHA, view->width, view->height);
	offscreen=cairo_create(*surface);
	cairo_set_source_rgba(offscreen, 0.0, 0.0, 0.0, 0.0);
	cairo_set_operator(offscreen, CAIRO_OPERATOR_OVER);
	cairo_paint(offscreen);

	/* browse the keyboards */
	cairo_save(offscreen);
	cairo_scale(offscreen, view->zoom, view->zoom);
	while (list)
	{
		keyboard=(struct keyboard *)list->data;
		if (keyboard_activated(keyboard)) {
			/* actual draw */
			switch(class) {
				case STYLE_SHAPE:
					keyboard_background_draw(keyboard, offscreen, view->style);
					break;
				case STYLE_SYMBOL:
					keyboard_symbols_draw(keyboard, offscreen, view->style,
						status_globalmod_get(view->status));
					break;
			}
		}
		list=list->next;
	}
	cairo_destroy(offscreen);
}

/* draws the background of florence */
void view_background_draw (struct view *view, cairo_t *cairoctx)
{
	view_draw(view, cairoctx, &(view->background), STYLE_SHAPE);
}

/* draws the symbols */
void view_symbols_draw (struct view *view, cairo_t *cairoctx) {
	view_draw(view, cairoctx, &(view->symbols), STYLE_SYMBOL);
}

/* update the keyboard positions */
void view_keyboards_set_pos(struct view *view)
{
	GSList *list=view->keyboards;
	struct keyboard *keyboard;
	gdouble width, height, xoffset, yoffset;
	gdouble x, y;

	/* browse the keyboards */
	while (list)
	{
		keyboard=(struct keyboard *)list->data;
		if (keyboard_activated(keyboard)) {
			/* get the position to draw the keyboard */
			switch (keyboard_get_placement(keyboard)) {
				case LAYOUT_VOID:
					width=keyboard_get_width(keyboard);
					height=keyboard_get_height(keyboard);
					xoffset=yoffset=0;
					x=y=0.0;
					break;
				case LAYOUT_TOP:
					yoffset+=keyboard_get_height(keyboard);
					x=0.0; y=-yoffset;
					break;
				case LAYOUT_BOTTOM:
					x=0.0; y=height;
					height+=keyboard_get_height(keyboard);
					break;
				case LAYOUT_LEFT:
					xoffset+=keyboard_get_width(keyboard);
					x=-xoffset; y=0.0;
					break;
				case LAYOUT_RIGHT:
					x=width; y=0.0;
					width+=keyboard_get_width(keyboard);
					break;
			}
			keyboard_set_pos(keyboard, x+view->xoffset, y+view->yoffset);
		}
		list = list->next;
	}
}

/* calculate the dimensions of Florence */
void view_set_dimensions(struct view *view)
{
	GSList *list=view->keyboards;
	struct keyboard *keyboard;

	while (list)
	{
		keyboard=(struct keyboard *)list->data;
		if (keyboard_activated(keyboard)) {
			switch (keyboard_get_placement(keyboard)) {
				case LAYOUT_VOID:
					view->vwidth=keyboard_get_width(keyboard);
					view->vheight=keyboard_get_height(keyboard);
					view->xoffset=view->yoffset=0;
					break;
				case LAYOUT_TOP:
					view->vheight+=(view->yoffset+=keyboard_get_height(keyboard));
					break;
				case LAYOUT_BOTTOM:
					view->vheight+=keyboard_get_height(keyboard);
					break;
				case LAYOUT_LEFT:
					view->vwidth+=(view->xoffset+=keyboard_get_width(keyboard));
					break;
				case LAYOUT_RIGHT:
					view->vwidth+=keyboard_get_width(keyboard);
					break;
			}
		}
		list = list->next;
	}
	view->width=(guint)(view->vwidth*view->zoom);
	view->height=(guint)(view->vheight*view->zoom);
	view_keyboards_set_pos(view);
}

/* get the key at position */
struct key *view_hit_get (struct view *view, gint x, gint y)
{
	GSList *list=view->keyboards;
	struct keyboard *keyboard;
	struct key *key;
	gint kx, ky, kw, kh;

	/* find the hit keyboard */
	while (list)
	{
		keyboard=(struct keyboard *)list->data;
		/* TODO: record in pixel
		 * and move that to keyboard_test */
		kx=keyboard->xpos*view->zoom;
		ky=keyboard->ypos*view->zoom;
		kw=keyboard->width*view->zoom;
		kh=keyboard->height*view->zoom;
		if ((x>=kx) && (x<=(kx+kw)) && (y>=ky) && y<=(ky+kh)) {
			list=NULL;
		}
		else list = list->next;
	}
	key=keyboard_hit_get(keyboard, x-kx, y-ky, view->zoom);

	return key;
}

/* Create a window mask for transparent window for non-composited screen */
/* For composited screen, this function is useless, use alpha channel instead. */
void view_create_window_mask(struct view *view)
{
	GdkBitmap *mask=NULL;
	cairo_t *cairoctx=NULL;

	if (!view->composite && settings_get_bool("window/transparent")) {
		if (!(mask=(GdkBitmap*)gdk_pixmap_new(NULL, view->width, view->height, 1)))
			flo_fatal(_("Unable to create mask"));
		cairoctx=gdk_cairo_create(mask);
		view_background_draw(view, cairoctx);
		cairo_set_source_rgba(cairoctx, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cairoctx, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cairoctx);
		cairo_set_source_surface(cairoctx, view->background, 0, 0);
		cairo_paint(cairoctx);
		gdk_window_shape_combine_mask(GTK_WIDGET(view->window)->window, mask, 0, 0);
		cairo_destroy(cairoctx);
		cairo_surface_destroy(view->background);
		view->background=NULL;
		g_object_unref(G_OBJECT(mask));
	} else {
		gdk_window_shape_combine_mask(GTK_WIDGET(view->window)->window, NULL, 0, 0);
	}
}

/* Triggered by gconf when the "transparent" parameter is changed. Calls view_create_window_mask */
void view_set_transparent(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	gboolean shown=GTK_WIDGET_VISIBLE(GTK_WINDOW(view->window));
	gtk_widget_show(GTK_WIDGET(view->window));
	view_create_window_mask(view);
	if (!shown) gtk_widget_hide(GTK_WIDGET(view->window));
}

/* Triggered by gconf when the "decorated" parameter is changed. Decorates or undecorate the window. */
void view_set_decorated(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	gtk_window_set_decorated(view->window, gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* Triggered by gconf when the "always_on_top" parameter is changed. 
   Change the window property to be always on top or not to be. */
void view_set_always_on_top(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	gtk_window_set_keep_above(view->window, gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* Triggered by gconf when the "task_bar" parameter is changed. 
   Change the window hint to appear in the task bar or not. */
void view_set_task_bar(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	gtk_window_set_skip_taskbar_hint(view->window, !gconf_value_get_bool(gconf_entry_get_value(entry)));
}

/* Triggered by gconf when the "resizable" parameter is changed.
   makes the window (not)resizable the window. */
void view_set_resizable(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	if (settings_get_bool("window/resizable")) {
		gtk_widget_set_size_request(GTK_WIDGET(view->window), view->vwidth, view->vheight);
	}
	view_resize(view);
}

/* Triggered by gconf when the "zoom" parameter is changed. */
void view_redraw(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	char *key;
	key=strrchr(entry->key, '/');
	key+=1;
	style_update_colors(view->style);
	if ((!strcmp(key, "key")) || (!strcmp(key, "outline"))) {
		if (view->background) cairo_surface_destroy(view->background);
		view->background=NULL;
	} else if (!strcmp(key, "label")) {
		if (view->symbols) cairo_surface_destroy(view->symbols);
		view->symbols=NULL;
	}
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
}

/* Redraw the key to the window */
void view_update(struct view *view, struct key *key, gboolean statechange)
{
	GdkRectangle rect;
	gdouble x, y, w, h;
	GdkCursor *cursor;

	if (key) {
		if (statechange) {
			if (view->symbols) cairo_surface_destroy(view->symbols);
			view->symbols=NULL;
			gtk_widget_queue_draw(GTK_WIDGET(view->window));
		} else {
			keyboard_key_getrect((struct keyboard *)key_get_userdata(key), key, &x, &y, &w, &h);
			rect.x=x*view->zoom-5.0; rect.y=y*view->zoom-5.0;
			rect.width=w*view->zoom+10.0; rect.height=h*view->zoom+10.0;
			gdk_window_invalidate_rect(GTK_WIDGET(view->window)->window, &rect, TRUE);
		}
	}
	if (status_focus_get(view->status)) { if (!view->hand_cursor) {
			cursor=gdk_cursor_new(GDK_HAND2);
			gdk_window_set_cursor(GTK_WIDGET(view->window)->window, cursor);
			view->hand_cursor=TRUE;
		}
	} else if (view->hand_cursor) {
			gdk_window_set_cursor(GTK_WIDGET(view->window)->window, NULL);
			view->hand_cursor=FALSE;
		}
}

/* on screen change event: check for composite extension */
void view_screen_changed (GtkWidget *widget, GdkScreen *old_screen, struct view *view)
{
	GdkScreen *screen = NULL;
	GdkColormap *colormap = NULL;

	screen = gtk_widget_get_screen (widget);
	if (colormap == NULL) {
		view->composite = FALSE;
		colormap = gdk_screen_get_rgb_colormap(screen);
	} else { 
		view->composite = TRUE;
		colormap = gdk_screen_get_rgba_colormap (screen);
	}

	gtk_widget_set_colormap (widget, colormap);
}

/* on configure events: record window position */
void view_configure (GtkWidget *window, GdkEventConfigure* pConfig, struct view *view)
{
	GdkRectangle rect;

	/* record window position */
	if (settings_get_int("window/xpos")!=pConfig->x)
		settings_set_int("window/xpos", pConfig->x);
	if (settings_get_int("window/ypos")!=pConfig->y)
		settings_set_int("window/ypos", pConfig->y);

	/* handle resize events */
	if ((pConfig->width!=view->width) || (pConfig->height!=view->height)) {
		view->zoom=(gdouble)pConfig->width/view->vwidth;
		if (view->zoom > 200.0) flo_warn(_("Window size out of range :%d"), view->zoom);
		else settings_set_double("window/zoom", view->zoom);
		view->width=pConfig->width; view->height=pConfig->height;
		if (view->background) cairo_surface_destroy(view->background);
		view->background=NULL;
		if (view->symbols) cairo_surface_destroy(view->symbols);
		view->symbols=NULL;
		view_create_window_mask(view);
		rect.x=0; rect.y=0;
		rect.width=pConfig->width; rect.height=pConfig->height;
		gdk_window_invalidate_rect(GTK_WIDGET(view->window)->window, &rect, TRUE);
	}

	gdk_window_configure_finished (GTK_WIDGET(view->window)->window);
}

/* on expose event: draws the keyboards to the window */
void view_expose (GtkWidget *window, GdkEventExpose* pExpose, struct view *view)
{
	cairo_t *context;
	GList *list=status_pressedkeys_get(view->status);
	struct keyboard *keyboard;
	struct key *key;

	/* Don't need to redraw several times in one chunk */
	if (!view->redraw) view->redraw=gdk_region_new();
	gdk_region_union(view->redraw, pExpose->region);
	if (pExpose->count>0) return;

	/* create the context */
	context=gdk_cairo_create(window->window);
	gdk_cairo_region(context, view->redraw);
	cairo_clip(context); 
	gdk_region_destroy(view->redraw);
	view->redraw=NULL;

	/* prepare the background */
	if (!view->background) {
		view_background_draw(view, context);
	}
	cairo_set_operator(context, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(context, view->background, 0, 0);
	cairo_paint(context);

	/* draw highlights (pressed keys) */
	cairo_save(context);
	cairo_scale(context, view->zoom, view->zoom);
	if (list) {
		while (list) {
			key=(struct key *)list->data;
			keyboard=(struct keyboard *)key_get_userdata(key);
			keyboard_press_draw(keyboard, context, view->zoom, view->style, key);
			list=list->next;
		}
	}

	/* focused key */
	if (status_focus_get(view->status)) {
		key=status_focus_get(view->status);
		keyboard=(struct keyboard *)key_get_userdata(key);
		keyboard_focus_draw(keyboard, context, view->zoom, view->style, key, status_timer_get(view->status));
	}
	cairo_restore(context);

	/* draw the symbols */
	if (!view->symbols) {
		view_symbols_draw(view, context);
	}
	cairo_set_source_surface(context, view->symbols, 0, 0);
	cairo_paint(context);

	/* and free up drawing memory */
	cairo_destroy(context);
}

/* Triggered by gconf when the "extensions" parameter is changed. */
void view_update_extensions(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	view_set_dimensions(view);
	view_resize(view);
	if (view->background) cairo_surface_destroy(view->background);
	view->background=NULL;
	if (view->symbols) cairo_surface_destroy(view->symbols);
	view->symbols=NULL;
	view_create_window_mask(view);
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
}

/* Triggered by gconf when the "zoom" parameter is changed. */
void view_set_zoom(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	view->zoom=gconf_value_get_float(gconf_entry_get_value(entry));
	view_update_extensions(client, xnxn_id, entry, user_data);
}

/* get gtk window of the view */
GtkWindow *view_window_get (struct view *view)
{
	return view->window;
}

/* get gtk window of the view */
void view_status_set (struct view *view, struct status *status)
{
	view->status=status;
}

/* liberate all the memory used by the view */
void view_free(struct view *view)
{
	if (view->background) cairo_surface_destroy(view->background);
	if (view->symbols) cairo_surface_destroy(view->symbols);
	g_free(view);
}

/* create a view of florence */
struct view *view_new (struct style *style, GSList *keyboards)
{
	struct view *view=g_malloc(sizeof(struct view));
	if (!view) flo_fatal(_("Unable to allocate memory for view"));
	memset(view, 0, sizeof(struct view));

	view->style=style;
	view->keyboards=keyboards;
	view->zoom=settings_get_double("window/zoom");
	view_set_dimensions(view);

	view->window=GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_keep_above(view->window, settings_get_bool("window/always_on_top"));
	gtk_window_set_accept_focus(view->window, FALSE);
	gtk_window_set_skip_taskbar_hint(view->window, !settings_get_bool("window/task_bar"));
	view_resize(view);
	gtk_container_set_border_width(GTK_CONTAINER(view->window), 0);
	gtk_widget_set_events(GTK_WIDGET(view->window), GDK_ALL_EVENTS_MASK);
	gtk_widget_set_app_paintable (GTK_WIDGET(view->window), TRUE);
	gtk_window_set_decorated(view->window, settings_get_bool("window/decorated"));
	gtk_window_move(view->window, settings_get_int("window/xpos"), settings_get_int("window/ypos"));

	g_signal_connect(G_OBJECT(view->window), "screen-changed", G_CALLBACK(view_screen_changed), view);
	g_signal_connect(G_OBJECT(view->window), "configure-event", G_CALLBACK(view_configure), view);
	g_signal_connect(G_OBJECT(view->window), "expose-event", G_CALLBACK(view_expose), view);
	view_screen_changed(GTK_WIDGET(view->window), NULL, view);
	gtk_widget_show(GTK_WIDGET(view->window));
	view_create_window_mask(view);

	/* register settings callbacks */
	settings_changecb_register("window/transparent", view_set_transparent, view);
	settings_changecb_register("window/decorated", view_set_decorated, view);
	settings_changecb_register("window/resizable", view_set_resizable, view);
	settings_changecb_register("window/always_on_top", view_set_always_on_top, view);
	settings_changecb_register("window/task_bar", view_set_task_bar, view);
	settings_changecb_register("window/zoom", view_set_zoom, view);
	settings_changecb_register("layout/extensions", view_update_extensions, view);
	settings_changecb_register("colours/key", view_redraw, view);
	settings_changecb_register("colours/outline", view_redraw, view);
	settings_changecb_register("colours/label", view_redraw, view);
	settings_changecb_register("colours/activated", view_redraw, view);

	/* set the window icon */
	tools_set_icon(view->window);

	return view;
}

/* Change the layout and style of the view and redraw */
void view_update_layout(struct view *view, struct style *style, GSList *keyboards)
{
	view->style=style;
	view->keyboards=keyboards;
	view_update_extensions(NULL, 0, NULL, (gpointer)view);
}

