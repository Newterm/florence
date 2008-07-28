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

#include <system.h>
#include "style.h"
#include "settings.h"
#include "trace.h"
#include "layoutreader.h"

/* simplified GnomeCanvasLine */
struct symbol_line {
	GnomeCanvasPoints *points;
	gdouble width;
	gboolean arrow;
};

/* simplified GnomeCanvasRect */
struct symbol_rect {
	gdouble x1;
	gdouble y1;
	gdouble x2;
	gdouble y2;
};

/* simplified GnomeCanvasItem */
struct symbol_item {
	GType type;
	union {
		struct symbol_line *line;
		struct symbol_rect *rect;
		GnomeCanvasPathDef *path;
	} element;
};

struct symbol {
	GRegex *name;
	gchar *label;
	GSList *items; /* symbol_item */
};

struct shape {
	gchar *name;
	GnomeCanvasPathDef *path;
};

gchar *style_colours[STYLE_NUM_COLOURS];

gchar *style_get_color(enum style_colours c)
{
	return style_colours[c];
}

void style_set_color(enum style_colours c, gchar *color)
{
	if (style_colours[c]) g_free(style_colours[c]);
	style_colours[c]=g_malloc(sizeof(gchar)*(1+strlen(color)));
	strcpy(style_colours[c], color);
}

void style_readpoint(void *userdata, double x, double y)
{
	GSList **ptlist=(GSList **)userdata;
	gdouble *px, *py;
	px=g_malloc(sizeof(gdouble));
	py=g_malloc(sizeof(gdouble));
	*px=x; *py=y;
	*ptlist=g_slist_append(*ptlist, px);
	*ptlist=g_slist_append(*ptlist, py);
}

void style_addpoint(gpointer data, gpointer userdata)
{
	gdouble *coord=(gdouble *)data;
	GnomeCanvasPoints *pts=(GnomeCanvasPoints *)userdata;
	pts->coords[(pts->num_points)++]=*coord;
	g_free(coord);
}

void style_symbol_additem(xmlTextReaderPtr reader, void *userdata, gboolean arrow)
{
	GSList **items=(GSList **)userdata;
	GSList *ptlist=NULL;
	GnomeCanvasPoints *pts=NULL;
	struct symbol_item *item=NULL;

	while (layoutreader_readpt(reader, &ptlist, style_readpoint));
	pts=gnome_canvas_points_new(g_slist_length(ptlist)>>1);
	pts->num_points=0;
	g_slist_foreach(ptlist, style_addpoint, pts);
	pts->num_points>>=1;
	g_slist_free(ptlist);
	item=g_malloc(sizeof(struct symbol_item));
	item->type=GNOME_TYPE_CANVAS_LINE;
	item->element.line=g_malloc(sizeof(struct symbol_line));
	item->element.line->points=pts;
	item->element.line->arrow=arrow;
	if (xmlTextReaderDepth(reader)==5) {
		item->element.line->width=layoutreader_readdouble(reader, "width", 5);
	} else { item->element.line->width=0.3; }
	*items=g_slist_append(*items, item);
}

void style_symbol_addline(xmlTextReaderPtr reader, void *userdata)
{
	style_symbol_additem(reader, userdata, FALSE);
}

void style_symbol_addarrow(xmlTextReaderPtr reader, void *userdata)
{
	style_symbol_additem(reader, userdata, TRUE);
}

void style_symbol_addrect(xmlTextReaderPtr reader, void *userdata)
{
	GSList **items=(GSList **)userdata;
	struct symbol_item *item=g_malloc(sizeof(struct symbol_item));
	struct symbol_rect *rect=g_malloc(sizeof(struct symbol_rect));
	layoutreader_readpt2(reader, &(rect->x1), &(rect->y1), "p1", 5);
	layoutreader_readpt2(reader, &(rect->x2), &(rect->y2), "p2", 5);
	item->type=GNOME_TYPE_CANVAS_RECT;
	item->element.rect=rect;
	*items=g_slist_append(*items, item);
}

void style_symbol_addpath(char *name, GnomeCanvasPathDef *path, void *userdata)
{
	GSList **items=(GSList **)userdata;
	struct symbol_item *item=g_malloc(sizeof(struct symbol_item));
	item->type=GNOME_TYPE_CANVAS_BPATH;
	item->element.path=path;
	*items=g_slist_append(*items, item);
}

void style_symbol_new(xmlTextReaderPtr reader, char *name, char *label, void *userdata)
{
	gchar *regex;
	struct style *style=(struct style *)userdata;
	struct symbol *symbol=g_malloc(sizeof(struct symbol));
	memset(symbol, 0, sizeof(struct symbol));
	if (name) {
		regex=g_malloc((strlen(name)+3)*sizeof(gchar));
		sprintf(regex, "^%s$", name);
		symbol->name=g_regex_new(regex, G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, NULL);
		g_free(regex);
	}
	if (label) {
		symbol->label=g_malloc(sizeof(char)*(strlen(label)+1));
		strcpy(symbol->label, label);
	} else {
		layoutreader_readdraw(reader, &(symbol->items), style_symbol_addarrow, style_symbol_addline,
			style_symbol_addrect, style_symbol_addpath);
	}
	style->symbols=g_slist_append(style->symbols, (gpointer)symbol);
	flo_debug("[new symbol] name=%s label=%s", name, symbol->label);
}

void style_symbol_freeitem(gpointer data, gpointer userdata)
{
	struct symbol_item *item=(struct symbol_item *)data;
	if (item) {
		if (item->type==GNOME_TYPE_CANVAS_LINE) {
			gnome_canvas_points_unref(item->element.line->points);
			g_free(item->element.line);
		} else if (item->type==GNOME_TYPE_CANVAS_RECT) {
			g_free(item->element.rect);
		} else if (item->type==GNOME_TYPE_CANVAS_BPATH) {
			gnome_canvas_path_def_unref(item->element.path);
		} else { flo_error(_("Freeing unknown item type")); }
		g_free(item);
	}
	/*gtk_object_destroy(GTK_OBJECT(group));*/
}

void style_symbol_free(gpointer data, gpointer userdata)
{
	struct symbol *symbol=(struct symbol *)data;
	if (symbol) {
		if (symbol->name) g_regex_unref(symbol->name);
		if (symbol->label) g_free(symbol->label);
		g_slist_foreach(symbol->items, style_symbol_freeitem, NULL);
		g_slist_free(symbol->items);
		g_free(symbol);
	}
}

gboolean style_symbol_matches(struct symbol *symbol, gchar *name)
{
	if (!name) return FALSE;
	return g_regex_match(symbol->name, name, G_REGEX_MATCH_ANCHORED|G_REGEX_MATCH_NOTEMPTY, NULL);
}

GnomeCanvasItem *style_symbol_bpath_draw(GnomeCanvasGroup *group, GnomeCanvasPathDef *path)
{
	return gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_BPATH, "width_units", 0.1, "outline-color",
		style_colours[STYLE_TEXT_COLOR], "bpath", path, NULL);
}

GnomeCanvasItem *style_symbol_rect_draw(GnomeCanvasGroup *group, struct symbol_rect *rect)
{
	return gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_RECT, "x1", rect->x1, "y1", rect->y1, "x2",
		rect->x2, "y2", rect->y2, "fill_color", style_colours[STYLE_TEXT_COLOR], "width_units", 0.2, NULL);
}

GnomeCanvasItem *style_symbol_line_draw(GnomeCanvasGroup *group, struct symbol_line *line)
{
	GnomeCanvasItem *ret=NULL;
	if (line->arrow) {
		ret=gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_LINE,
			"points", line->points, "fill_color", style_colours[STYLE_TEXT_COLOR], "last_arrowhead", TRUE,
			"arrow_shape_b", -0.4, "arrow_shape_a", -0.4, "arrow_shape_c", 0.4, "width_units",
			line->width, NULL);
	} else {
		ret=gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_LINE,
			"points", line->points, "fill_color", style_colours[STYLE_TEXT_COLOR], "width_units",
			line->width, NULL);
	}
	return ret;
}

GnomeCanvasItem **style_symbol_draw(struct style *style, GnomeCanvasGroup *group, guint keyval)
{
	GnomeCanvasItem **ret=NULL;
	GnomeCanvasItem **br=NULL;
	GSList *item=style->symbols;
	struct symbol *symbol;
	struct symbol_item *symbolItem;
	gchar name[7];
	guint keyval2=keyval;

	while (item && !style_symbol_matches((struct symbol *)item->data, gdk_keyval_name(keyval))) {
		item=g_slist_next(item);
	}
	if (!item) {
		name[0]='\0';
		if (gdk_keyval_name(keyval) && !strncmp(gdk_keyval_name(keyval), "dead_", 5)) {
			if (!strcmp(gdk_keyval_name(keyval), "dead_circumflex")) { strcpy(name,"^"); }
			else keyval2=gdk_keyval_from_name(gdk_keyval_name(keyval)+5);
		}
		if (!name[0]) name[g_unichar_to_utf8(gdk_keyval_to_unicode(keyval2), name)]='\0';
		/* if (!name[0] && gdk_keyval_name(keyval)) { strncpy(name, gdk_keyval_name(keyval), 3); name[3]='\0'; } */
		ret=g_malloc(2*sizeof(GnomeCanvasItem *));
		*ret=gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_TEXT, "x", 0.0, "y", 0.0, "text", name,
			"fill_color", style_colours[STYLE_TEXT_COLOR], "size-set", TRUE, "size-points", 12.0, NULL);
		*(ret+1)=NULL;
	} else if (((struct symbol *)item->data)->label) {
		ret=g_malloc(2*sizeof(GnomeCanvasItem *));
		*ret=gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_TEXT, "x", 0.0, "y", 0.0, "text",
			((struct symbol *)item->data)->label, "fill_color", style_colours[STYLE_TEXT_COLOR],
			"size-set", TRUE, "size-points", 12.0, NULL);
		*(ret+1)=NULL;
	} else {
		symbol=(struct symbol *)item->data;
		ret=g_malloc(sizeof(GnomeCanvasItem *)*(1+g_slist_length(symbol->items)));
		br=ret;
		item=symbol->items;
		while (item!=NULL)
		{
			symbolItem=(struct symbol_item *)item->data;
			if (symbolItem->type==GNOME_TYPE_CANVAS_LINE) {
				*(br++)=style_symbol_line_draw(group, symbolItem->element.line);
			} else if (symbolItem->type==GNOME_TYPE_CANVAS_RECT) {
				*(br++)=style_symbol_rect_draw(group, symbolItem->element.rect);
			} else if (symbolItem->type==GNOME_TYPE_CANVAS_BPATH) {
				*(br++)=style_symbol_bpath_draw(group, symbolItem->element.path);
			}
			item=g_slist_next(item);
		}
		*(br)=NULL;
	}

	return ret;
}

void style_shape_new(char *name, GnomeCanvasPathDef *path, void *userdata)
{
	struct style *style=(struct style *)userdata;
	struct shape *shape=g_malloc(sizeof(struct shape));
	memset(shape, 0, sizeof(struct shape));
	if (name) {
		shape->name=g_malloc(sizeof(gchar)*strlen(name)+1);
		strcpy(shape->name, name);
	}
	shape->path=path;
	style->shapes=g_slist_append(style->shapes, (gpointer)shape);
	flo_debug("[new shape] name=%s path=%p", shape->name, shape->path);
}

void style_shape_free(gpointer data, gpointer userdata)
{
	struct shape *shape=(struct shape *)data;
	if (shape) {
		if (shape->name) g_free(shape->name);
		if (shape->path) gnome_canvas_path_def_unref(shape->path);
		g_free(shape);
	}
}

GnomeCanvasItem *style_shape_draw(struct style *style, GnomeCanvasGroup *group, gchar *name)
{
	GnomeCanvasItem *ret;
	GSList *item=style->shapes;

	while (item && strcmp(((struct shape *)item->data)->name, name)) item=g_slist_next(item);
	if (!item) flo_fatal(_("Shape doesn't exist: %s"), name);
	ret=gnome_canvas_item_new(group, GNOME_TYPE_CANVAS_BPATH, "bpath", ((struct shape *)item->data)->path,
		"fill_color", style_colours[STYLE_KEY_COLOR], "outline_color",
		style_colours[STYLE_OUTLINE_COLOR], "width_units", 0.1, NULL);
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(group), "path", ((struct shape *)item->data)->path, NULL);
	return ret;
}

struct style *style_new(xmlTextReaderPtr reader)
{
	struct style *style=g_malloc(sizeof(struct style));
	memset(style, 0, sizeof(struct style));
	memset(style_colours, 0, sizeof(style_colours));
	style_set_color(STYLE_KEY_COLOR, (gchar *)settings_get_string("colours/key"));
	style_set_color(STYLE_OUTLINE_COLOR, (gchar *)settings_get_string("colours/outline"));
	style_set_color(STYLE_TEXT_COLOR, (gchar *)settings_get_string("colours/label"));
	style_set_color(STYLE_ACTIVATED_COLOR, (gchar *)settings_get_string("colours/activated"));
	style_set_color(STYLE_MOUSE_OVER_COLOR, (gchar *)settings_get_string("colours/mouseover"));
	layoutreader_readstyle(reader, style_shape_new, style_symbol_new, style);
	return style;
}

void style_free(struct style *style)
{
	if (style) {
		g_slist_foreach(style->shapes, style_shape_free, NULL);
		g_slist_foreach(style->symbols, style_symbol_free, NULL);
		g_free(style);
	}
}

