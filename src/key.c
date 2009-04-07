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
#include <math.h>
#include <string.h>
#include <cspi/spi.h>

#define PI M_PI

/* Instanciates a key
 * the key may have a static label which will be always drawn in place of the symbol */
struct key *key_new(struct layout *layout, struct style *style, XkbDescPtr xkb,
	XkbStateRec rec, void *userdata, struct status *status)
{
	struct layout_key *lkey=layoutreader_key_new(layout);
	struct key *key=NULL;
	
	if (lkey) {
		key=g_malloc(sizeof(struct key));
		memset(key, 0, sizeof(struct key));
		key->code=lkey->code;
		key->locker=XkbKeyAction(xkb, key->code, 0)?
			XkbKeyAction(xkb, key->code, 0)->type==XkbSA_LockMods:FALSE;
		key->modifier=xkb->map->modmap[key->code];
		if (key->modifier&rec.locked_mods) {
			key_set_pressed(key, TRUE);
			status_press(status, key);
		}
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

/* Send a key press event */
void key_press(struct key *key, struct status *status)
{
	GList *list=status_pressedkeys_get(status);
	struct key *pressed;
	while (list) {
		pressed=((struct key *)list->data);
		if (key_get_modifier(pressed) && !key_is_locker(pressed))
			SPI_generateKeyboardEvent(pressed->code, NULL, SPI_KEY_PRESS);
		list=list->next;
	}
	SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_PRESS);
	list=status_pressedkeys_get(status);
	while (list) {
		pressed=((struct key *)list->data);
		if (key_get_modifier(pressed) && !key_is_locker(pressed))
			SPI_generateKeyboardEvent(pressed->code, NULL, SPI_KEY_RELEASE);
		list=list->next;
	}
}

/* Send a key release event */
void key_release(struct key *key)
{
	SPI_generateKeyboardEvent(key->code, NULL, SPI_KEY_RELEASE);
}

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
	cairo_scale(cairoctx, 1.0/z, 1.0/z);
	cairo_mask_surface(cairoctx, style_shape_get_mask(key->shape, (guint)(z*key->w), (guint)(z*key->h)), 0.0, 0.0);
	cairo_restore(cairoctx);
}

/* Draw a colored layer on top of the shape */
void key_color_draw(struct key *key, struct style *style, cairo_t *cairoctx, gdouble z, enum style_colours c)
{
	cairo_save(cairoctx);
	style_cairo_set_color(style, cairoctx, c);
	cairo_scale(cairoctx, 1.0/z, 1.0/z);
	cairo_mask_surface(cairoctx, style_shape_get_mask(key->shape, (guint)(z*key->w), (guint)(z*key->h)), 0.0, 0.0);
	cairo_restore(cairoctx);
}

/* Draw the shape of the key to the cairo surface. */
void key_shape_draw(struct key *key, struct style *style, cairo_t *cairoctx)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	style_shape_draw(key->shape, cairoctx, key->w, key->h);
	cairo_restore(cairoctx);
}

/* Draw the symbol of the key to the cairo surface. The symbol drawn on the key depends on the modifier */
void key_symbol_draw(struct key *key, struct style *style, cairo_t *cairoctx, GdkModifierType mod)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	style_symbol_draw(style, cairoctx, key_getKeyval(key, mod), key->w, key->h);
	cairo_restore(cairoctx);
}

/* Draw the focus notifier to the cairo surface. */
void key_focus_draw(struct key *key, struct style *style, cairo_t *cairoctx, gdouble z, gdouble timer)
{
	cairo_save(cairoctx);
	cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	if (timer>0.0) key_timer_draw(key, style, cairoctx, z, timer);
	else key_color_draw(key, style, cairoctx, z, STYLE_MOUSE_OVER_COLOR);
	cairo_restore(cairoctx);
}

/* Draw the key press notifier to the cairo surface. */
void key_press_draw(struct key *key, struct style *style, cairo_t *cairoctx, gdouble z)
{
	if (key->pressed) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
		key_color_draw(key, style, cairoctx, z, STYLE_ACTIVATED_COLOR);
		cairo_restore(cairoctx);
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
