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
#include "view.h"
#include "trace.h"
#include "settings.h"
#include "keyboard.h"
#include "tools.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo/cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>


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
	START_FUNC
	gtk_widget_show(GTK_WIDGET(view->window));
	/* Some winwow managers forget it */
	gtk_window_set_keep_above(view->window, TRUE);
	/* reposition the window */
	gtk_window_move(view->window, settings_get_int(SETTINGS_XPOS), settings_get_int(SETTINGS_YPOS));
#ifdef ENABLE_AT_SPI
	/* positionnement intelligent */
	if (settings_get_bool(SETTINGS_AUTO_HIDE) && 
		settings_get_bool(SETTINGS_MOVE_TO_WIDGET) && object) {
		tools_window_move(view->window, object);
	}
#endif
	END_FUNC
}

/* Hides the view */
void view_hide (struct view *view)
{
	START_FUNC
	gtk_widget_hide(GTK_WIDGET(view->window));
	END_FUNC
}

/* resize the window */
void view_resize (struct view *view)
{
	START_FUNC
	GdkRectangle rect;
	GdkGeometry hints;
	hints.win_gravity=GDK_GRAVITY_NORTH_WEST;
	if (settings_get_bool(SETTINGS_RESIZABLE)) {
		gtk_window_set_resizable(view->window, TRUE);
		if (settings_get_bool(SETTINGS_KEEP_RATIO)) {
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
		gtk_widget_set_size_request(GTK_WIDGET(view->window),
			view->width, view->height);
	}
	/* refresh the view */
	if (view->window && gtk_widget_get_window(GTK_WIDGET(view->window))) {
		rect.x=0; rect.y=0;
		rect.width=view->width; rect.height=view->height;
		gdk_window_invalidate_rect(gtk_widget_get_window(GTK_WIDGET(view->window)), &rect, TRUE);
	}
	END_FUNC
}

/* draws the background of florence */
void view_draw (struct view *view, cairo_t *cairoctx, cairo_surface_t **surface, enum style_class class)
{
	START_FUNC
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
	END_FUNC
}

/* draws the background of florence */
void view_background_draw (struct view *view, cairo_t *cairoctx)
{
	START_FUNC
	view_draw(view, cairoctx, &(view->background), STYLE_SHAPE);
	END_FUNC
}

/* draws the symbols */
void view_symbols_draw (struct view *view, cairo_t *cairoctx)
{
	START_FUNC
	view_draw(view, cairoctx, &(view->symbols), STYLE_SYMBOL);
	END_FUNC
}

/* update the keyboard positions */
void view_keyboards_set_pos(struct view *view, struct keyboard *over)
{
	START_FUNC
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
	END_FUNC
}

/* calculate the dimensions of Florence */
void view_set_dimensions(struct view *view)
{
	START_FUNC
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
	END_FUNC
}

/* get the key at position */
#ifdef ENABLE_RAMBLE
struct key *view_hit_get (struct view *view, gint x, gint y, enum key_hit *hit)
#else
struct key *view_hit_get (struct view *view, gint x, gint y)
#endif
{
	START_FUNC
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

	END_FUNC
	return key;
}

/* Create a window mask for transparent window for non-composited screen */
/* For composited screen, this function is useless, use alpha channel instead. */
void view_create_window_mask(struct view *view)
{
	START_FUNC
	Pixmap shape;
	cairo_surface_t *mask=NULL;
	cairo_t *cairoctx=NULL;
	Display *disp=(Display *)gdk_x11_get_default_xdisplay();

	if (settings_get_bool(SETTINGS_TRANSPARENT) && (!view->composite)) {
		shape=XCreatePixmap(disp, GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(view->window))),
			view->width, view->height, 1);
		mask=cairo_xlib_surface_create_for_bitmap(disp, shape,
			DefaultScreenOfDisplay(disp), view->width, view->height);
		cairoctx=cairo_create(mask);
		view_background_draw(view, cairoctx);
		cairo_set_source_rgba(cairoctx, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cairoctx, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cairoctx);
		cairo_set_source_surface(cairoctx, view->background, 0, 0);
		cairo_paint(cairoctx);
		XShapeCombineMask(disp, GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(view->window))),
			ShapeBounding, 0, 0, cairo_xlib_surface_get_drawable(mask), ShapeSet);
		cairo_destroy(cairoctx);
		cairo_surface_destroy(view->background);
		view->background=NULL;
		g_object_unref(G_OBJECT(mask));
		status_focus_zoom_set(view->status, FALSE);
	} else {
		XShapeCombineMask(disp, GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(view->window))),
			ShapeBounding, 0, 0, 0, ShapeSet);
		status_focus_zoom_set(view->status, TRUE);
	}
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
	END_FUNC
}

/* Triggered by gconf when the "transparent" parameter is changed. Calls view_create_window_mask */
void view_set_transparent(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	gboolean shown=gtk_widget_get_visible(GTK_WIDGET(view->window));
	gtk_widget_show(GTK_WIDGET(view->window));
	view_create_window_mask(view);
	if (!shown) gtk_widget_hide(GTK_WIDGET(view->window));
	END_FUNC
}

/* Triggered by gconf when the "decorated" parameter is changed. Decorates or undecorate the window. */
void view_set_decorated(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	gtk_window_set_decorated(view->window, settings_get_bool(SETTINGS_DECORATED));
	gtk_window_move(view->window, settings_get_int(SETTINGS_XPOS), settings_get_int(SETTINGS_YPOS));
	END_FUNC
}

/* Triggered by gconf when the "always_on_top" parameter is changed. 
   Change the window property to be always on top or not to be. */
void view_set_always_on_top(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	gtk_window_set_keep_above(view->window, settings_get_bool(SETTINGS_ALWAYS_ON_TOP));
	END_FUNC
}

/* Triggered by gconf when the "task_bar" parameter is changed. 
   Change the window hint to appear in the task bar or not. */
void view_set_task_bar(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	gtk_window_set_skip_taskbar_hint(view->window, !settings_get_bool(SETTINGS_TASK_BAR));
	END_FUNC
}

/* Triggered by gconf when the "resizable" parameter is changed.
   makes the window (not)resizable the window. */
void view_set_resizable(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	if (settings_get_bool(SETTINGS_RESIZABLE)) {
		gtk_widget_set_size_request(GTK_WIDGET(view->window), view->vwidth, view->vheight);
	}
	view_resize(view);
	END_FUNC
}

/* Triggered by gconf when a color parameter is changed. */
void view_redraw(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	style_update_colors(view->style);
	if ((!strcmp(key, "key")) || (!strcmp(key, "outline"))) {
		if (view->background) cairo_surface_destroy(view->background);
		view->background=NULL;
	} else if (!strncmp(key, "label", 5) || (!strcmp(key, "font")) || (!strcmp(key, "system_font"))) {
		if (view->symbols) cairo_surface_destroy(view->symbols);
		view->symbols=NULL;
	}
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
	END_FUNC
}

/* Triggered by gconf when the "resizable" parameter is changed.
   makes the window (not)resizable the window. */
void view_set_keep_ratio(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	if (settings_get_bool(SETTINGS_KEEP_RATIO)) {
		view->scaley=view->scalex;
	}
	view_resize(view);
	view_redraw(settings, key, user_data);
	END_FUNC
}

/* Redraw the key to the window */
void view_update(struct view *view, struct key *key, gboolean statechange)
{
	START_FUNC
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
			gdk_window_invalidate_rect(gtk_widget_get_window(GTK_WIDGET(view->window)), rect, TRUE);
		}
	}
	if (status_focus_get(view->status)) {
		if (!view->hand_cursor) {
			cursor=gdk_cursor_new(GDK_HAND2);
			gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(view->window)), cursor);
			view->hand_cursor=TRUE;
		}
	} else if (view->hand_cursor) {
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(view->window)), NULL);
		view->hand_cursor=FALSE;
	}
	END_FUNC
}

/* on screen change event: check for composite extension */
void view_screen_changed (GtkWidget *widget, GdkScreen *old_screen, struct view *view)
{
	START_FUNC
	GdkVisual *visual;
	if (gtk_widget_is_composited(widget)) {
		flo_info(_("X11 composite extension detected. Semi-transparency is enabled."));
		if (view) view->composite=TRUE;
		visual=gdk_screen_get_rgba_visual(gdk_screen_get_default());
		if (visual==NULL) visual=gdk_screen_get_system_visual(gdk_screen_get_default());
		gtk_widget_set_visual(widget, visual);
	} else { 
		flo_info(_("Your screen does not support alpha channel. Semi-transparency is disabled"));
		if (view) view->composite=FALSE;
	}
	END_FUNC
}

/* on configure events: record window position */
void view_configure (GtkWidget *window, GdkEventConfigure* pConfig, struct view *view)
{
	START_FUNC
	GdkRectangle rect;
	gint xpos, ypos;
	if (!gtk_widget_get_visible(window)) return;

	/* record window position */
	if (gtk_window_get_decorated(GTK_WINDOW(view->window)))
		gtk_window_get_position(GTK_WINDOW(view->window), &xpos, &ypos);
	else { xpos=pConfig->x; ypos=pConfig->y; }
	if (settings_get_int(SETTINGS_XPOS)!=xpos)
		settings_set_int(SETTINGS_XPOS, xpos);
	if (settings_get_int(SETTINGS_YPOS)!=ypos)
		settings_set_int(SETTINGS_YPOS, ypos);

	/* handle resize events */
	if ((pConfig->width!=view->width) || (pConfig->height!=view->height)) {
		if (settings_get_bool(SETTINGS_KEEP_RATIO)) {
			view->scalex=view->scaley=(gdouble)pConfig->width/view->vwidth;
		} else {
			view->scalex=(gdouble)pConfig->width/view->vwidth;
			view->scaley=(gdouble)pConfig->height/view->vheight;
		}
		if ((view->scalex>200.0)||(view->scaley>200.0))
			flo_warn(_("Window size out of range :%d, %d"), view->scalex, view->scaley);
		else {
			settings_set_double(SETTINGS_SCALEX, view->scalex, FALSE);
			settings_set_double(SETTINGS_SCALEY, view->scaley, FALSE);
		}
		view->width=pConfig->width; view->height=pConfig->height;
		if (view->background) cairo_surface_destroy(view->background);
		view->background=NULL;
		if (view->symbols) cairo_surface_destroy(view->symbols);
		view->symbols=NULL;
		view_create_window_mask(view);
		rect.x=0; rect.y=0;
		rect.width=pConfig->width; rect.height=pConfig->height;
		gtk_widget_size_allocate(GTK_WIDGET(view->window), &rect);
		gdk_window_invalidate_rect(gtk_widget_get_window(GTK_WIDGET(view->window)), &rect, TRUE);
		gdk_window_process_updates(gtk_widget_get_window(GTK_WIDGET(view->window)), FALSE);
	}

	gdk_window_configure_finished(gtk_widget_get_window(GTK_WIDGET(view->window)));
	END_FUNC
}

/* draw the background of the keyboard */
void view_draw_background (struct view *view, cairo_t *context)
{
	START_FUNC
	/* prepare the background */
	if (!view->background) {
		view_background_draw(view, context);
	}

	/* paint the background */
	cairo_set_operator(context, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(context, view->background, 0, 0);
	cairo_paint(context);
	END_FUNC
}

/* draw a list of keys (latched or locked keys) */
void view_draw_list (struct view *view, cairo_t *context, GList *list)
{
	START_FUNC
	struct keyboard *keyboard;
	struct key *key;
	while (list) {
		key=(struct key *)list->data;
		keyboard=(struct keyboard *)key_get_keyboard(key);
		keyboard_press_draw(keyboard, context, view->style, key, view->status);
		list=list->next;
	}
	END_FUNC
}

/* draw a single key (pressed or focused) */
void view_draw_key (struct view *view, cairo_t *context, struct key *key)
{
	START_FUNC
	struct keyboard *keyboard;
	if (key) {
		keyboard=(struct keyboard *)key_get_keyboard(key);
		keyboard_focus_draw(keyboard, context,
			(gdouble)cairo_xlib_surface_get_width(view->background),
			(gdouble)cairo_xlib_surface_get_height(view->background),
			view->style, key, view->status);
	}
	END_FUNC
}

/* on draw event: draws the keyboards to the window */
void view_expose (GtkWidget *window, cairo_t* context, struct view *view)
{
	START_FUNC
	enum key_state state;

	/* clear the area */
	if (settings_get_bool(SETTINGS_TRANSPARENT)) {
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
	if (view->composite && settings_get_double(SETTINGS_OPACITY)!=100.0) {
		if (settings_get_double(SETTINGS_OPACITY)>100.0 ||
			settings_get_double(SETTINGS_OPACITY)<1.0) {
			flo_error(_("Window opacity out of range (1.0 to 100.0): %f"),
				settings_get_double(SETTINGS_OPACITY));
		}
		cairo_set_source_rgba(context, 0.0, 0.0, 0.0,
			(100.0-settings_get_double(SETTINGS_OPACITY))/100.0);
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

	/* restore configure event handler. */
	if (!view->configure_handler) 
		view->configure_handler=g_signal_connect(G_OBJECT(view->window), "configure-event",
			G_CALLBACK(view_configure), view);
	END_FUNC
}

/* on keys changed events */
void view_on_keys_changed(gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	if (view->symbols) cairo_surface_destroy(view->symbols);
	view->symbols=NULL;
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
	END_FUNC
}

/* track the windows state changes */
void view_window_state (GtkWidget *window, GdkEventWindowState *event, struct view *view)
{
	START_FUNC
	gint is_iconified=gdk_window_get_state(gtk_widget_get_window(window))&GDK_WINDOW_STATE_ICONIFIED;
	if (is_iconified) {
		view_hide(view);
		gtk_window_deiconify(GTK_WINDOW(view->window));
	} 
	END_FUNC
}


/* Triggered by gconf when the "extensions" parameter is changed. */
void view_update_extensions(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	GSList *list=view->keyboards;
	struct keyboard *keyboard;

	/* Do not call configure signal handler */
	if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
	view->configure_handler=0;

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
	view_create_window_mask(view);
	status_focus_set(view->status, NULL);
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
	END_FUNC
}

/* Triggered by gconf when the "zoom" parameter is changed. */
void view_set_scalex(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	/* Do not call configure signal handler */
	if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
	view->configure_handler=0;
	view->scalex=settings_get_double(SETTINGS_SCALEX);
	if (settings_get_bool(SETTINGS_KEEP_RATIO)) view->scaley=view->scalex;
	view_update_extensions(settings, key, user_data);
	END_FUNC
}

/* Triggered by gconf when the "zoom" parameter is changed. */
void view_set_scaley(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	/* Do not call configure signal handler */
	if (view->configure_handler) g_signal_handler_disconnect(G_OBJECT(view->window), view->configure_handler);
	view->configure_handler=0;
	view->scaley=settings_get_double(SETTINGS_SCALEY);
	if (settings_get_bool(SETTINGS_KEEP_RATIO)) view->scalex=view->scaley;
	view_update_extensions(settings, key, user_data);
	END_FUNC
}

/* Triggered by gconf when the "opacity" parameter is changed. */
void view_set_opacity(GSettings *settings, gchar *key, gpointer user_data)
{
	START_FUNC
	struct view *view=(struct view *)user_data;
	gtk_widget_queue_draw(GTK_WIDGET(view->window));
	END_FUNC
}

/* get gtk window of the view */
GtkWindow *view_window_get (struct view *view)
{
	START_FUNC
	END_FUNC
	return view->window;
}

/* get gtk window of the view */
void view_status_set (struct view *view, struct status *status)
{
	START_FUNC
	view->status=status;
	END_FUNC
}

/* liberate all the memory used by the view */
void view_free(struct view *view)
{
	START_FUNC
	if (view->background) cairo_surface_destroy(view->background);
	if (view->symbols) cairo_surface_destroy(view->symbols);
	g_free(view);
	END_FUNC
}

/* create a view of florence */
struct view *view_new (struct status *status, struct style *style, GSList *keyboards)
{
	START_FUNC
	struct view *view=g_malloc(sizeof(struct view));
	if (!view) flo_fatal(_("Unable to allocate memory for view"));
	memset(view, 0, sizeof(struct view));

	view->status=status;
	view->style=style;
	view->keyboards=keyboards;
	view->scalex=settings_get_double(SETTINGS_SCALEX);
	view->scaley=settings_get_double(SETTINGS_SCALEY);
	view_set_dimensions(view);
	view->window=GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_keep_above(view->window, settings_get_bool(SETTINGS_ALWAYS_ON_TOP));
 	gtk_window_set_accept_focus(view->window, FALSE);
	gtk_window_set_skip_taskbar_hint(view->window, !settings_get_bool(SETTINGS_TASK_BAR));
	/* Remove resize grip since it is buggy */
	gtk_window_set_has_resize_grip(view->window, FALSE);
	view_resize(view);
	gtk_container_set_border_width(GTK_CONTAINER(view->window), 0);
	gtk_widget_set_events(GTK_WIDGET(view->window),
		GDK_EXPOSURE_MASK|GDK_POINTER_MOTION_HINT_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
		GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK|GDK_STRUCTURE_MASK|GDK_POINTER_MOTION_MASK);
	gtk_widget_set_app_paintable(GTK_WIDGET(view->window), TRUE);
	gtk_window_set_decorated(view->window, settings_get_bool(SETTINGS_DECORATED));
	gtk_window_move(view->window, settings_get_int(SETTINGS_XPOS), settings_get_int(SETTINGS_YPOS));
	/*g_signal_connect(gdk_keymap_get_default(), "keys-changed", G_CALLBACK(view_on_keys_changed), view);*/
	xkeyboard_register_events(status->xkeyboard, view_on_keys_changed, (gpointer)view);
	g_signal_connect(G_OBJECT(view->window), "screen-changed", G_CALLBACK(view_screen_changed), view);
	view->configure_handler=g_signal_connect(G_OBJECT(view->window), "configure-event",
		G_CALLBACK(view_configure), view);
	g_signal_connect(G_OBJECT(view->window), "draw", G_CALLBACK(view_expose), view);
	g_signal_connect(G_OBJECT(view->window), "window-state-event", G_CALLBACK(view_window_state), view);
	view_screen_changed(GTK_WIDGET(view->window), NULL, view);
	gtk_widget_show(GTK_WIDGET(view->window));
	view_create_window_mask(view);

	/* register settings callbacks */
	settings_changecb_register(SETTINGS_TRANSPARENT, view_set_transparent, view);
	settings_changecb_register(SETTINGS_DECORATED, view_set_decorated, view);
	settings_changecb_register(SETTINGS_RESIZABLE, view_set_resizable, view);
	settings_changecb_register(SETTINGS_ALWAYS_ON_TOP, view_set_always_on_top, view);
	settings_changecb_register(SETTINGS_TASK_BAR, view_set_task_bar, view);
	settings_changecb_register(SETTINGS_KEEP_RATIO, view_set_keep_ratio, view);
	settings_changecb_register(SETTINGS_SCALEX, view_set_scalex, view);
	settings_changecb_register(SETTINGS_SCALEY, view_set_scaley, view);
	settings_changecb_register(SETTINGS_OPACITY, view_set_opacity, view);
	settings_changecb_register(SETTINGS_EXTENSIONS, view_update_extensions, view);
	settings_changecb_register(SETTINGS_KEY, view_redraw, view);
	settings_changecb_register(SETTINGS_OUTLINE, view_redraw, view);
	settings_changecb_register(SETTINGS_LABEL, view_redraw, view);
	settings_changecb_register(SETTINGS_LABEL_OUTLINE, view_redraw, view);
	settings_changecb_register(SETTINGS_ACTIVATED, view_redraw, view);
	settings_changecb_register(SETTINGS_LATCHED, view_redraw, view);
	settings_changecb_register(SETTINGS_SYSTEM_FONT, view_redraw, view);
	settings_changecb_register(SETTINGS_FONT, view_redraw, view);

	/* set the window icon */
	tools_set_icon(view->window);
	END_FUNC
	return view;
}

/* Change the layout and style of the view and redraw */
void view_update_layout(struct view *view, struct style *style, GSList *keyboards)
{
	START_FUNC
	view->style=style;
	view->keyboards=keyboards;
	view_update_extensions(NULL, NULL, (gpointer)view);
	END_FUNC
}

