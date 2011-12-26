/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008, 2009, 2010 François Agrech

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
#include <cairo/cairo-xlib.h>

/* Show the view next to the accessible object if specified. */
#ifdef AT_SPI
#ifdef ENABLE_AT_SPI2
void view_show (struct view *view, AtspiAccessible *object)
#else
void view_show (struct view *view, Accessible *object)
#endif
#else
void view_show (struct view *view)
#endif
{
#ifndef APPLET
	gtk_widget_show(GTK_WIDGET(view->window));
	/* Some winwow managers forget it */
	gtk_window_set_keep_above(view->window, TRUE);
	/* reposition the window */
	gtk_window_move(view->window, settings_get_int("window/xpos"), settings_get_int("window/ypos"));
#ifdef ENABLE_AT_SPI
	/* positionnement intelligent */
	if (settings_get_bool("behaviour/auto_hide") && 
		settings_get_bool("behaviour/move_to_widget") && object) {
		tools_window_move(view->window, object);
	}
#endif
#else
	gtk_widget_show(GTK_WIDGET(view->window));
#endif
}

/* Hides the view */
void view_hide (struct view *view)
{
	gtk_widget_hide(GTK_WIDGET(view->window));
}

/* resize the window */
void view_resize (struct view *view)
{
	GdkRectangle rect;
	GdkGeometry hints;
	hints.win_gravity=GDK_GRAVITY_NORTH_WEST;
#ifndef APPLET
	if (settings_get_bool("window/resizable")) {
		gtk_window_set_resizable(view->window, TRUE);
		if (settings_get_bool("window/keep_ratio")) {
			hints.min_aspect=view->vwidth/view->vheight;
			hints.max_aspect=view->vwidth/view->vheight;
			gtk_window_set_geometry_hints(view->window, NULL, &hints,
				GDK_HINT_ASPECT|GDK_HINT_WIN_GRAVITY);
		} else {
			gtk_window_set_geometry_hints(view->window, NULL, &hints,
				GDK_HINT_WIN_GRAVITY);
		}
		/* Do not call configure signal handler */
		if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
		view->configure_handler=0;
		gtk_window_resize(view->window, view->width, view->height);
	} else {
		gtk_window_set_geometry_hints(view->window, NULL, &hints,
			GDK_HINT_WIN_GRAVITY);
		gtk_window_set_resizable(view->window, FALSE);
#endif
		gtk_widget_set_size_request(GTK_WIDGET(view->window),
			view->width, view->height);
#ifndef APPLET
	}
#endif
	/* refresh the view */
	if (view->window && GTK_WIDGET(view->window)->window) {
		rect.x=0; rect.y=0;
		rect.width=view->width; rect.height=view->height;
		gdk_window_invalidate_rect(GTK_WIDGET(view->window)->window, &rect, TRUE);
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
	cairo_set_operator(offscreen, CAIRO_OPERATOR_SOURCE);
	cairo_paint(offscreen);
	cairo_set_operator(offscreen, CAIRO_OPERATOR_OVER);

	/* browse the keyboards */
	cairo_save(offscreen);
	cairo_scale(offscreen, view->scalex, view->scaley);
	while (list)
	{
		keyboard=(struct keyboard *)list->data;
		if (keyboard_activated(keyboard)) {
			/* actual draw */
			switch(class) {
				case STYLE_SHAPE:
					keyboard_background_draw(keyboard, offscreen, view->style, view->status);
					if (keyboard->under) {
						cairo_set_source_rgba(offscreen, 0.0, 0.0, 0.0, 0.75);
						cairo_set_operator(offscreen, CAIRO_OPERATOR_OVER);
						cairo_rectangle(offscreen, keyboard->xpos, keyboard->ypos,
							keyboard_get_width(keyboard), keyboard_get_height(keyboard));
						cairo_fill(offscreen);
					}
					break;
				case STYLE_SYMBOL:
					keyboard_symbols_draw(keyboard, offscreen, view->style, view->status);
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
void view_keyboards_set_pos(struct view *view, struct keyboard *over)
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
					if (over) keyboard_set_under(keyboard); else keyboard_set_over(keyboard);
					break;
				case LAYOUT_TOP:
					yoffset+=keyboard_get_height(keyboard);
					x=0.0; y=-yoffset;
					if (over) keyboard_set_under(keyboard); else keyboard_set_over(keyboard);
					break;
				case LAYOUT_BOTTOM:
					x=0.0; y=height;
					height+=keyboard_get_height(keyboard);
					if (over) keyboard_set_under(keyboard); else keyboard_set_over(keyboard);
					break;
				case LAYOUT_LEFT:
					xoffset+=keyboard_get_width(keyboard);
					x=-xoffset; y=0.0;
					if (over) keyboard_set_under(keyboard); else keyboard_set_over(keyboard);
					break;
				case LAYOUT_RIGHT:
					x=width; y=0.0;
					width+=keyboard_get_width(keyboard);
					if (over) keyboard_set_under(keyboard); else keyboard_set_over(keyboard);
					break;
				case LAYOUT_OVER:
					if (keyboard_get_width(keyboard)>width) width=keyboard_get_width(keyboard);
					if (keyboard_get_height(keyboard)>height) height=keyboard_get_height(keyboard);
					x=(width-view->xoffset-keyboard_get_width(keyboard))/2.0;
					y=(height-view->yoffset-keyboard_get_height(keyboard))/2.0;
					if (over==keyboard) keyboard_set_over(keyboard); else keyboard_set_under(keyboard);
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
	struct keyboard *over=NULL;

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
				case LAYOUT_OVER:
					if (keyboard_get_width(keyboard)>view->vwidth) view->vwidth=keyboard_get_width(keyboard);
					if (keyboard_get_height(keyboard)>view->vheight) view->vheight=keyboard_get_height(keyboard);
					over=keyboard;
					break;
			}
		}
		list = list->next;
	}
	view->width=(guint)(view->vwidth*view->scalex);
	view->height=(guint)(view->vheight*view->scaley);
	view_keyboards_set_pos(view, over);
}

/* get the key at position */
#ifdef ENABLE_RAMBLE
struct key *view_hit_get (struct view *view, gint x, gint y, enum key_hit *hit)
#else
struct key *view_hit_get (struct view *view, gint x, gint y)
#endif
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
		kx=keyboard->xpos*view->scalex;
		ky=keyboard->ypos*view->scaley;
		kw=keyboard->width*view->scalex;
		kh=keyboard->height*view->scaley;
		if (keyboard_activated(keyboard) && (!keyboard->under) && (x>=kx) && (x<=(kx+kw)) && (y>=ky) && y<=(ky+kh)) {
			list=NULL;
		}
		else list = list->next;
	}
#ifdef ENABLE_RAMBLE
	key=keyboard_hit_get(keyboard, x-kx, y-ky, view->scalex, view->scaley, hit);
#else
	key=keyboard_hit_get(keyboard, x-kx, y-ky, view->scalex, view->scaley);
#endif

	return key;
}

#ifndef APPLET
/* Create a window mask for transparent window for non-composited screen */
/* For composited screen, this function is useless, use alpha channel instead. */
void view_create_window_mask(struct view *view)
{
	GdkBitmap *mask=NULL;
	cairo_t *cairoctx=NULL;

	if (settings_get_bool("window/transparent") && (!view->composite)) {
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
		status_focus_zoom_set(view->status, FALSE);
	} else {
		gdk_window_shape_combine_mask(GTK_WIDGET(view->window)->window, NULL, 0, 0);
		status_focus_zoom_set(view->status, TRUE);
	}
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
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
	gtk_window_move(view->window, settings_get_int("window/xpos"), settings_get_int("window/ypos"));
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
#endif

/* Triggered by gconf when a color parameter is changed. */
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
	} else if (!strncmp(key, "label", 5)) {
		if (view->symbols) cairo_surface_destroy(view->symbols);
		view->symbols=NULL;
	}
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
}

/* Triggered by gconf when the "resizable" parameter is changed.
   makes the window (not)resizable the window. */
void view_set_keep_ratio(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	if (settings_get_bool("window/keep_ratio")) {
		view->scaley=view->scalex;
	}
	view_resize(view);
	view_redraw(client, xnxn_id, entry, user_data);
}

/* Redraw the key to the window */
void view_update(struct view *view, struct key *key, gboolean statechange)
{
	GdkRectangle *rect;
	GdkCursor *cursor;

	if (key) {
		if (statechange) {
			if (view->symbols) cairo_surface_destroy(view->symbols);
			view->symbols=NULL;
			gtk_widget_queue_draw(GTK_WIDGET(view->window));
		} else {
			rect=keyboard_key_getrect((struct keyboard *)key_get_keyboard(key),
				key, status_focus_zoom_get(view->status));
			gdk_window_invalidate_rect(GTK_WIDGET(view->window)->window, rect, TRUE);
		}
	}
	if (status_focus_get(view->status)) {
		if (!view->hand_cursor) {
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
	GdkScreen *screen=gtk_widget_get_screen(widget);
	GdkColormap *colormap=gdk_screen_get_rgba_colormap(screen);
	if (colormap) {
		if (view) view->composite=TRUE;
	} else { 
		flo_info(_("Your screen does not support alpha channel. Semi-transparency is disabled"));
		if (view) view->composite=FALSE;
		colormap=gdk_screen_get_rgb_colormap(screen);
	}
	gtk_widget_set_colormap(widget, colormap);
}

#ifndef APPLET
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
		if (settings_get_bool("window/keep_ratio")) {
			view->scalex=view->scaley=(gdouble)pConfig->width/view->vwidth;
		} else {
			view->scalex=(gdouble)pConfig->width/view->vwidth;
			view->scaley=(gdouble)pConfig->height/view->vheight;
		}
		if ((view->scalex>200.0)||(view->scaley>200.0))
			flo_warn(_("Window size out of range :%d, %d"), view->scalex, view->scaley);
		else {
			settings_double_set("window/scalex", view->scalex, FALSE);
			settings_double_set("window/scaley", view->scaley, FALSE);
		}
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
#endif

/* draw the background of the keyboard */
void view_draw_background (struct view *view, cairo_t *context)
{
	/* prepare the background */
	if (!view->background) {
		view_background_draw(view, context);
	}

	/* paint the background */
	cairo_set_operator(context, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(context, view->background, 0, 0);
	cairo_paint(context);
}

/* draw a list of keys (latched or locked keys) */
void view_draw_list (struct view *view, cairo_t *context, GList *list)
{
	struct keyboard *keyboard;
	struct key *key;
	while (list) {
		key=(struct key *)list->data;
		keyboard=(struct keyboard *)key_get_keyboard(key);
		keyboard_press_draw(keyboard, context, view->style, key, view->status);
		list=list->next;
	}
}

/* draw a single key (pressed or focused) */
void view_draw_key (struct view *view, cairo_t *context, struct key *key)
{
	struct keyboard *keyboard;
	if (key) {
		keyboard=(struct keyboard *)key_get_keyboard(key);
		keyboard_focus_draw(keyboard, context,
			(gdouble)cairo_xlib_surface_get_width(view->background),
			(gdouble)cairo_xlib_surface_get_height(view->background),
			view->style, key, view->status);
	}
}

/* on expose event: draws the keyboards to the window */
void view_expose (GtkWidget *window, GdkEventExpose* pExpose, struct view *view)
{
	cairo_t *context;
	enum key_state state;

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

	/* clear the area */
	if (settings_get_bool("window/transparent")) {
		cairo_set_source_rgba(context, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
		cairo_paint(context);
	}

	view_draw_background(view, context);

	/* draw the symbols */
	if (!view->symbols) {
		view_symbols_draw(view, context);
	}
	cairo_set_source_surface(context, view->symbols, 0, 0);
	cairo_paint(context);

	/* handle composited transparency */
	/* TODO: check for transparency support in WM */
	if (view->composite && settings_double_get("window/opacity")!=100.0) {
		if (settings_double_get("window/opacity")>100.0 ||
			settings_double_get("window/opacity")<1.0) {
			flo_error(_("Window opacity out of range (1.0 to 100.0): %f"),
				settings_double_get("window/opacity"));
		}
		cairo_set_source_rgba(context, 0.0, 0.0, 0.0,
			(100.0-settings_double_get("window/opacity"))/100.0);
		cairo_set_operator(context, CAIRO_OPERATOR_DEST_OUT);
		cairo_paint(context);
		cairo_set_operator(context, CAIRO_OPERATOR_OVER);
	}

	cairo_save(context);
	cairo_scale(context, view->scalex, view->scaley);

	/* draw highlights (pressed keys) */
	view_draw_list(view, context, status_list_latched(view->status));
	view_draw_list(view, context, status_list_locked(view->status));

	/* pressed and focused key */
	view_draw_key(view, context, status_focus_get(view->status));
	if (status_pressed_get(view->status)) {
		state=status_pressed_get(view->status)->state;
		key_state_set(status_pressed_get(view->status), KEY_PRESSED);
		view_draw_key(view, context, status_pressed_get(view->status));
		key_state_set(status_pressed_get(view->status), state);
	}

	cairo_restore(context);

#ifdef ENABLE_RAMBLE
	if (view->ramble) ramble_draw(view->ramble, context);
#endif

	/* and free up drawing memory */
	cairo_destroy(context);

#ifndef APPLET
	/* restore configure event handler. */
	if (!view->configure_handler) 
		view->configure_handler=g_signal_connect(G_OBJECT(view->window), "configure-event",
			G_CALLBACK(view_configure), view);
#endif
}

/* on keys changed events */
void view_on_keys_changed(gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	if (view->symbols) cairo_surface_destroy(view->symbols);
	view->symbols=NULL;
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
}

/* Triggered by gconf when the "extensions" parameter is changed. */
void view_update_extensions(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	GSList *list=view->keyboards;
	struct keyboard *keyboard;

#ifndef APPLET
	/* Do not call configure signal handler */
	if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
	view->configure_handler=0;
#endif

	while (list)
	{
		keyboard=(struct keyboard *)list->data;
		keyboard_status_update(keyboard, view->status);
		list=list->next;
	}

	view_set_dimensions(view);
	view_resize(view);
	if (view->background) cairo_surface_destroy(view->background);
	view->background=NULL;
	if (view->symbols) cairo_surface_destroy(view->symbols);
	view->symbols=NULL;
#ifndef APPLET
	view_create_window_mask(view);
#endif
	status_focus_set(view->status, NULL);
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
}

/* Triggered by gconf when the "zoom" parameter is changed. */
void view_set_scalex(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
#ifndef APPLET
	/* Do not call configure signal handler */
	if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
	view->configure_handler=0;
#endif
	view->scalex=gconf_value_get_float(gconf_entry_get_value(entry));
	if (settings_get_bool("window/keep_ratio")) view->scaley=view->scalex;
	view_update_extensions(client, xnxn_id, entry, user_data);
}

/* Triggered by gconf when the "zoom" parameter is changed. */
void view_set_scaley(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
#ifndef APPLET
	/* Do not call configure signal handler */
	if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
	view->configure_handler=0;
#endif
	view->scaley=gconf_value_get_float(gconf_entry_get_value(entry));
	if (settings_get_bool("window/keep_ratio")) view->scalex=view->scaley;
	view_update_extensions(client, xnxn_id, entry, user_data);
}

/* Triggered by gconf when the "opacity" parameter is changed. */
void view_set_opacity(GConfClient *client, guint xnxn_id, GConfEntry *entry, gpointer user_data)
{
	struct view *view=(struct view *)user_data;
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
}

/* get gtk window of the view */
#ifdef APPLET
PanelApplet *view_window_get (struct view *view)
#else
GtkWindow *view_window_get (struct view *view)
#endif
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
#ifdef APPLET
struct view *view_new (struct status *status, struct style *style, GSList *keyboards, PanelApplet *applet)
#else
struct view *view_new (struct status *status, struct style *style, GSList *keyboards)
#endif
{
	struct view *view=g_malloc(sizeof(struct view));
	if (!view) flo_fatal(_("Unable to allocate memory for view"));
	memset(view, 0, sizeof(struct view));

	view->status=status;
	view->style=style;
	view->keyboards=keyboards;
	view->scalex=settings_double_get("window/scalex");
	view->scaley=settings_double_get("window/scaley");
	view_set_dimensions(view);
#ifdef APPLET
	view->window = applet;
#else
	view->window=GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_keep_above(view->window, settings_get_bool("window/always_on_top"));
 	gtk_window_set_accept_focus(view->window, FALSE);
	gtk_window_set_skip_taskbar_hint(view->window, !settings_get_bool("window/task_bar"));
#ifdef HAVE_RESIZE_GRIP
	/* Remove resize grip since it is buggy */
	gtk_window_set_has_resize_grip(view->window, FALSE);
#endif
#endif
	view_resize(view);
	gtk_container_set_border_width(GTK_CONTAINER(view->window), 0);
#if GTK_CHECK_VERSION(2,12,0)
	gtk_widget_set_events(GTK_WIDGET(view->window),
		GDK_EXPOSURE_MASK|GDK_POINTER_MOTION_HINT_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
		GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK|GDK_STRUCTURE_MASK|GDK_POINTER_MOTION_MASK);
#else
	gtk_widget_set_events(GTK_WIDGET(view->window),
		GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
		GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK|GDK_STRUCTURE_MASK|GDK_POINTER_MOTION_MASK);
#endif
	gtk_widget_set_app_paintable(GTK_WIDGET(view->window), TRUE);
#ifndef APPLET
	gtk_window_set_decorated(view->window, settings_get_bool("window/decorated"));
	gtk_window_move(view->window, settings_get_int("window/xpos"), settings_get_int("window/ypos"));
#endif
	/*g_signal_connect(gdk_keymap_get_default(), "keys-changed", G_CALLBACK(view_on_keys_changed), view);*/
	xkeyboard_register_events(status->xkeyboard, view_on_keys_changed, (gpointer)view);
	g_signal_connect(G_OBJECT(view->window), "screen-changed", G_CALLBACK(view_screen_changed), view);
#ifndef APPLET
	view->configure_handler=g_signal_connect(G_OBJECT(view->window), "configure-event",
		G_CALLBACK(view_configure), view);
#endif
	g_signal_connect(G_OBJECT(view->window), "expose-event", G_CALLBACK(view_expose), view);
	view_screen_changed(GTK_WIDGET(view->window), NULL, view);
	gtk_widget_show(GTK_WIDGET(view->window));
#ifndef APPLET
	view_create_window_mask(view);
#endif

	/* register settings callbacks */
#ifndef APPLET
	settings_changecb_register("window/transparent", view_set_transparent, view);
	settings_changecb_register("window/decorated", view_set_decorated, view);
	settings_changecb_register("window/resizable", view_set_resizable, view);
	settings_changecb_register("window/always_on_top", view_set_always_on_top, view);
	settings_changecb_register("window/task_bar", view_set_task_bar, view);
#endif
	settings_changecb_register("window/keep_ratio", view_set_keep_ratio, view);
	settings_changecb_register("window/scalex", view_set_scalex, view);
	settings_changecb_register("window/scaley", view_set_scaley, view);
	settings_changecb_register("window/opacity", view_set_opacity, view);
	settings_changecb_register("layout/extensions", view_update_extensions, view);
	settings_changecb_register("colours/key", view_redraw, view);
	settings_changecb_register("colours/outline", view_redraw, view);
	settings_changecb_register("colours/label", view_redraw, view);
	settings_changecb_register("colours/label_outline", view_redraw, view);
	settings_changecb_register("colours/activated", view_redraw, view);
	settings_changecb_register("colours/latched", view_redraw, view);

#ifndef APPLET
	/* set the window icon */
	tools_set_icon(view->window);
#endif
	return view;
}

/* Change the layout and style of the view and redraw */
void view_update_layout(struct view *view, struct style *style, GSList *keyboards)
{
	view->style=style;
	view->keyboards=keyboards;
	view_update_extensions(NULL, 0, NULL, (gpointer)view);
}

