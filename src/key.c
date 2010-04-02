/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009, 2010 François Agrech

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
#include <cairo/cairo-xlib.h>

#define PI M_PI

/* Instanciates a key
 * the key may have a static label which will be always drawn in place of the symbol */
struct key *key_new(struct layout *layout, struct style *style, struct xkeyboard *xkeyboard, void *userdata)
{
	struct layout_key *lkey=layoutreader_key_new(layout);
	struct key *key=NULL;
	guint n=0;
	struct layout_modifier **lmod;
	struct key_action **actions;
	
	if (lkey) {
		key=g_malloc(sizeof(struct key));
		memset(key, 0, sizeof(struct key));
		key->code=lkey->code;
		key->state=KEY_RELEASED;
		lmod=lkey->actions;
		if (lmod) while(*(lmod++)) n++;
		if (n) {
			actions=(key->actions=g_malloc(sizeof(struct key_modifier *)*(n+1)));
			lmod=lkey->actions;
			while (*lmod) {
				*actions=g_malloc(sizeof(struct key_action));
				(*actions)->modifier=(GdkModifierType)((*lmod)->mod);
				(*actions)->type=(*lmod)->type;
				lmod++; actions++;
			}
			*actions=NULL;
		}
		xkeyboard_key_properties_get(xkeyboard, key->code, &(key->modifier), &(key->locker));
		key->shape=style_shape_get(style, lkey->shape);
		key->x=lkey->pos.x;
		key->y=lkey->pos.y;
		key->w=lkey->size.w==0.0?2.0:lkey->size.w;
		key->h=lkey->size.h==0.0?2.0:lkey->size.h;
		key->userdata=userdata;
		layoutreader_key_free(lkey);
		flo_debug("[new key] code=%d x=%f y=%f w=%f h=%f",
			key->code, key->x, key->y, key->w, key->h);
	}

	return key;
}

/* liberate memory used by the key */
void key_free(struct key *key)
{
	struct key_action **action;
	if (key->actions) {
		action=key->actions;
		while (*(action++)) g_free(*action);
		g_free(key->actions);
	}
	g_free(key);
}

/* send a simple event: press (pressed=TRUE) or release (pressed=FALSE) */
void key_event(unsigned int code, gboolean pressed, gboolean spi_enabled)
{
#ifdef ENABLE_XTST
	if (spi_enabled)
#ifdef ENABLE_AT_SPI
		SPI_generateKeyboardEvent(code, NULL, pressed?SPI_KEY_PRESS:SPI_KEY_RELEASE);
#else
		flo_fatal(_("Unreachable code"));
#endif
	else XTestFakeKeyEvent(
		(Display *)gdk_x11_get_default_xdisplay(),
		code, pressed, 0);
#else
#ifdef ENABLE_AT_SPI
	SPI_generateKeyboardEvent(code, NULL, pressed?SPI_KEY_PRESS:SPI_KEY_RELEASE);
#else
#error _("Neither at-spi nor Xtest is compiled. You should compile one.")
#endif
#endif
}

/* find the right action according to the modifier */
struct key_action *key_action_find(struct key_action **actions, GdkModifierType mod)
{
	struct key_action *action=NULL;
	guint score=0;
	while (*actions) {
		if ( (!action) || (score<(mod&((*actions)->modifier))) ) {
			action=*actions;
			score=mod&((*actions)->modifier);
		}
		actions++;
	}
	return action;
}

/* Send a key press event. */
void key_press(struct key *key, struct status *status)
{
	struct key_action *action=NULL;
	if (key->actions) {
		action=key_action_find(key->actions, status_globalmod_get(status));
		switch (action->type) {
			case LAYOUT_MOVE: status->moving=TRUE; break;
			case LAYOUT_BIGGER:
			case LAYOUT_SMALLER:
			case LAYOUT_CONFIG:
			case LAYOUT_CLOSE:
			case LAYOUT_REDUCE:
			case LAYOUT_SWITCH: break;
			default: flo_warn(_("unknown action key type pressed = %d"), action->type);
		}
	} else key_event(key->code, TRUE, status->spi);
}

/* Send a key release event. */
void key_release(struct key *key, struct status *status)
{
	struct key_action *action=NULL;
	if (key->actions) {
		action=key_action_find(key->actions, status_globalmod_get(status));
		switch (action->type) {
			case LAYOUT_CLOSE: gtk_main_quit(); break;
			case LAYOUT_REDUCE: view_hide(status->view); break;
			case LAYOUT_CONFIG: settings(); break;
			case LAYOUT_MOVE: status->moving=FALSE; break;
			case LAYOUT_BIGGER: settings_double_set("window/zoom",
				settings_double_get("window/zoom")*1.05, TRUE); break;
			case LAYOUT_SMALLER: settings_double_set("window/zoom",
				settings_double_get("window/zoom")*0.95, TRUE); break;
			case LAYOUT_SWITCH:
				xkeyboard_layout_change(status->xkeyboard); break;
			default: flo_warn(_("unknown action key type released = %d"), action->type);
		}
	}
	else key_event(key->code, FALSE, status->spi);
}

/* Draw the representation of the auto-click timer on the key
 * value is between 0 and 1 */
void key_timer_draw(struct key *key, struct style *style, cairo_t *cairoctx, double value)
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
	cairo_t *cairoctx, struct status *status, gboolean use_matrix)
{
	struct key_action *action=NULL;
	if (!use_matrix) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	}
	if (key->actions) {
		action=key_action_find(key->actions, status_globalmod_get(status));
		if (action->type==LAYOUT_SWITCH)
			style_draw_text(style, cairoctx,
				xkeyboard_next_layout_get(status->xkeyboard), key->w, key->h);
		else
			style_symbol_type_draw(style, cairoctx, action->type, key->w, key->h);
	} else {
		style_symbol_draw(style, cairoctx,
			xkeyboard_getKeyval(status->xkeyboard, key->code, status_globalmod_get(status)),
			key->w, key->h);
	}
	if (!use_matrix) cairo_restore(cairoctx);
}

/* Draw the focus notifier to the cairo surface. */
void key_focus_draw(struct key *key, struct style *style, cairo_t *cairoctx,
	gdouble width, gdouble height, struct status *status)
{
	enum style_colours color;
	cairo_matrix_t matrix;
	gdouble focus_zoom=status_focus_zoom_get(status)?settings_double_get("style/focus_zoom"):1.0;
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w*focus_zoom/2.0), key->y-(key->h*focus_zoom/2.0));
	cairo_scale(cairoctx, focus_zoom, focus_zoom);

	/* Make sure all of the key is displayed inside the window */
	cairo_get_matrix(cairoctx, &matrix);
	if (((matrix.xx*key->w)+matrix.x0)>width) matrix.x0=width-(matrix.xx*key->w);
	else if (matrix.x0<0.0) matrix.x0=0.0;
	if (((matrix.yy*key->h)+matrix.y0)>height) matrix.y0=height-(matrix.yy*key->h);
	else if (matrix.y0<0.0) matrix.y0=0.0;
	cairo_set_matrix(cairoctx, &matrix);

	/* determine the color of th key */
	switch (key->state) {
		case KEY_LOCKED:
		case KEY_PRESSED: color=STYLE_ACTIVATED_COLOR; break;
		case KEY_RELEASED: color=STYLE_KEY_COLOR; break;
		case KEY_LATCHED: color=STYLE_LATCHED_COLOR; break;
		default: flo_warn(_("unknown key type: %d"), key->state);
			 color=STYLE_KEY_COLOR; break;
	}

	if (status_timer_get(status)>0.0) {
		style_shape_draw(style, key->shape, cairoctx, key->w, key->h, color);
		key_timer_draw(key, style, cairoctx, status_timer_get(status));
	} else style_shape_draw(style, key->shape, cairoctx, key->w, key->h,
		key->state==KEY_RELEASED?STYLE_MOUSE_OVER_COLOR:color);
	key_symbol_draw(key, style, cairoctx, status, TRUE);
	cairo_restore(cairoctx);
}

/* Draw the key press notifier to the cairo surface. */
void key_press_draw(struct key *key, struct style *style, cairo_t *cairoctx, struct status *status)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	style_shape_draw(style, key->shape, cairoctx, key->w, key->h,
		key->state==KEY_LATCHED?STYLE_LATCHED_COLOR:STYLE_ACTIVATED_COLOR);
	key_symbol_draw(key, style, cairoctx, status, TRUE);
	cairo_restore(cairoctx);
}

/* getters and setters */
void key_state_set(struct key *key, enum key_state state) { key->state=state; }
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
