/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009 François Agrech

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
#include "key.h"
#include "trace.h"
#include "settings.h"
#include <math.h>
#include <string.h>
#include <gdk/gdkx.h>
#ifdef ENABLE_AT_SPI
#include <cspi/spi.h>
#endif
#ifdef ENABLE_XTST
#include <X11/extensions/XTest.h>
#endif

#define PI M_PI

/* get keyval according to modifier */
guint key_getKeyval(struct key *key, GdkModifierType mod)
{
	guint keyval=0;
	if (!gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(), key->code, mod, 0,
		&keyval, NULL, NULL, NULL)) {
		keyval=0;
		/*flo_warn(_("Unable to translate keyboard state: keycode=%d modifiers=%d"), key->code, mod);*/
	}
	return keyval;
}

/* Instanciates a key
 * the key may have a static label which will be always drawn in place of the symbol */
#ifdef ENABLE_XKB
struct key *key_new(struct layout *layout, struct style *style, XkbDescPtr xkb,
	XkbStateRec rec, void *userdata, struct status *status)
#else
struct key *key_new(struct layout *layout, struct style *style, void *userdata,
	struct status *status)
#endif
{
	struct layout_key *lkey=layoutreader_key_new(layout);
	struct key *key=NULL;
#ifndef ENABLE_XKB
	gchar *symbolname;
#endif
	
	if (lkey) {
		key=g_malloc(sizeof(struct key));
		memset(key, 0, sizeof(struct key));
		key->code=lkey->code;
		key->type=lkey->type;
#ifdef ENABLE_XKB
		key->modifier=xkb->map->modmap[key->code];
		if (XkbKeyAction(xkb, key->code, 0)) {
			switch (XkbKeyAction(xkb, key->code, 0)->type) {
				case XkbSA_LockMods:key->locker=TRUE; break;
				case XkbSA_SetMods:key->modifier=XkbKeyAction(xkb, key->code, 0)->mods.mask;
					break;
			}
		}
		if (key->modifier&rec.locked_mods) {
			key_set_pressed(key, TRUE);
			status_press(status, key);
		}
#else
		symbolname=gdk_keyval_name(key_getKeyval(key, 0));
		if (!strcmp(symbolname, "Caps_Lock")) {
			key->locker=TRUE;
			key->modifier=2;
		} else if (!strcmp(symbolname, "Num_Lock")) {
			key->locker=TRUE;
			key->modifier=16;
		} else if (!strcmp(symbolname, "Shift_L") ||
			!strcmp(symbolname, "Shift_R")) {
			key->modifier=1;
		} else if ( !strcmp(symbolname, "Alt_L") ||
			!strcmp(symbolname, "Alt_R")) {
			key->modifier=8;
		} else if (!strcmp(symbolname, "Control_L") ||
			!strcmp(symbolname, "Control_R")) {
			key->modifier=4;
		} else if (!strcmp(symbolname, "ISO_Level3_Shift")) {
			key->modifier=128;
		}
#endif
		key->shape=style_shape_get(style, lkey->shape);
		key->x=lkey->pos.x;
		key->y=lkey->pos.y;
		key->w=lkey->size.w==0.0?2.0:lkey->size.w;
		key->h=lkey->size.h==0.0?2.0:lkey->size.h;
		key->userdata=userdata;
		layoutreader_key_free(lkey);
		flo_debug(_("[new key] code=%d x=%f y=%f w=%f h=%f"),
			key->code, key->x, key->y, key->w, key->h);
	}

	return key;
}

/* liberate memory used by the key */
void key_free(struct key *key)
{
	g_free(key);
}

/* handle X11 errors */
int key_error_handler(Display *my_dpy, XErrorEvent *event)
{
	flo_warn(_("Unable to focus window."));
	return 0;
}

/* send a simple key press event */
void key_simple_press(unsigned int code, struct status *status)
{
	struct status_focus *focus;
	int (*old_handler)(Display *, XErrorEvent *);
	if ((focus=status_w_focus_get(status))) {
		old_handler=XSetErrorHandler(key_error_handler);
		XSetInputFocus(gdk_x11_get_default_xdisplay(), focus->w,
				focus->revert_to, CurrentTime);
		XSync(gdk_x11_get_default_xdisplay(), FALSE);
		XSetErrorHandler(old_handler);
	}
#ifdef ENABLE_XTST
	if (status_spi_is_enabled(status))
#ifdef ENABLE_AT_SPI
		SPI_generateKeyboardEvent(code, NULL, SPI_KEY_PRESS);
#else
		flo_fatal(_("Unreachable code"));
#endif
	else XTestFakeKeyEvent(
		(Display *)gdk_x11_get_default_xdisplay(),
		code, TRUE, 0);
#else
#ifdef ENABLE_AT_SPI
	SPI_generateKeyboardEvent(code, NULL, SPI_KEY_PRESS);
#else
#error _("Neither at-spi nor Xtest is compiled. You should compile one.")
#endif
#endif
}

/* send a simple key release event */
void key_simple_release(unsigned int code, struct status *status)
{
#ifdef ENABLE_XTST
	if (status_spi_is_enabled(status))
#ifdef ENABLE_AT_SPI
		SPI_generateKeyboardEvent(code, NULL, SPI_KEY_RELEASE);
#else
		flo_fatal(_("Unreachable code"));
#endif
	else XTestFakeKeyEvent(
		(Display *)gdk_x11_get_default_xdisplay(),
		code, FALSE, 0);
#else
#ifdef ENABLE_AT_SPI
	SPI_generateKeyboardEvent(code, NULL, SPI_KEY_RELEASE);
#else
#error _("Neither at-spi nor Xtest is compiled. You should compile one.")
#endif
#endif
}

/* Send a key press event */
void key_press(struct key *key, struct status *status)
{
	GList *list=NULL;
	struct key *pressed;
	if (key->type) {
		key->pressed=TRUE;
		status_key_press_update(status, key);
		switch (key->type) {
			case LAYOUT_MOVE: status_set_moving(status, TRUE); break;
			case LAYOUT_BIGGER:
			case LAYOUT_SMALLER:
			case LAYOUT_CONFIG:
			case LAYOUT_CLOSE: break;
			default: flo_warn(_("unknown action key type pressed = %d"), key->type);
		}
	} else {
		list=status_pressedkeys_get(status);
		while (list) {
			pressed=((struct key *)list->data);
			if (key_get_modifier(pressed) && !key_is_locker(pressed))
				key_simple_press(pressed->code, status);
			list=list->next;
		}
		key_simple_press(key->code, status);
		list=status_pressedkeys_get(status);
		while (list) {
			pressed=((struct key *)list->data);
			if (key_get_modifier(pressed) && !key_is_locker(pressed))
				key_simple_release(pressed->code, status);
			list=list->next;
		}
	}
}

/* Send a key release event */
void key_release(struct key *key, struct status *status)
{
	if (key->type && key->pressed) {
		switch (key->type) {
			case LAYOUT_CLOSE: gtk_main_quit(); break;
			case LAYOUT_CONFIG: settings(); break;
			case LAYOUT_MOVE: status_set_moving(status, FALSE); break;
			case LAYOUT_BIGGER: settings_double_set("window/zoom",
				settings_double_get("window/zoom")*1.05, TRUE); break;
			case LAYOUT_SMALLER: settings_double_set("window/zoom",
				settings_double_get("window/zoom")*0.95, TRUE); break;
			default: flo_warn(_("unknown action key type released = %d"), key->type);
		}
		key->pressed=FALSE;
		status_key_release_update(status, key);
	}
	else key_simple_release(key->code, status);
}

/* Draw the representation of the auto-click timer on the key
 * value is between 0 and 1 */
void key_timer_draw(struct key *key, struct style *style, cairo_t *cairoctx, gdouble z, double value)
{
	cairo_save(cairoctx);
	style_cairo_set_color(style, cairoctx, STYLE_MOUSE_OVER_COLOR);
	cairo_move_to(cairoctx, key->w/2.0, key->h/2.0);
	cairo_line_to(cairoctx, key->w/2.0, 0.0);
	if (value>0.125) cairo_line_to(cairoctx, key->w, 0.0);
	if (value>0.375) cairo_line_to(cairoctx, key->w, key->h);
	if (value>0.625) cairo_line_to(cairoctx, 0.0, key->h);
	if (value>0.875) cairo_line_to(cairoctx, 0.0, 0.0);
	if (value<0.125 || value>0.875) cairo_line_to(cairoctx, key->w/2+(key->w/2*tan(value*2.0*PI)), 0.0);
	else if (value>0.125 && value<0.375) cairo_line_to(cairoctx, key->w, key->h/2+(key->h/2*tan((value-0.25)*2.0*PI)));
	else if (value>0.375 && value<0.625) cairo_line_to(cairoctx, key->w/2-(key->w/2*tan((value-0.5)*2.0*PI)), key->h);
	else if (value>0.625 && value<0.875) cairo_line_to(cairoctx, 0.0, key->h/2-(key->h/2*tan((value-0.75)*2.0*PI)));
	cairo_close_path(cairoctx);
	cairo_clip(cairoctx);
	style_shape_draw(style, key->shape, cairoctx, key->w, key->h, STYLE_MOUSE_OVER_COLOR);
	cairo_restore(cairoctx);
}

/* Draw the shape of the key to the cairo surface. */
void key_shape_draw(struct key *key, struct style *style, cairo_t *cairoctx)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	style_shape_draw(style, key->shape, cairoctx, key->w, key->h, STYLE_KEY_COLOR);
	cairo_restore(cairoctx);
}

/* Draw the symbol of the key to the cairo surface. The symbol drawn on the key depends on the modifier */
void key_symbol_draw(struct key *key, struct style *style,
	cairo_t *cairoctx, GdkModifierType mod, gdouble size)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w*size/2.0), key->y-(key->h*size/2.0));
	cairo_scale(cairoctx, size, size);
	if (key->type==LAYOUT_NORMAL) style_symbol_draw(style, cairoctx, key_getKeyval(key, mod), key->w, key->h);
	else style_symbol_type_draw(style, cairoctx, key->type, key->w, key->h);
	cairo_restore(cairoctx);
}

/* Draw the focus notifier to the cairo surface. */
void key_focus_draw(struct key *key, struct style *style, cairo_t *cairoctx, gdouble z, struct status *status)
{
	gdouble focus_zoom=status_focus_zoom_get(status)?settings_double_get("style/focus_zoom"):1.0;
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w*focus_zoom/2.0), key->y-(key->h*focus_zoom/2.0));
	cairo_scale(cairoctx, focus_zoom, focus_zoom);
	if (status_timer_get(status)>0.0) {
		style_shape_draw(style, key->shape, cairoctx, key->w, key->h,
			key->pressed?STYLE_ACTIVATED_COLOR:STYLE_KEY_COLOR);
		key_timer_draw(key, style, cairoctx, z*focus_zoom, status_timer_get(status));
	}
	else style_shape_draw(style, key->shape, cairoctx, key->w, key->h,
		key->pressed?STYLE_ACTIVATED_COLOR:STYLE_MOUSE_OVER_COLOR);
	cairo_restore(cairoctx);
	key_symbol_draw(key, style, cairoctx, status->globalmod, focus_zoom);
}

/* Draw the key press notifier to the cairo surface. */
void key_press_draw(struct key *key, struct style *style, cairo_t *cairoctx, gdouble z, struct status *status)
{
	if (key->pressed && (key!=status_focus_get(status))) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
		style_shape_draw(style, key->shape, cairoctx, key->w, key->h, STYLE_ACTIVATED_COLOR);
		cairo_restore(cairoctx);
		key_symbol_draw(key, style, cairoctx, status->globalmod, 1.0);
	}
}

/* getters and setters */
void key_set_pressed(struct key *key, gboolean pressed) { key->pressed=pressed; }
gboolean key_is_pressed(struct key *key) { return key->pressed; }
gboolean key_is_locker(struct key *key) { return key->locker; }
void *key_get_userdata(struct key *key) { return key->userdata; }
GdkModifierType key_get_modifier(struct key *key) { return key->modifier; }

/* return if key is it at position */
gboolean key_hit(struct key *key, gint x, gint y, gdouble z)
{
	gint x1=z*(key->x-(key->w/2.0));
	gint y1=z*(key->y-(key->h/2.0));
	gint x2=x1+(z*key->w);
	gint y2=y1+(z*key->h);
	gboolean ret=FALSE;
	if ((x>=x1) && (x<=x2) && (y>=y1) && (y<=y2)) {
		ret=style_shape_test(key->shape, x-x1, y-y1, key->w*z, key->h*z);
	}
	return ret;
}
