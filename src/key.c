/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008 François Agrech

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
#include "keyboard.h"
#include <math.h>
#include <string.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp_render_aa.h>

#define PI 3.1415926535897931

struct style *key_style;

void key_init(xmlTextReaderPtr layout)
{
	key_style=style_new(layout);
}

void key_exit()
{
	if (key_style) style_free(key_style);
}

struct key *key_new(struct keyboard *keyboard, guint code, GnomeCanvasClipgroup *group, GdkModifierType mod,
	gboolean locker, gchar *label)
{
	struct key *key=g_malloc(sizeof(struct key));
	key->keyboard=keyboard;
	key->code=code;
	key->group=group;
	key->pressed=FALSE;
	key->modifier=mod;
	key->locker=locker;
	if (label) {
		key->label=g_malloc((1+strlen(label))*sizeof(gchar));
		strcpy(key->label, label);
	} else key->label=NULL;
	key->timer=NULL;
	key->shape=NULL;
	key->symbol=NULL;
	key->drawn_syms=NULL;
	return key;
}

void key_free(struct key *key)
{
	GnomeCanvasItem *br;
	GSList *list=key->drawn_syms;
	if (key->label) g_free(key->label);
	if (key->shape) gtk_object_destroy(GTK_OBJECT(key->shape));
	if (key->timer) gtk_object_destroy(GTK_OBJECT(key->timer));
	if (key->group) gtk_object_destroy(GTK_OBJECT(key->group));
	while (list) {
		br=*(((struct key_symbol *)list->data)->sym);
		while (br) gtk_object_destroy(GTK_OBJECT(br++));
		g_free(((struct key_symbol *)list->data)->sym);
		g_free((struct key_symbol *)list->data);
		list=g_slist_next(list);
	}
	g_slist_free(key->drawn_syms);
	g_free(key);
}

void key_update_draw (void *callback_data, int y, int start, ArtSVPRenderAAStep *steps, int n_steps)
{
	struct key *key=(struct key *)callback_data;
	guint rowstride=keyboard_get_width(key_get_keyboard(key));
	int i=0, x=0, j;
	int value=start>>16;
	guchar *adr;

	if (n_steps && value==0) { i=1 ; x=steps[0].x; value+=steps[0].delta>>16; }
	adr=keyboard_get_map(key->keyboard)+(y*rowstride);
	for (;i<n_steps;i++) {
		j=x;
		if (value<-128||value>128)
			while (j<steps[i].x) adr[j++]=(guchar)key->code;
		x=steps[i].x;
		value+=steps[i].delta>>16;
	}
	j=x;
	if (n_steps && (value<-128||value>128)) while (j<rowstride) adr[j++]=(guchar)key->code;
}

void key_update_map(struct key *key)
{
	GnomeCanvasPathDef *gcbpath=NULL;
	ArtBpath *bpath=NULL;
	ArtVpath *vpath=NULL;
	ArtSVP *svp=NULL;
	guint x, y;
	gdouble dx, dy;
	double affine[6];
	
	gnome_canvas_w2c_affine(keyboard_get_canvas(key->keyboard), affine);
	g_object_get(G_OBJECT(key->group), "x", &dx, "y", &dy, NULL);
	gnome_canvas_w2c(keyboard_get_canvas(key->keyboard), dx, dy, &x, &y);
	affine[4]+=x;affine[5]+=y;
	affine[0]*=key->width/2.0;affine[3]*=key->height/2.0;

	g_object_get(G_OBJECT(key->shape), "bpath", &gcbpath, NULL);
	bpath=gnome_canvas_path_def_bpath(gcbpath);
	vpath=(ArtVpath *)art_bez_path_to_vec(art_bpath_affine_transform(bpath, affine), 0.1);
	if (!vpath) flo_fatal(_("Unable to create vpath"));
	svp=(ArtSVP *)art_svp_from_vpath(vpath);
	if (!svp) flo_fatal(_("Unable to create svp"));
	art_svp_render_aa(svp, 0, 0, keyboard_get_width(key->keyboard),
		keyboard_get_height(key->keyboard), key_update_draw, key);

	art_free(svp);
	art_free(vpath);
}

void key_resize(struct key *key, gdouble zoom)
{
	GnomeCanvasItem **br=NULL;
	GSList *symbols=key->drawn_syms;
	while (symbols) {
		br=((struct key_symbol *)symbols->data)->sym;
		while (*br) {
			if (G_OBJECT_TYPE(*br)==GNOME_TYPE_CANVAS_TEXT) {
				gnome_canvas_item_set(*br, "size-points", 1.2*zoom, NULL);
			}
			br++;
		}
		symbols=g_slist_next(symbols);
	}
	key_update_map(key);
}

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

void key_draw(struct key *key, double w, double h, GdkModifierType mod)
{
	double matrix[6]; /* affine matrix */
	GnomeCanvasItem **br=NULL;

	key->width=w;
	key->height=h;
	key->shape=style_shape_draw(key_style, GNOME_CANVAS_GROUP(key->group), key->label?key->label:"default");
	art_affine_scale(matrix, w/2.0, h/2.0);
	gnome_canvas_item_affine_relative(GNOME_CANVAS_ITEM(key->group), matrix);
	key_switch_mode(key, GDK_SHIFT_MASK);
	key_switch_mode(key, GDK_MOD5_MASK);
	key_switch_mode(key, 0);
	key_update_map(key);
}

void key_update_color(enum style_colours colclass, gchar *color)
{
	style_set_color(colclass, color);
}

void key_update_text_color (struct key *key)
{
	GSList *symbols=key->drawn_syms;
	GnomeCanvasItem **br=NULL;
	while (symbols) {
		br=((struct key_symbol *)symbols->data)->sym;
		while (*br) {
			if (G_OBJECT_TYPE(*br)==GNOME_TYPE_CANVAS_BPATH) {
				gnome_canvas_item_set(*(br++), "outline_color", style_get_color(STYLE_TEXT_COLOR), NULL);
			} else {
				gnome_canvas_item_set(*(br++), "fill_color", style_get_color(STYLE_TEXT_COLOR), NULL);
			}
		}
		symbols=g_slist_next(symbols);
	}
}

void key_set_color(struct key *key, enum style_colours color)
{
	gnome_canvas_item_set(key->shape, "fill_color", style_get_color(color), NULL);
}

void key_update_timer(struct key *key, double value)
{
	GnomeCanvasPoints *points;
	int i=0;

	points=gnome_canvas_points_new((3+(int)((value+0.125)*4.0))%8);
	*(points->coords+(i++))=0.0;*(points->coords+(i++))=0.0;
	*(points->coords+(i++))=0.0;*(points->coords+(i++))=-2.0;
	if (value>0.125) {
		*(points->coords+(i++))=2.0;*(points->coords+(i++))=-2.0;
	}
	if (value>0.375) {
		*(points->coords+(i++))=2.0;*(points->coords+(i++))=2.0;
	}
	if (value>0.625) {
		*(points->coords+(i++))=-2.0;*(points->coords+(i++))=2.0;
	}
	if (value>0.875) {
		*(points->coords+(i++))=-2.0;*(points->coords+(i++))=-2.0;
	}
	if (value<0.125 || value>0.875) {
		*(points->coords+(i++))=2.0*tan(value*2.0*PI);*(points->coords+(i++))=-2.0;
	} else if (value>0.125 && value<0.375) {
		*(points->coords+(i++))=2.0;*(points->coords+(i++))=2.0*tan((value-0.25)*2.0*PI);
	} else if (value>0.375 && value<0.625) {
		*(points->coords+(i++))=-2.0*tan((value-0.5)*2.0*PI);*(points->coords+(i++))=2.0;
	} else if (value>0.625 && value<0.875) {
		*(points->coords+(i++))=-2.0;*(points->coords+(i++))=-2.0*tan((value-0.75)*2.0*PI);
	}
			
	if (value==0.0) {
		if (key->timer) gtk_object_destroy(GTK_OBJECT(key->timer));
		key->timer=NULL;
	} else if (!key->timer) {
		GnomeCanvasItem **br=key->symbol;
		key->timer=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_POLYGON,
			"points", points, "fill_color", style_get_color(STYLE_MOUSE_OVER_COLOR), NULL);
		if (br) while (*br) gnome_canvas_item_raise_to_top(*(br++));
	} else {
		gnome_canvas_item_set(key->timer, "points", points, NULL);
	}

	gnome_canvas_points_unref(points);
}

void key_swap_symbol(struct key *key, GnomeCanvasItem **symbol) 
{
	GnomeCanvasItem **br=key->symbol;
	if (br) while (*br) gnome_canvas_item_hide(*(br++));
	key->symbol=symbol;
}

void key_switch_mode(struct key *key, GdkModifierType mod)
{
	GnomeCanvasItem **br;
	double matrix[6]; /* affine matrix */
	GSList *syms=key->drawn_syms;
	struct key_symbol *symbol=NULL;
	guint keyval=key_getKeyval(key, mod);

	while (syms && (((struct key_symbol *)syms->data)->keyval!=keyval)) syms=g_slist_next(syms);
	if (!syms) {
		symbol=g_malloc(sizeof(struct key_symbol));
		symbol->keyval=keyval;
		symbol->sym=style_symbol_draw(key_style, GNOME_CANVAS_GROUP(key->group), keyval);
		br=symbol->sym;
		if (*br) while (*br) {
			if (G_OBJECT_TYPE(*br)==GNOME_TYPE_CANVAS_TEXT) {
				gnome_canvas_item_set(*br, "size-points", 1.2*key->keyboard->zoom, NULL);
			}
			art_affine_scale(matrix, 2.0/key->width, 2.0/key->height);
			gnome_canvas_item_affine_relative(*(br++), matrix);
		}
		key->drawn_syms=g_slist_append(key->drawn_syms, (gpointer)symbol);
		key_swap_symbol(key, symbol->sym);
	} else if (((struct key_symbol *)syms->data)->sym!=key->symbol) {
		symbol=(struct key_symbol*)syms->data;
		br=symbol->sym;
		while (*br) gnome_canvas_item_show(*(br++));
		key_swap_symbol(key, symbol->sym);
	}
}

struct keyboard *key_get_keyboard(struct key *key)
{
	return key->keyboard;
}

