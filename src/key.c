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

#include "key.h"
#include <string.h>
#include <libart_lgpl/art_svp_render_aa.h>

gchar *colors[NUM_COLORS];
guchar *keyboard_map;
gint keyboard_map_rowstride;
GnomeCanvas *canvas;

void key_init(GnomeCanvas *gnome_canvas, gchar *cols[], guchar *buf)
{
	gint i;
	for (i=0;i<NUM_COLORS;i++) {
		if (cols[i]!=NULL) {
			colors[i]=g_malloc((1+strlen(cols[i]))*sizeof(gchar));
			strcpy(colors[i], cols[i]);
		}
	}
	keyboard_map=buf;
	canvas=gnome_canvas;
}

void key_exit()
{
	gint i;
	for (i=0;i<NUM_COLORS;i++) {
		if (colors[i]) g_free(colors[i]);
	}
}

struct key *key_new(guint code, GnomeCanvasClipgroup *group, GdkModifierType mod, gchar *label)
{
	struct key *key=g_malloc(sizeof(struct key));
	key->code=code;
	key->group=group;
	key->pressed=FALSE;
	key->modifier=mod;
	if (label) {
		key->label=g_malloc((1+strlen(label)*sizeof(gchar)));
		strcpy(key->label, label);
	} else key->label=NULL;
	key->timer=NULL;
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

void key_update_draw (void *callback_data, int y, int start, ArtSVPRenderAAStep *steps, int n_steps)
{
	struct key *key=(struct key *)callback_data;
	int i, j;
	int x=steps[0].x;
	guchar *adr=keyboard_map+(y*keyboard_map_rowstride);
	for (i=1;i<n_steps;i++) {
		j=x;
		while (j<steps[i].x) *(adr+(j++))=(guchar)key->code;
		x=steps[i].x;
	}
}

void key_update_map(struct key *key, GnomeCanvasPathDef *path)
{
	ArtBpath *bpath;
	ArtVpath *vpath;
	ArtSVP *svp;
	guint w, h;
	guint x, y;
	gdouble dx, dy;
	gdouble affine[6];
	
	gtk_layout_get_size(GTK_LAYOUT(canvas), &w, &h);
	keyboard_map_rowstride=w;
	gnome_canvas_w2c_affine(canvas, affine);
	g_object_get(G_OBJECT(key->group), "x", &dx, "y", &dy, NULL);
	gnome_canvas_w2c(canvas, dx, dy, &x, &y);
	affine[4]+=x;
	affine[5]+=y;

	bpath=gnome_canvas_path_def_bpath(path);
	vpath=art_bez_path_to_vec(art_bpath_affine_transform(bpath, affine), 0.1);
	svp=art_svp_from_vpath(vpath);
	art_svp_render_aa(svp, 0, 0, w, h, key_update_draw, key);

	art_free(svp);
	art_free(vpath);
}

guint key_getKeyval (struct key *key)
{
	guint *keyvals;
	guint len;
	if (!gdk_keymap_get_entries_for_keycode(NULL, key->code, NULL, &keyvals, &len)) {
		fprintf (stderr, "keycode=<%d> ==> no keyval\n", key->code);
		fatal ("Unknown keycode.");
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

void key_draw_return(struct key *key, double x, double y)
{
	GnomeCanvasPathDef *bpath;
	GnomeCanvasPoints *points;

	key->textItem=NULL;
        key->items=g_malloc(2*sizeof(GnomeCanvasItem *));
	key->width=3.0;
	key->height=4.0;

	bpath=gnome_canvas_path_def_new_sized(7);
	gnome_canvas_path_def_moveto(bpath, -3.0, -2.0);
	gnome_canvas_path_def_curveto(bpath, -3.0, -3.0, -2.0, -4.0, -1.0, -4.0);
	gnome_canvas_path_def_lineto(bpath, 1.0, -4.0);
	gnome_canvas_path_def_curveto(bpath, 2.0, -4.0, 3.0, -2.0, 3.0, 0.0);
	gnome_canvas_path_def_curveto(bpath, 3.0, 2.0, 2.0, 4.0, 1.0, 4.0);
	gnome_canvas_path_def_curveto(bpath, 0.0, 4.0, -1.0, 2.0, -1.0, 0.0);
	gnome_canvas_path_def_curveto(bpath, -2.0, 0.0, -3.0, -1.0, -3.0, -2.0);
	gnome_canvas_path_def_closepath(bpath);
	key->shape=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_BPATH,
		"bpath", bpath, "fill_color", colors[KEY_COLOR], "outline_color",
		colors[KEY_OUTLINE_COLOR], "width_units", 0.1, NULL);
	gnome_canvas_item_set((GnomeCanvasItem *)key->group, "path", bpath, NULL);
	key_update_map(key, bpath);
	gnome_canvas_path_def_unref(bpath);

	points=gnome_canvas_points_new(3);
	*(points->coords)=1.0;*(points->coords+1)=-2.0;
	*(points->coords+2)=1.0;*(points->coords+3)=-1.5;
	*(points->coords+4)=-1.0;*(points->coords+5)=-1.5;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", colors[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);

        *(key->items+1)=NULL;
}

void key_draw_backspace(struct key *key, double x, double y, double w)
{
	key->textItem=NULL;
        key->items=g_malloc(2*sizeof(GnomeCanvasItem *));
	GnomeCanvasPoints *points;

	points=gnome_canvas_points_new(2);
	*(points->coords)=w/4.0;*(points->coords+1)=0.0;
	*(points->coords+2)=-w/4.0;*(points->coords+3)=0.0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", colors[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);
	*(key->items+1)=NULL;
}

void key_draw_shift(struct key *key, double x, double y, double h)
{
	key->textItem=NULL;
        key->items=g_malloc(3*sizeof(GnomeCanvasItem *));
	GnomeCanvasPoints *points;

	points=gnome_canvas_points_new(2);
	*(points->coords)=0.0;*(points->coords+1)=h/4.0;
	*(points->coords+2)=0.0;*(points->coords+3)=-h/4.0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", colors[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);
	*(key->items+1)=NULL;
}

void key_draw_tab(struct key *key, double x, double y, double w)
{
	key->textItem=NULL;
        key->items=g_malloc(3*sizeof(GnomeCanvasItem *));
	GnomeCanvasPoints *points;

	points=gnome_canvas_points_new(2);
	*(points->coords)=0.15-(w/4.0);*(points->coords+1)=0;
	*(points->coords+2)=(w/4.0)-0.15;*(points->coords+3)=0;
	*(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", colors[KEY_TEXT_COLOR], "last_arrowhead", TRUE, 
		"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);

	points=gnome_canvas_points_new(2);
	*(points->coords)=(w/4.0);*(points->coords+1)=-0.5;
	*(points->coords+2)=(w/4.0);*(points->coords+3)=0.5;
	*(key->items+1)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_LINE,
		"points", points, "fill_color", colors[KEY_TEXT_COLOR], "width_units", 0.3, NULL);
	gnome_canvas_points_unref(points);

	*(key->items+2)=NULL;
}

void key_draw_capslock(struct key *key, double x, double y)
{
	key->textItem=NULL;
        key->items=g_malloc(3*sizeof(GnomeCanvasItem *));
        *(key->items)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_ELLIPSE,
                "x1", -0.25, "y1", -0.5, "x2", 0.25, "y2", 0.0,
                "outline_color", colors[KEY_TEXT_COLOR], "width_units", 0.2, NULL);
        *(key->items+1)=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_RECT,
                "x1", -0.5, "y1", -0.25, "x2", 0.5, "y2", 0.75,
                "fill_color", colors[KEY_TEXT_COLOR], "width_units", 0.1, NULL);
	*(key->items+2)=NULL;
}

void key_draw4(struct key *key, double x, double y, double w, double h)
{
	GnomeCanvasPathDef *bpath;
	guint keyval;
	key->width=w;
	key->height=h;

	bpath=gnome_canvas_path_def_new_sized(5);
	gnome_canvas_path_def_moveto(bpath, -w, 0.0);
	gnome_canvas_path_def_curveto(bpath, -w, h/2.0, -w/2.0, h, 0.0, h);
	gnome_canvas_path_def_curveto(bpath, w/2.0, h, w, h/2.0, w, 0.0);
	gnome_canvas_path_def_curveto(bpath, w, -h/2.0, w/2.0, -h, 0.0, -h);
	gnome_canvas_path_def_curveto(bpath, -w/2.0, -h, -w, -h/2.0, -w, 0.0);
	gnome_canvas_path_def_closepath(bpath);
	key->shape=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_BPATH,
		"bpath", bpath, "fill_color", colors[KEY_COLOR], "outline_color",
		colors[KEY_OUTLINE_COLOR], "width_units", 0.1, NULL);
	gnome_canvas_item_set((GnomeCanvasItem *)key->group, "path", bpath, NULL);
	key_update_map(key, bpath);
	gnome_canvas_path_def_unref(bpath);

	keyval=key_getKeyval(key);
	if (keyval==gdk_keyval_from_name("BackSpace")) { key_draw_backspace(key, x, y, w); }
	else if (keyval==gdk_keyval_from_name("Tab")) { key_draw_tab(key, x, y, w); }
	else if (keyval==gdk_keyval_from_name("Caps_Lock")) { key_draw_capslock(key, x, y); }
	else if (key->modifier & GDK_SHIFT_MASK) { key_draw_shift(key, x, y, h); }
	else {
	        key->items=g_malloc(sizeof(GnomeCanvasItem *));
        	key->textItem=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_TEXT,
                	"x", 0.0, "y", 0.0, "text", key_getLabel(key, 0), "fill_color", colors[KEY_TEXT_COLOR], NULL);
        	*(key->items)=NULL;
	}
}

void key_draw2(struct key *key, double x, double y)
{
	if (key_getKeyval(key)==gdk_keyval_from_name("Return")) { key_draw_return(key, x, y); }
	else key_draw4(key, x, y, 2.0, 2.0);
}

void key_set_color(struct key *key, enum colors color)
{
	gnome_canvas_item_set(key->shape, "fill_color", colors[color], NULL);
}

void key_update_timer(struct key *key, double value)
{
	double height=2*key->height*(0.5-value);
	if (value==0.0) {
		if (key->timer) gtk_object_destroy(GTK_OBJECT(key->timer));
		key->timer=NULL;
	} else if (!key->timer) {
		GnomeCanvasItem **br=key->items;
		key->timer=gnome_canvas_item_new((GnomeCanvasGroup *)key->group, GNOME_TYPE_CANVAS_RECT, 
			"x1", -key->width, "y1", height, "x2", key->width, "y2", key->height,
			"fill_color", colors[KEY_MOUSE_OVER_COLOR], "width_units", 0.2, NULL);
		while (*br) gnome_canvas_item_raise_to_top(*(br++));
		if (key->textItem) gnome_canvas_item_raise_to_top(key->textItem);
	} else {
		gnome_canvas_item_set(key->timer, "y1", height, NULL);
	}
}

void key_switch_mode(struct key *key, GdkModifierType mod)
{
	if (key->textItem) {
		gnome_canvas_item_set(key->textItem, "text", key_getLabel(key, mod), NULL);
	}
}

