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

#include <stdio.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include "system.h"
#include "trace.h"
#include "style.h"
#include "settings.h"
#include "layoutreader.h"

/* Constants */
static gchar *style_css_file="%s/.florence/florence.css";
static gchar *style_css_file_source=DATADIR "/florence.css";
static gchar *style_svg_format="<?xml-stylesheet type=\"text/css\" href=\"%s/.florence/florence.css\"?>%s";

/* a symbol is drawn over the shape to identify the effect of key.
 * the symbol is either text (label) or svg. Either one is NULL */
struct symbol {
	GRegex *name;
	gchar *label;
	RsvgHandle *svg;
	gchar *source;
};

/* color functions */
gchar *style_get_color(enum style_colours c)
{
	gchar *color;
	switch(c) {
		case STYLE_KEY_COLOR: color=(gchar *)settings_get_string("colours/key"); break;
		case STYLE_OUTLINE_COLOR: color=(gchar *)settings_get_string("colours/outline"); break;
		case STYLE_TEXT_COLOR: color=(gchar *)settings_get_string("colours/label"); break;
		case STYLE_ACTIVATED_COLOR: color=(gchar *)settings_get_string("colours/activated"); break;
		case STYLE_MOUSE_OVER_COLOR: color=(gchar *)settings_get_string("colours/mouseover"); break;
		default: color=NULL;
	}
	if (!color) flo_warn(_("Unknown style color: %d"), c);
	return color;
}

/* set cairo color to one of the style colors */
void style_cairo_set_color(struct style *style, cairo_t *cairoctx, enum style_colours c)
{
	guint r, g, b;
	gchar *color=style_get_color(c);
	if (3!=sscanf(color, "#%02x%02x%02x", &r, &g, &b)) {
		flo_warn(_("can't parse color %s"), color);
		cairo_set_source_rgb(cairoctx, 0.0, 0.0, 0.0);
	} else cairo_set_source_rgb(cairoctx, (gdouble)r/255.0, (gdouble)g/255.0, (gdouble)b/255.0);
	if (color) g_free(color);
}

/* Renders a svg handle to a cairo surface at dimensions */
void style_render_svg(cairo_t *cairoctx, RsvgHandle *handle, gdouble w, gdouble h, gboolean keep_ratio, gchar *sub)
{
	gdouble xscale, yscale;
	gdouble xoffset=0., yoffset=0.;
	RsvgDimensionData dimensions;
	rsvg_handle_get_dimensions(handle, &dimensions);
	cairo_save(cairoctx);
	if (keep_ratio) {
		if ((dimensions.width/dimensions.height)<(w/h)) {
			yscale=h/dimensions.height;
			xscale=yscale*dimensions.width/dimensions.height;
			xoffset=(w-(dimensions.width*xscale))/2.;
		} else {
			xscale=w/dimensions.width;
			yscale=xscale*dimensions.height/dimensions.width;
			yoffset=(h-(dimensions.height*yscale))/2.;
		}
	} else {
		xscale=w/dimensions.width;
		yscale=h/dimensions.height;
	}
	cairo_translate(cairoctx, xoffset, yoffset);
	cairo_scale(cairoctx, xscale, yscale);
	rsvg_handle_render_cairo_sub(handle, cairoctx, sub);
	cairo_restore(cairoctx);
}

/* create a new symbol */
void style_symbol_new(struct style *style, char *name, char *svg, char *label)
{
	GError *error=NULL;
	gchar *regex=NULL;
	struct symbol *symbol=g_malloc(sizeof(struct symbol));
	memset(symbol, 0, sizeof(struct symbol));
	if (name) {
		regex=g_strdup_printf("^%s$", name);
		symbol->name=g_regex_new(regex, G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, NULL);
		g_free(regex);
	}
	if (label) {
		symbol->label=g_strdup(label);
	} else if (svg) {
		symbol->source=g_strdup_printf(style_svg_format, g_getenv("HOME"), svg);
		symbol->svg=rsvg_handle_new_from_data((guint8 *)symbol->source, (gsize)strlen(symbol->source), &error);
		if (error) flo_fatal(_("Unable to parse svg from layout file: %s"), svg);
	} else { flo_fatal(_("Bad symbol: should have either svg or label :%s"), name); }
	style->symbols=g_slist_append(style->symbols, (gpointer)symbol);
	flo_debug("[new symbol] name=%s label=%s", name, symbol->label);
}

/* free up memory used by the symbol */
void style_symbol_free(gpointer data, gpointer userdata)
{
	struct symbol *symbol=(struct symbol *)data;
	if (symbol) {
		if (symbol->name) g_regex_unref(symbol->name);
		if (symbol->label) g_free(symbol->label);
		if (symbol->svg) rsvg_handle_free(symbol->svg);
		if (symbol->source) g_free(symbol->source);
		g_free(symbol);
	}
}

/* return TRUE if the name matches the symbol regexp */
gboolean style_symbol_matches(struct symbol *symbol, gchar *name)
{
	if (!name) return FALSE;
	return g_regex_match(symbol->name, name, G_REGEX_MATCH_ANCHORED|G_REGEX_MATCH_NOTEMPTY, NULL);
}

/* Draws text with cairo */
void style_draw_text(struct style *style, cairo_t *cairoctx, gchar *text, gdouble w, gdouble h)
{
	cairo_font_extents_t fe;
	cairo_text_extents_t te;
	PangoFontDescription *fontdesc;
	gchar *fontname;
	const gchar *fontfamilly;
	GtkSettings *settings=gtk_settings_get_default();
	cairo_font_slant_t slant;

	g_object_get(settings, "gtk-font-name", &fontname, NULL);
	fontdesc=pango_font_description_from_string(fontname?fontname:"sans 10");
	g_free(fontname);
	fontfamilly=pango_font_description_get_family(fontdesc);
	switch(pango_font_description_get_style(fontdesc)) {
		case PANGO_STYLE_NORMAL: slant=CAIRO_FONT_SLANT_NORMAL; break;
		case PANGO_STYLE_OBLIQUE: slant=CAIRO_FONT_SLANT_OBLIQUE; break;
		case PANGO_STYLE_ITALIC: slant=CAIRO_FONT_SLANT_ITALIC; break;
		default: flo_warn(_("unknown slant for font %s: %d"), fontfamilly, pango_font_description_get_style(fontdesc));
			slant=CAIRO_FONT_SLANT_NORMAL; break;
	}

	cairo_save(cairoctx);
	style_cairo_set_color(style, cairoctx, STYLE_TEXT_COLOR);
	cairo_select_font_face(cairoctx, fontfamilly, slant, 
		pango_font_description_get_weight(fontdesc)<=500?CAIRO_FONT_WEIGHT_NORMAL:CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cairoctx, 0.8);
	cairo_text_extents(cairoctx, text, &te);
	cairo_font_extents(cairoctx, &fe);
	cairo_move_to(cairoctx, (w-te.width)/2-te.x_bearing, (h-fe.descent+fe.height)/2);
	cairo_show_text(cairoctx, text);
	cairo_restore(cairoctx);

	pango_font_description_free(fontdesc);
}

/* Draw the symbol represented by keyval */
void style_symbol_draw(struct style *style, cairo_t *cairoctx, guint keyval, gdouble w, gdouble h)
{
	GSList *item=style->symbols;
	gchar name[7];
	guint keyval2=keyval;

	while (item && !style_symbol_matches((struct symbol *)item->data, gdk_keyval_name(keyval))) {
		item=g_slist_next(item);
	}
	/* No predifined symbol => get the label according to keyval */
	if (!item) {
		/* find the string representation of keyval */
		name[0]='\0';
		if (gdk_keyval_name(keyval) && !strncmp(gdk_keyval_name(keyval), "dead_", 5)) {
			if (!strcmp(gdk_keyval_name(keyval), "dead_circumflex")) { name[0]='^'; name[1]='\0'; }
			else keyval2=gdk_keyval_from_name(gdk_keyval_name(keyval)+5);
		}
		if (!name[0]) name[g_unichar_to_utf8(gdk_keyval_to_unicode(keyval2), name)]='\0';
		/* if (!name[0] && gdk_keyval_name(keyval)) { strncpy(name, gdk_keyval_name(keyval), 3); name[3]='\0'; } */
		style_draw_text(style, cairoctx, name, w, h);
	/* the symbol has a label ==> let's draw it */
	} else if (((struct symbol *)item->data)->label) {
		style_draw_text(style, cairoctx, ((struct symbol *)item->data)->label, w, h);
	/* the symbol must have a svg => draw it */
	} else {
		style_render_svg(cairoctx, ((struct symbol *)item->data)->svg, w, h, TRUE, NULL);
	}
}

/* callback for layoutreader for shape */
void style_shape_new(struct style *style, char *name, char *svg)
{
	struct shape *shape=g_malloc(sizeof(struct shape));
	GError *error=NULL;
	gchar *default_uri;
	memset(shape, 0, sizeof(struct shape));
	if (name) shape->name=g_strdup(name);
	if (svg)
		shape->source=(guchar *)g_strdup_printf(style_svg_format, g_getenv("HOME"), svg);
	shape->svg=rsvg_handle_new();
	default_uri=settings_get_string("layout/style");
	rsvg_handle_set_base_uri(shape->svg, style->base_uri?style->base_uri:default_uri);
	if (default_uri) g_free(default_uri);
	rsvg_handle_write(shape->svg, shape->source, (gsize)strlen((gchar *)shape->source), &error);
	rsvg_handle_close(shape->svg, &error);
	if (error) flo_fatal(_("Unable to parse svg from layout file: svg=\"%s\" error=\"%s\""),
		shape->source, error->message);
	style->shapes=g_slist_append(style->shapes, (gpointer)shape);
	if (!strcmp(name, "default")) style->default_shape=shape;
	flo_debug(_("[new shape] name=%s svg=\"%s\""), shape->name, shape->source);
}

/* liberate memory used for shape */
void style_shape_free(gpointer data, gpointer userdata)
{
	struct shape *shape=(struct shape *)data;
	if (shape) {
		if (shape->name) g_free(shape->name);
		if (shape->source) g_free(shape->source);
		if (shape->svg) rsvg_handle_free(shape->svg);
		if (shape->mask) cairo_surface_destroy(shape->mask);
		g_free(shape);
	}
}

/* get the shape by its name */
struct shape *style_shape_get (struct style *style, gchar *name) {
	GSList *item=style->shapes;
	if (!name) return style->default_shape;
	while (item && strcmp(((struct shape *)item->data)->name, name)) item=g_slist_next(item);
	if (!item) flo_fatal(_("Shape doesn't exist: %s"), name);
	return (struct shape *)item->data;
}

/* draw the shape to the cairo context. */
void style_shape_draw(struct shape *shape, cairo_t *cairoctx, gdouble w, gdouble h)
{
	style_render_svg(cairoctx, shape->svg, w, h, FALSE, NULL);
}

/* create a mask surface for the shape, if it doesn't already exist */
cairo_surface_t *style_shape_get_mask(struct shape *shape, guint w, guint h)
{
	cairo_t *maskctx;
	if ((!shape->mask)||(w!=shape->maskw)||(h!=shape->maskh)) {
		if (shape->mask) cairo_surface_destroy(shape->mask);
		shape->maskw=w; shape->maskh=h;
		shape->mask=cairo_image_surface_create(CAIRO_FORMAT_A8, w, h);
		maskctx=cairo_create(shape->mask);
		style_shape_draw(shape, maskctx, w, h);
		cairo_destroy(maskctx);
	}
	return shape->mask;
}

/* test if point is inside the mask */
gboolean style_shape_test(struct shape *shape, gint x, gint y, guint w, guint h)
{
	cairo_surface_t *mask=style_shape_get_mask(shape, w, h);
	unsigned char *data=cairo_image_surface_get_data(mask);
	int stride=cairo_image_surface_get_stride(mask);
	return data[(y*stride)+x]>127;
}

/* Create a css file called $HOME/.florence/florence.css */
void style_create_css(struct style *style)
{ 
 	GRegex *shapecolor=g_regex_new("@SHAPE_COLOR@", G_REGEX_OPTIMIZE, 0, NULL);
 	GRegex *symcolor=g_regex_new("@SYM_COLOR@", G_REGEX_OPTIMIZE, 0, NULL);
 	GRegex *bordercolor=g_regex_new("@BORDER_COLOR@", G_REGEX_OPTIMIZE, 0, NULL);
	gchar *filename=g_strdup_printf(style_css_file, g_getenv("HOME"));
	FILE *in;
	FILE *out;
	gchar *tmp1, *tmp2, *line, *color;

	/* create the directory if it doesn't exist already */
	if (!settings_mkhomedir()) {
		flo_warn(_("Unable to create %s because $HOME/.florence does not exist"), filename);
		return;
	}

	/* open the files */
	in=fopen(style_css_file_source, "r");
	out=fopen(filename, "w");
	if (!in) flo_warn(_("Unable to open file %s for read"), style_css_file_source);
	if (!out) flo_warn(_("Unable to open file %s for write"), filename);
	g_free(filename);

	/* copy/parse the file */
	line=g_malloc(1024);
	if (!line) flo_warn(_("Unable to allocate memory for css read buffer"));
	if (line && in && out ) while (fgets(line, 1024, in)) {
		color=style_get_color(STYLE_KEY_COLOR);
		tmp1=g_regex_replace_literal(shapecolor, line, -1, 0, color, 0, NULL);
		if (color) g_free(color); color=style_get_color(STYLE_TEXT_COLOR);
		tmp2=g_regex_replace_literal(symcolor, tmp1, -1, 0, color, 0, NULL);
		g_free(tmp1);
		if (color) g_free(color); color=style_get_color(STYLE_OUTLINE_COLOR);
		tmp1=g_regex_replace_literal(bordercolor, tmp2, -1, 0, color, 0, NULL);
		if (color) g_free(color);
		g_free(tmp2);
		fputs(tmp1, out);
		g_free(tmp1);
	}
	if (line) g_free(line);

	/* release memory */
	g_regex_unref(shapecolor);
	g_regex_unref(symcolor);
	g_regex_unref(bordercolor);
	if (out) fclose(out);
	if (in) fclose(in);
}

/* update the colors */
void style_update_colors (struct style *style)
{
	GError *error=NULL;
	GSList *list;
	struct symbol *symbol;
	struct shape *shape;
	gchar *default_uri;

	style_create_css(style);

	list=style->symbols;
	while (list) {
		symbol=(struct symbol *)list->data;
		if (symbol->svg) {
			rsvg_handle_free(symbol->svg);
			symbol->svg=rsvg_handle_new_from_data((guint8 *)symbol->source, (gsize)strlen(symbol->source), &error);
			if (error) flo_fatal(_("Unable to parse svg from layout file: %s"), symbol->source);
		}
		list=g_slist_next(list);
	}

	list=style->shapes;
	while (list) {
		shape=(struct shape *)list->data;
		if (shape->svg) rsvg_handle_free(shape->svg);
		shape->svg=rsvg_handle_new();
		default_uri=settings_get_string("layout/style");
		rsvg_handle_set_base_uri(shape->svg, style->base_uri?style->base_uri:default_uri);
		if (default_uri) g_free(default_uri);
		rsvg_handle_write(shape->svg, shape->source, (gsize)strlen((gchar *)shape->source), &error);
		rsvg_handle_close(shape->svg, &error);
		if (error) flo_fatal(_("Unable to parse svg from layout file: %s"), shape->source);
		list=g_slist_next(list);
	}
}

/* draw a style preview to a 32x32 gdk pixbuf 
 * this function is called by the settings dialog */
GdkPixbuf *style_pixbuf_draw(struct style *style)
{
	struct shape *shape=style_shape_get(style, NULL);
	GdkPixbuf *temp=rsvg_handle_get_pixbuf(shape->svg);
	GdkPixbuf *ret=gdk_pixbuf_scale_simple(temp, 32, 32, GDK_INTERP_HYPER);
	gdk_pixbuf_unref(temp);
	return ret;
}

/* create a new style from the layout file */
struct style *style_new(gchar *base_uri)
{
	struct layout *layout=NULL;
	struct layout_shape *shape=NULL;
	struct layout_symbol *symbol=NULL;
	struct style *style=g_malloc(sizeof(struct style));

	memset(style, 0, sizeof(struct style));
	style->base_uri=base_uri;
	style_create_css(style);
	layout=layoutreader_new(base_uri?base_uri:settings_get_string("layout/style"),
		DATADIR "/styles/default/florence.style",
		DATADIR "/relaxng/style.rng");

	layoutreader_element_open(layout, "style");
	if (layoutreader_element_open(layout, "shapes")) {
		while ((shape=layoutreader_shape_new(layout))) {
			style_shape_new(style, shape->name, shape->svg);
			layoutreader_shape_free(shape);
		}
	}

	layoutreader_reset(layout);
	layoutreader_element_open(layout, "style");
	if (layoutreader_element_open(layout, "symbols")) {
		while ((symbol=layoutreader_symbol_new(layout))) {
			style_symbol_new(style, symbol->name, symbol->svg, symbol->label);
			layoutreader_symbol_free(symbol);
		}
	}

	layoutreader_free(layout);
	return style;
}

/* free style */
void style_free(struct style *style)
{
	if (style) {
		g_slist_foreach(style->shapes, style_shape_free, NULL);
		g_slist_free(style->shapes);
		g_slist_foreach(style->symbols, style_symbol_free, NULL);
		g_slist_free(style->symbols);
		g_free(style);
	}
}

