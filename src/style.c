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
#include <glib.h>
#include <glib/gprintf.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include "trace.h"
#include "style.h"
#include "settings.h"
#include "layoutreader.h"

/* Constants */
static gchar *style_css="<?xml-stylesheet type=\"text/css\" href=\"" DATADIR "/florence.css\"?>";

/* a symbol is drawn over the shape to identify the effect of key.
 * the symbol is either text (label) or svg. Either one is NULL */
struct symbol {
	GRegex *name;
	gchar *label;
	RsvgHandle *svg;
};

/* color functions */
gchar *style_get_color(struct style *style, enum style_colours c)
{
	return style->colours[c];
}

void style_set_color(struct style *style, enum style_colours c, gchar *color)
{
	if (style->colours[c]) g_free(style->colours[c]);
	style->colours[c]=g_malloc(sizeof(gchar)*(1+strlen(color)));
	strcpy(style->colours[c], color);
}

/* set cairo color to one of the style colors */
void style_cairo_set_color(struct style *style, cairo_t *cairoctx, enum style_colours c)
{
	guint r, g, b;
	if (3!=sscanf(style->colours[c], "#%02x%02x%02x", &r, &g, &b)) {
		flo_warn(_("can't parse color %s"), style->colours[c]);
		cairo_set_source_rgb(cairoctx, 0.0, 0.0, 0.0);
	} else cairo_set_source_rgb(cairoctx, (gdouble)r/255.0, (gdouble)g/255.0, (gdouble)b/255.0);
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
void style_symbol_new(char *name, char *svg, char *label, void *userdata)
{
	GError *error=NULL;
	gchar *regex=NULL;
	struct style *style=(struct style *)userdata;
	struct symbol *symbol=g_malloc(sizeof(struct symbol));
	memset(symbol, 0, sizeof(struct symbol));
	if (name) {
		regex=g_malloc((strlen(name)+3)*sizeof(gchar));
		g_sprintf(regex, "^%s$", name);
		symbol->name=g_regex_new(regex, G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, NULL);
		g_free(regex);
	}
	if (label) {
		symbol->label=g_malloc(sizeof(char)*(strlen(label)+1));
		strcpy(symbol->label, label);
	} else if (svg) {
		symbol->svg=rsvg_handle_new_from_data((guint8 *)svg, (gsize)strlen(svg), &error);
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
	cairo_move_to(cairoctx, (w - te.width) / 2 - te.x_bearing, (h - fe.descent + fe.height) / 2);
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
			if (!strcmp(gdk_keyval_name(keyval), "dead_circumflex")) { strcpy(name,"^"); }
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
void style_shape_new(char *name, char *svg, void *userdata)
{
	struct style *style=(struct style *)userdata;
	struct shape *shape=g_malloc(sizeof(struct shape));
	GError *error=NULL;
	gchar *source;
	memset(shape, 0, sizeof(struct shape));
	if (name) {
		shape->name=g_malloc(sizeof(gchar)*strlen(name)+1);
		strcpy(shape->name, name);
	}
	if (svg) {
		source=g_malloc(sizeof(gchar)*(strlen(svg)+strlen(style_css)+1));
		strcpy(source, style_css);
		strcat(source, svg);
	}
	shape->svg=rsvg_handle_new_from_data((guint8 *)source, (gsize)strlen(source), &error);
	if (error) flo_fatal(_("Unable to parse svg from layout file: svg=\"%s\" error=\"%s\""),
		source, error->message);
	g_free(source);
	style->shapes=g_slist_append(style->shapes, (gpointer)shape);
	if (!strcmp(name, "default")) style->default_shape=shape;
	flo_debug(_("[new shape] name=%s svg=%p"), shape->name, shape->svg);
}

/* liberate memory used for shape */
void style_shape_free(gpointer data, gpointer userdata)
{
	struct shape *shape=(struct shape *)data;
	if (shape) {
		if (shape->name) g_free(shape->name);
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

/* create a new style from the layout file */
struct style *style_new(xmlTextReaderPtr reader)
{
	struct style *style=g_malloc(sizeof(struct style));
	memset(style, 0, sizeof(struct style));
	memset(style->colours, 0, sizeof(style->colours));
	style_set_color(style, STYLE_KEY_COLOR, (gchar *)settings_get_string("colours/key"));
	style_set_color(style, STYLE_OUTLINE_COLOR, (gchar *)settings_get_string("colours/outline"));
	style_set_color(style, STYLE_TEXT_COLOR, (gchar *)settings_get_string("colours/label"));
	style_set_color(style, STYLE_ACTIVATED_COLOR, (gchar *)settings_get_string("colours/activated"));
	style_set_color(style, STYLE_MOUSE_OVER_COLOR, (gchar *)settings_get_string("colours/mouseover"));
	layoutreader_readstyle(reader, style_shape_new, style_symbol_new, style);
	return style;
}

/* free style */
void style_free(struct style *style)
{
	if (style) {
		g_slist_foreach(style->shapes, style_shape_free, NULL);
		g_slist_foreach(style->symbols, style_symbol_free, NULL);
		g_free(style);
	}
}

