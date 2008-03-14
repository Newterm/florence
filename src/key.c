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
#include <libart_lgpl/art_svp_render_aa.h>

#define PI 3.1415926535897931

gchar *key_colours[NUM_COLORS];

void key_init(gchar *cols[])
{
	gint i;
	for (i=0;i<NUM_COLORS;i++) {
		key_colours[i]=NULL;
		key_update_color(i, cols[i]);
	}
}

void key_exit()
{
	gint i;
	for (i=0;i<NUM_COLORS;i++) {
		if (key_colours[i]) g_free(key_colours[i]);
	}
}

struct key *key_new(struct keyboard *keyboard, guint code, GnomeCanvasClipgroup *group, GdkModifierType mod, gchar *label)
{
	struct key *key=g_malloc(sizeof(struct key));
	key->keyboard=keyboard;
	key->code=code;
	key->group=group;
	key->pressed=FALSE;
	key->modifier=mod;
	if (label) {
		key->label=g_malloc((1+strlen(label)*sizeof(gchar)));
		strcpy(key->label, label);
	} else key->label=NULL;
	key->timer=NULL;
	key->textItem=NULL;
	key->shape=NULL;
	key->items=NULL;
	return key;
}

void key_free(struct key *key)
{
	GnomeCanvasItem *br=*(key->items);
	if (key->label) g_free(key->label);
	if (key->group) gtk_object_destroy(GTK_OBJECT(key->group));
	if (key->textItem) gtk_object_destroy(GTK_OBJECT(key->textItem));
	if (key->shape) gtk_object_destroy(GTK_OBJECT(key->shape));
	if (key->timer) gtk_object_destroy(GTK_OBJECT(key->timer));
	while (br) gtk_object_destroy(GTK_OBJECT(br++));
	if (key->items) g_free(key->items);
	g_free(key);
}

void key_update_color(enum colours colclass, gchar *color)
{
	if (color) {
		if(key_colours[colclass]) g_free(key_colours[colclass]);
		key_colours[colclass]=g_malloc((1+strlen(color))*sizeof(gchar));
		strcpy(key_colours[colclass], color);
	}
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
	affine[4]+=x;
	affine[5]+=y;

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
	if (key->textItem) {
		gnome_canvas_item_set(key->textItem, "size-points", 1.2*zoom, NULL);
	}
	key_update_map(key);
}

guint key_getKeyval (struct key *key)
{
	guint *keyvals;
	guint len;
	if (!gdk_keymap_get_entries_for_keycode(NULL, key->code, NULL, &keyvals, &len)) {
		fprintf (stderr, "keycode=<%d> ==> no keyval\n", key->code);
		flo_fatal (_("Unknown keycode."));
	}
	return *keyvals;
}

gchar *key_getLabel(struct key *key, GdkModifierType mod)
{
	static gchar text[7];
	gchar *label;
	guint keyval;

	text[0]='\0';
	if (key->label) label=key->label; else {
		if (gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(), key->code, mod, 0,
			&keyval, NULL, NULL, NULL)) {
			if (gdk_keyval_name(keyval) && !strncmp(gdk_keyval_name(keyval), "dead_", 5)) {
				if (!strcmp(gdk_keyval_name(keyval), "dead_circumflex")) { strcpy(text,"^"); }
				else keyval=gdk_keyval_from_name(gdk_keyval_name(keyval)+5);
			}
			if (!text[0]) text[g_unichar_to_utf8(gdk_keyval_to_unicode(keyval), text)]='\0';
		}
		label=text;
	}
	return label;
}

void key_draw_return(struct key *key)
{
	GnomeCanvasPathDef *bpath;
	GnomeCanvasPoints *points;
	double w=key->width;
	double h=key->height;

	key->textItem=NULL;
        key->items=g_malloc(2*sizeof(GnomeCanvasItem *));

	bpath=gnome_canvas_path_def_new_sized(7);
	gnome_canvas_path_def_moveto(bpath, -w, -h/2.0);
	gnome_canvas_path_def_curveto(bpath, -w, -h*0.75, -w, -h, -w/3.0, -h);
	gnome_canvas_path_def_lineto(bpath, w/3.0, -h);
	gnome_canvas_path_def_curveto(bpath, w, -h, w, -h/2.0, w, 0.0);
	gnome_canvas_path_def_curveto(bpath, w, h/2.0, w, h, w/3.0, h);
	gnome_canvas_path_def_curveto(bpath, -w/3.0, h, -w/3.0, h/2.0, -w/3.0, 0.0);
	gnome_canvas_path_def_curveto(bpath, -w, 0.0, -w, -1.0, -w, -h/2.0);
	gnome_canvas_path_def_closepath(bpath);
	key->shape=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_BPATH,
		"bpath", bpath, "fill_color", key_colours[KEY_COLOR], "outline_color",
		key_colours[KEY_OUTLINE_COLOR], "width_units", 0.1, NULL);
	gnome_canvas_item_set((GnomeCanvasItem *)key->group, "path", bpath, NULL);
	key_update_map(key);
	gnome_canvas_path_def_unref(bpath);

	points=gnome_canvas_points_new(3);
	*(points->coords)=w/3.0;*(points->coords+1)=-h/2.0;
	*(points->coords+2)=w/3.0;*(points->coords+3)=-h*3.0/8.0;
	*(points->coords+4)=-w/3.0;*(points->coords+5)=-h*3.0/8.0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", key_colours[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);

        *(key->items+1)=NULL;
}

void key_draw_backspace(struct key *key, double w)
{
	key->textItem=NULL;
        key->items=g_malloc(2*sizeof(GnomeCanvasItem *));
	GnomeCanvasPoints *points;

	points=gnome_canvas_points_new(2);
	*(points->coords)=w/4.0;*(points->coords+1)=0.0;
	*(points->coords+2)=-w/4.0;*(points->coords+3)=0.0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", key_colours[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);
	*(key->items+1)=NULL;
}

void key_draw_shift(struct key *key, double h)
{
	key->textItem=NULL;
        key->items=g_malloc(3*sizeof(GnomeCanvasItem *));
	GnomeCanvasPoints *points;

	points=gnome_canvas_points_new(2);
	*(points->coords)=0.0;*(points->coords+1)=h/4.0;
	*(points->coords+2)=0.0;*(points->coords+3)=-h/4.0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", key_colours[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);
	*(key->items+1)=NULL;
}

void key_draw_tab(struct key *key, double w)
{
	key->textItem=NULL;
        key->items=g_malloc(3*sizeof(GnomeCanvasItem *));
	GnomeCanvasPoints *points;

	points=gnome_canvas_points_new(2);
	*(points->coords)=0.15-(w/4.0);*(points->coords+1)=0;
	*(points->coords+2)=(w/4.0)-0.15;*(points->coords+3)=0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", key_colours[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);

	points=gnome_canvas_points_new(2);
	*(points->coords)=(w/4.0);*(points->coords+1)=-0.5;
	*(points->coords+2)=(w/4.0);*(points->coords+3)=0.5;
	*(key->items+1)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", key_colours[KEY_TEXT_COLOR], "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);

	*(key->items+2)=NULL;
}

void key_draw_capslock(struct key *key)
{
	key->textItem=NULL;
        key->items=g_malloc(3*sizeof(GnomeCanvasItem *));
        *(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_ELLIPSE,
                "x1", -0.25, "y1", -0.5, "x2", 0.25, "y2", 0.0,
                "outline_color", key_colours[KEY_TEXT_COLOR], "width_units", 0.2, NULL);
        *(key->items+1)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_RECT,
                "x1", -0.5, "y1", -0.25, "x2", 0.5, "y2", 0.75,
                "fill_color", key_colours[KEY_TEXT_COLOR], "width_units", 0.1, NULL);
	*(key->items+2)=NULL;
}

void key_draw_general(struct key *key, enum key_class class)
{
	GnomeCanvasPathDef *bpath;
	double w=key->width;
	double h=key->height;

	bpath=gnome_canvas_path_def_new_sized(5);
	gnome_canvas_path_def_moveto(bpath, -w, 0.0);
	gnome_canvas_path_def_curveto(bpath, -w, h/2.0, -w, h, 0.0, h);
	gnome_canvas_path_def_curveto(bpath, w, h, w, h/2.0, w, 0.0);
	gnome_canvas_path_def_curveto(bpath, w, -h/2.0, w, -h, 0.0, -h);
	gnome_canvas_path_def_curveto(bpath, -w, -h, -w, -h/2.0, -w, 0.0);
	gnome_canvas_path_def_closepath(bpath);
	key->shape=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_BPATH,
		"bpath", bpath, "fill_color", key_colours[KEY_COLOR], "outline_color",
		key_colours[KEY_OUTLINE_COLOR], "width_units", 0.1, NULL);
	gnome_canvas_item_set((GnomeCanvasItem *)key->group, "path", bpath, NULL);
	key_update_map(key);
	gnome_canvas_path_def_unref(bpath);

	switch(class) {
		case KEY_BACKSPACE: key_draw_backspace(key, w); break;
		case KEY_TAB: key_draw_tab(key, w); break;
		case KEY_CAPSLOCK: key_draw_capslock(key); break;
		case KEY_SHIFT: key_draw_shift(key, h); break;
		case KEY_DEFAULT:
	        	key->items=g_malloc(sizeof(GnomeCanvasItem *));
        		key->textItem=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_TEXT,
				"x", 0.0, "y", 0.0, "text", key_getLabel(key, 0), "fill_color", key_colours[KEY_TEXT_COLOR],
				"size-set", TRUE, "size-points", 1.2*keyboard_get_zoom(key->keyboard), NULL);
				*(key->items)=NULL;
			break;
		default:
			flo_fatal(_("Unknown key class :%d"), class);
	}
}

void key_draw(struct key *key, enum key_class class, double w, double h)
{
	key->width=w;
	key->height=h;
	if (class==KEY_RETURN) key_draw_return(key);
	else key_draw_general(key, class);
}

void key_update_text_color (struct key *key)
{
	GnomeCanvasItem **br=key->items;
	if (key->textItem) {
		gnome_canvas_item_set(key->textItem, "fill_color", key_colours[KEY_TEXT_COLOR], NULL);
	} else if (key->items) {
		while (*br) {
			if ((key->modifier & GDK_LOCK_MASK)&&br==key->items)
				gnome_canvas_item_set(*(br++), "outline_color", key_colours[KEY_TEXT_COLOR], NULL);
			else
				gnome_canvas_item_set(*(br++), "fill_color", key_colours[KEY_TEXT_COLOR], NULL);
		}
	}
}

void key_set_color(struct key *key, enum colours color)
{
	gnome_canvas_item_set(key->shape, "fill_color", key_colours[color], NULL);
}

void key_update_timer(struct key *key, double value)
{
	GnomeCanvasPoints *points;
	int i=0;

	points=gnome_canvas_points_new((3+(int)((value+0.125)*4.0))%8);
	*(points->coords+(i++))=0.0;*(points->coords+(i++))=0.0;
	*(points->coords+(i++))=0.0;*(points->coords+(i++))=-key->height;
	if (value>0.125) {
		*(points->coords+(i++))=key->width;*(points->coords+(i++))=-key->height;
	}
	if (value>0.375) {
		*(points->coords+(i++))=key->width;*(points->coords+(i++))=key->height;
	}
	if (value>0.625) {
		*(points->coords+(i++))=-key->width;*(points->coords+(i++))=key->height;
	}
	if (value>0.875) {
		*(points->coords+(i++))=-key->width;*(points->coords+(i++))=-key->height;
	}
	if (value<0.125 || value>0.875) {
		*(points->coords+(i++))=key->width*tan(value*2.0*PI);*(points->coords+(i++))=-(key->height);
	} else if (value>0.125 && value<0.375) {
		*(points->coords+(i++))=key->width;*(points->coords+(i++))=key->height*tan((value-0.25)*2.0*PI);
	} else if (value>0.375 && value<0.625) {
		*(points->coords+(i++))=-key->width*tan((value-0.5)*2.0*PI);*(points->coords+(i++))=key->height;
	} else if (value>0.625 && value<0.875) {
		*(points->coords+(i++))=-key->width;*(points->coords+(i++))=-key->height*tan((value-0.75)*2.0*PI);
	}
			
	if (value==0.0) {
		if (key->timer) gtk_object_destroy(GTK_OBJECT(key->timer));
		key->timer=NULL;
	} else if (!key->timer) {
		GnomeCanvasItem **br=key->items;
		key->timer=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_POLYGON,
			"points", points, "fill_color", key_colours[KEY_MOUSE_OVER_COLOR], NULL);
		while (*br) gnome_canvas_item_raise_to_top(*(br++));
		if (key->textItem) gnome_canvas_item_raise_to_top(key->textItem);
	} else {
		gnome_canvas_item_set(key->timer, "points", points, NULL);
	}

	gnome_canvas_points_unref(points);
}

void key_switch_mode(struct key *key, GdkModifierType mod)
{
	if (key->textItem) {
		gnome_canvas_item_set(key->textItem, "text", key_getLabel(key, mod), NULL);
	}
}

struct keyboard *key_get_keyboard(struct key *key)
{
	return key->keyboard;
}

