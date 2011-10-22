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
#include "status.h"
#include <math.h>
#include <string.h>
#include <gdk/gdkx.h>
#ifdef ENABLE_AT_SPI
#define AT_SPI
#include <dbus/dbus.h>
#include <cspi/spi.h>
#endif
#ifdef ENABLE_AT_SPI2
#define AT_SPI
#include <atspi/atspi.h>
#endif
#ifdef ENABLE_XTST
#include <X11/extensions/XTest.h>
#endif
#include <cairo/cairo-xlib.h>

#define PI M_PI
#ifdef ENABLE_RAMBLE
#define BORDER_THRESHOLD 0.2
#endif

/* Parse string into key type enumeration */
enum key_action_type key_action_type_get(gchar *str)
{
	enum key_action_type ret=KEY_UNKNOWN;
	if (!strcmp(str, "close")) ret=KEY_CLOSE;
	else if (!strcmp(str, "reduce")) ret=KEY_REDUCE;
	else if (!strcmp(str, "config")) ret=KEY_CONFIG;
	else if (!strcmp(str, "move")) ret=KEY_MOVE;
	else if (!strcmp(str, "bigger")) ret=KEY_BIGGER;
	else if (!strcmp(str, "smaller")) ret=KEY_SMALLER;
	else if (!strcmp(str, "switch")) ret=KEY_SWITCH;
	else if (!strcmp(str, "extend")) ret=KEY_EXTEND;
	else if (!strcmp(str, "unextend")) ret=KEY_UNEXTEND;
	else flo_error(_("Unknown action key type %s"), str);
	return ret;
}

/* Append a new modifier to a key (object) */
void key_modifier_append (struct layout_modifier *mod, void *object, void *xkb)
{
	struct key *key=(struct key*) object;
	struct xkeyboard *xkeyboard=(struct xkeyboard *)xkb;
	struct key_mod *keymod=g_malloc(sizeof(struct key_mod));
	struct key_action *action;
	memset(keymod, 0, sizeof(struct key_mod));
	keymod->modifier=mod->mod;
	if (mod->code) {
		keymod->type=KEY_CODE;
		keymod->data=g_malloc(sizeof(struct key_code));
		memset(keymod->data, 0, sizeof(struct key_code));
		((struct key_code *)keymod->data)->code=mod->code;
		xkeyboard_key_properties_get(xkeyboard, mod->code,
			&(((struct key_code *)keymod->data)->modifier),
			&(((struct key_code *)keymod->data)->locker));
	} else {
		keymod->type=KEY_ACTION;
		keymod->data=g_malloc(sizeof(struct key_action));
		memset(keymod->data, 0, sizeof(struct key_action));
		action=(struct key_action *)keymod->data;
		action->type=key_action_type_get((gchar *)mod->action);
		if (mod->argument) {
			action->argument=g_malloc(sizeof(gchar)*(strlen((char *)mod->argument)+1));
			strcpy(action->argument, (char *)mod->argument);
		}
	}
	key->mods=g_slist_append(key->mods, keymod);
}

/* Instanciates a key
 * the key may have a static label which will be always drawn in place of the symbol */
struct key *key_new(struct layout *layout, struct style *style, struct xkeyboard *xkeyboard, void *keyboard)
{
	struct key *key;
	struct layout_key *lkey;
	
	key=g_malloc(sizeof(struct key));
	memset(key, 0, sizeof(struct key));
	lkey=layoutreader_key_new(layout, key_modifier_append, (void *)key, (void *)xkeyboard);
	if (lkey) {
		key->state=KEY_RELEASED;
		key->shape=style_shape_get(style, lkey->shape);
		key->x=lkey->pos.x;
		key->y=lkey->pos.y;
		key->w=lkey->size.w==0.0?2.0:lkey->size.w;
		key->h=lkey->size.h==0.0?2.0:lkey->size.h;
		key->keyboard=keyboard;
		layoutreader_key_free(lkey);
		flo_debug("[new key] x=%f y=%f w=%f h=%f",
			key->x, key->y, key->w, key->h);
	} else {
		g_free(key);
		key=NULL;
	}

	return key;
}

/* liberate memory used by the key */
void key_free(struct key *key)
{
	struct key_action *action;
	struct key_mod *mod;
	GSList *list=key->mods;
	while (list) {
		mod=(struct key_mod *)list->data;
		if (mod->type==KEY_ACTION) {
			action=(struct key_action *)mod->data;
			if (action->argument) g_free(action->argument);
		}
		g_free(mod->data);
		g_free(list->data);
		list=list->next;
	}
	g_slist_free(key->mods);
	g_free(key);
}

/* send a simple event: press (pressed=TRUE) or release (pressed=FALSE) */
void key_event(unsigned int code, gboolean pressed, gboolean spi_enabled)
{
#ifdef ENABLE_XTST
	if (spi_enabled)
#ifdef AT_SPI
#ifdef ENABLE_AT_SPI2
		atspi_generate_keyboard_event(code, NULL, pressed?ATSPI_KEY_PRESS:ATSPI_KEY_RELEASE, NULL);
#else
		SPI_generateKeyboardEvent(code, NULL, pressed?SPI_KEY_PRESS:SPI_KEY_RELEASE);
#endif
#else
		flo_fatal(_("Unreachable code"));
#endif
	else XTestFakeKeyEvent(
		(Display *)gdk_x11_get_default_xdisplay(),
		code, pressed, 0);
#else
#ifdef AT_SPI
#ifdef ENABLE_AT_SPI2
	atspi_generate_keyboard_event(code NULL, pressed?ATSPI_KEY_PRESS:ATSPI_KEY_RELEASE);
#else
	SPI_generateKeyboardEvent(code, NULL, pressed?SPI_KEY_PRESS:SPI_KEY_RELEASE);
#endif
#else
#error _("Neither at-spi nor Xtest is compiled. You should compile one.")
#endif
#endif
}

/* find the right modifier according to the global modifier */
struct key_mod *key_mod_find(struct key *key, GdkModifierType mod)
{
	GSList *list=key->mods;
	struct key_mod *keymod;
	guint score=0;
	if (!list) flo_fatal(_("key %p has no modification."), key);
	keymod=(struct key_mod *)list->data;
	while (list) {
		if (score<(mod&(((struct key_mod *)list->data)->modifier))) {
			keymod=(struct key_mod *)list->data;
			score=mod&(((struct key_mod *)list->data)->modifier);
		}
		list=list->next;
	}
	return keymod;
}

/* event triggered when the "extend" key is pressed 
 * activate the extension mentioned in the action argument */
void key_extend(struct key_action *action) {
	gboolean activated=FALSE;
	gchar *newexts=NULL;
	gchar *allextstr=NULL;
	gchar **extstrs=NULL;
	gchar **extstr=NULL;
	if ((allextstr=settings_get_string("layout/extensions"))) {
		extstrs=g_strsplit(allextstr, ":", -1);
		extstr=extstrs;
		while (extstr && *extstr) {
			if (!strcmp(action->argument, *(extstr++))) { activated=TRUE; break; }
		}
		g_strfreev(extstrs);
		if (!activated) {
			newexts=g_malloc(sizeof(gchar)*(strlen(allextstr)+strlen(action->argument)+2));
			sprintf(newexts, "%s:%s", allextstr, action->argument);
			settings_string_set("layout/extensions", newexts);
			g_free(newexts);
		}
		g_free(allextstr);
	}
}

/* event triggered when the "extend" key is pressed 
 * activate the extension mentioned in the action argument */
void key_unextend(struct key_action *action) {
	gboolean activated=FALSE;
	gboolean started=FALSE;
	gchar *newexts=NULL;
	gchar *allextstr=NULL;
	gchar **extstrs=NULL;
	gchar **extstr=NULL;
	if ((allextstr=settings_get_string("layout/extensions"))) {
		newexts=g_malloc(sizeof(gchar)*(strlen(allextstr)));
		newexts[0]='\0';
		extstrs=g_strsplit(allextstr, ":", -1);
		extstr=extstrs;
		while (extstr && *extstr) {
			if (!strcmp(action->argument, *extstr)) {
				activated=TRUE;
			} else {
				if (started) strcat(newexts, ":");
				strcat(newexts, *(extstr));
				started=TRUE;
			}
			extstr++;
		}
		g_strfreev(extstrs);
		if (activated) {
			settings_string_set("layout/extensions", newexts);
		}
		g_free(newexts);
		g_free(allextstr);
	}
}

/* Send a key press event. */
void key_press(struct key *key, struct status *status)
{
	struct key_mod *mod=key_mod_find(key, status_globalmod_get(status));
	struct key_action *action;
	if (mod) {
		switch (mod->type) {
			case KEY_CODE:
				key_event(((struct key_code *)mod->data)->code, TRUE, status->spi);
				break;
			case KEY_ACTION:
				action=(struct key_action *)mod->data;
				switch (action->type) {
					case KEY_MOVE: status->moving=TRUE; break;
					case KEY_BIGGER:
					case KEY_SMALLER:
					case KEY_CONFIG:
					case KEY_CLOSE:
					case KEY_REDUCE:
					case KEY_SWITCH:
					case KEY_EXTEND:
					case KEY_UNEXTEND: break;
					default: flo_warn(_("unknown action key type pressed = %d"), action->type);
				}
				break;
			default: flo_warn(_("unknown key type pressed."));
		}
	} else flo_warn(_("pressed key has no action associated."));
}

/* Send a key release event. */
void key_release(struct key *key, struct status *status)
{
	struct key_mod *mod=key_mod_find(key, status_globalmod_get(status));
	struct key_action *action;
	if (mod) {
		switch (mod->type) {
			case KEY_CODE:
				key_event(((struct key_code *)mod->data)->code, FALSE, status->spi);
				break;
			case KEY_ACTION:
				action=(struct key_action *)mod->data;
				switch (action->type) {
					case KEY_CLOSE: gtk_main_quit(); break;
					case KEY_REDUCE: view_hide(status->view); break;
					case KEY_CONFIG: settings(); break;
					case KEY_MOVE: status->moving=FALSE; break;
					case KEY_BIGGER: settings_double_set("window/scalex",
							settings_double_get("window/scalex")*1.05, TRUE);
						settings_double_set("window/scaley",
							settings_double_get("window/scaley")*1.05, TRUE);
						break;
					case KEY_SMALLER: settings_double_set("window/scalex",
							settings_double_get("window/scalex")*0.95, TRUE);
						settings_double_set("window/scaley",
							settings_double_get("window/scaley")*0.95, TRUE);
					case KEY_SWITCH:
						xkeyboard_layout_change(status->xkeyboard); break;
					case KEY_EXTEND: key_extend(action); break;
					case KEY_UNEXTEND: key_unextend(action); break;
					default: flo_warn(_("unknown action key type released = %d"), action->type);
				}
				break;
			default: flo_warn(_("unknown key type released."));
		}
	} else flo_warn(_("released key has no action associated."));
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
	struct key_mod *mod=key_mod_find(key, status_globalmod_get(status));
	struct key_action *action;

	if (!use_matrix) {
		cairo_save(cairoctx);
		cairo_translate(cairoctx, key->x-(key->w/2.0), key->y-(key->h/2.0));
	}

	switch (mod->type) {
		case KEY_CODE:
			style_symbol_draw(style, cairoctx,
				xkeyboard_getKeyval(status->xkeyboard,
					((struct key_code *)mod->data)->code,
					status_globalmod_get(status)),
				key->w, key->h);
			break;
		case KEY_ACTION:
			action=(struct key_action *)mod->data;
			if (action->type==KEY_SWITCH)
				style_draw_text(style, cairoctx,
					xkeyboard_next_layout_get(status->xkeyboard), key->w, key->h);
			else
				style_symbol_type_draw(style, cairoctx, action->type, key->w, key->h);
			break;
		default: flo_warn(_("unknown key type to draw."));
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
/* note: a locker is a key which modification 0 is a locker key code*/
gboolean key_is_locker(struct key *key) {
	return ((struct key_mod *)key->mods->data)->type==KEY_CODE &&
		((struct key_code *)((struct key_mod *)key->mods->data)->data)->locker;
}
void *key_get_keyboard(struct key *key) { return key->keyboard; }
GdkModifierType key_get_modifier(struct key *key) {
	return ((struct key_mod *)key->mods->data)->type==KEY_CODE?
		((struct key_code *)((struct key_mod *)key->mods->data)->data)->modifier:0;
}

/* return if key is it at position */
#ifdef ENABLE_RAMBLE
enum key_hit key_hit(struct key *key, gint x, gint y, gdouble zx, gdouble zy)
#else
gboolean key_hit(struct key *key, gint x, gint y, gdouble zx, gdouble zy)
#endif
{
	gint x1=zx*(key->x-(key->w/2.0));
	gint y1=zy*(key->y-(key->h/2.0));
	gint x2=x1+(zx*key->w);
	gint y2=y1+(zy*key->h);
	gboolean ret=FALSE;
	if ((x>=x1) && (x<=x2) && (y>=y1) && (y<=y2)) {
		ret=style_shape_test(key->shape, x-x1, y-y1, key->w*zx, key->h*zy);
	}

#ifdef ENABLE_RAMBLE
	if (ret) {
		x1+=zx*key->w*BORDER_THRESHOLD;
		y1+=zy*key->h*BORDER_THRESHOLD;
		x2-=zx*key->w*BORDER_THRESHOLD;
		y2-=zy*key->h*BORDER_THRESHOLD;
		if ((x>=x1) && (x<=x2) && (y>=y1) && (y<=y2)) 
			return KEY_HIT;
		else return KEY_BORDER;
	} else return KEY_MISS;
#else
	return ret;
#endif
}
