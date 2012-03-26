/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2012 François Agrech

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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <gst/gst.h>
#include "system.h"
#include "trace.h"
#include "key.h"
#include "style.h"
#include "settings.h"
#include "layoutreader.h"

#if !GLIB_CHECK_VERSION(2,14,0)
#include "tools.h"
#endif

/* Constants */
static gchar *style_css_file_source=DATADIR "/florence.css";

/* a symbol is drawn over the shape to identify the effect of key.
 * the symbol is either text (label) or svg. Either one is NULL */
struct symbol {
	union {
		GRegex *name;
		enum key_action_type type;
	} id;
	gchar *label;
	RsvgHandle *svg;
	gchar *source;
};

/* color functions */
gchar *style_get_color(enum style_colours c)
{
	START_FUNC
	gchar *color=NULL;
	switch(c) {
		case STYLE_KEY_COLOR: color=(gchar *)settings_get_string("colours/key"); break;
		case STYLE_OUTLINE_COLOR: color=(gchar *)settings_get_string("colours/outline"); break;
		case STYLE_TEXT_COLOR: color=(gchar *)settings_get_string("colours/label"); break;
		case STYLE_TEXT_OUTLINE_COLOR: color=(gchar *)settings_get_string("colours/label_outline"); break;
		case STYLE_ACTIVATED_COLOR: color=(gchar *)settings_get_string("colours/activated"); break;
		case STYLE_MOUSE_OVER_COLOR: color=(gchar *)settings_get_string("colours/mouseover"); break;
		case STYLE_LATCHED_COLOR: color=(gchar *)settings_get_string("colours/latched"); break;
		case STYLE_RAMBLE_COLOR: color=(gchar *)settings_get_string("colours/ramble"); break;
		default: color=NULL;
	}
	if (!color) {
		flo_error(_("Unknown style color: %d"), c);
		flo_fatal(_("You probably installed Florence in a location that is not in the gconf path."
			" Please read the FAQ in Florence documentation to learn more about how to"
			" properly configure gconf."));
	}
	END_FUNC
	return color;
}

/* set cairo color to one of the style colors */
void style_cairo_set_color(cairo_t *cairoctx, enum style_colours c)
{
	START_FUNC
	guint r, g, b, a;
	gchar *color=style_get_color(c);
	if (4==sscanf(color, "#%02x%02x%02x%02x", &r, &g, &b, &a)) {
		cairo_set_source_rgba(cairoctx, (gdouble)r/255.0, (gdouble)g/255.0, (gdouble)b/255.0, (gdouble)a/255.0);
	} else if (3!=sscanf(color, "#%02x%02x%02x", &r, &g, &b)) {
		flo_warn(_("can't parse color %s"), color);
		cairo_set_source_rgb(cairoctx, 0.0, 0.0, 0.0);
	} else cairo_set_source_rgb(cairoctx, (gdouble)r/255.0, (gdouble)g/255.0, (gdouble)b/255.0);
	if (color) g_free(color);
	END_FUNC
}

/* insert css into an svg string */
gchar *style_svg_css_insert(gchar *svg, enum style_colours c)
{
	START_FUNC
	xmlSaveCtxtPtr save;
	xmlBufferPtr buffer=xmlBufferCreate();
	xmlDocPtr doc=xmlParseDoc((xmlChar *)svg);
	xmlNodePtr root=xmlDocGetRootElement(doc);
	FILE *in=fopen(style_css_file_source, "r");
	xmlNodePtr style=xmlNewNode(NULL, (xmlChar *)"style");
	gchar *line=g_malloc(1024*sizeof(gchar));
	gchar *ret=NULL;
	gchar *cur, *cur2, *color;

	xmlNewProp(style, (xmlChar *)"type", (xmlChar *)"text/css");
	if (!line) flo_warn(_("Unable to allocate memory for css read buffer"));
	if (line && in) while ((line=fgets(line, 1024, in))) {
		cur=cur2=line;
		while ((*(cur++)!='\0') && (cur-line<1023)) {
			if (!strncmp(cur, "@SHAPE_COLOR@", 13)) {
				color=style_get_color(c);
				memcpy((void *)cur, (void *)color, 7);
				if (color) g_free(color);
				cur+=7; cur2+=13;
			} else if (!strncmp(cur, "@BORDER_COLOR@", 14)) {
				color=style_get_color(STYLE_OUTLINE_COLOR);
				memcpy((void *)cur, (void *)color, 7);
				if (color) g_free(color);
				cur+=7; cur2+=14;
			} else if (!strncmp(cur, "@SYM_COLOR@", 11)) {
				color=style_get_color(STYLE_TEXT_COLOR);
				memcpy((void *)cur, (void *)color, 7);
				if (color) g_free(color);
				cur+=7; cur2+=11;
			} else if (!strncmp(cur, "@SYM_OUTLINE_COLOR@", 19)) {
				color=style_get_color(STYLE_TEXT_OUTLINE_COLOR);
				memcpy((void *)cur, (void *)color, 7);
				if (color) g_free(color);
				cur+=7; cur2+=19;
			}
			*cur=*(++cur2);
		}
		*cur='\0';
		xmlNodeAddContent(style, (xmlChar *)line);
	}
	g_free(line);
	fclose(in);

	if (strcmp((char *)root->name, "svg"))
		flo_error("element svg expected, but %s found instead", root->name);
	xmlAddPrevSibling(root->children, style);
	save=xmlSaveToBuffer(buffer, NULL, 0);
	xmlSaveTree(save, root);
	xmlSaveClose(save);
	ret=g_strdup((gchar *)xmlBufferContent(buffer));

	xmlFreeDoc(doc);
	xmlBufferFree(buffer);
	END_FUNC
	return ret;
}

/* check for cairo status */
void style_cairo_status_check(cairo_t *cairoctx)
{
	START_FUNC
	if (cairo_status(cairoctx)) {
		if (cairo_status(cairoctx)==CAIRO_STATUS_NO_MEMORY)
			flo_warn(_("Out of memory. Some symbols may not be displayed correctly."));
		else
			flo_warn(_("A cairo error occured: %d"), cairo_status(cairoctx));
	}
	END_FUNC
}

/* Renders a svg handle to a cairo surface at dimensions */
void style_render_svg(cairo_t *cairoctx, RsvgHandle *handle, gdouble w, gdouble h, gboolean keep_ratio, gchar *sub)
{
	START_FUNC
	gdouble xscale, yscale;
	gdouble xoffset=0., yoffset=0.;
	RsvgDimensionData dimensions;
	style_cairo_status_check(cairoctx);
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
	END_FUNC
}

/* create a new symbol */
void style_symbol_new(struct style *style, char *name, char *svg, char *label, char *type)
{
	START_FUNC
	GError *error=NULL;
	gchar *regex=NULL;
	gchar *source=NULL;
	struct symbol *symbol=g_malloc(sizeof(struct symbol));
	memset(symbol, 0, sizeof(struct symbol));
	if ((!type) && name) {
		regex=g_strdup_printf("^%s$", name);
		symbol->id.name=g_regex_new(regex, G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, NULL);
		g_free(regex);
	}
	if (label) {
		symbol->label=g_strdup(label);
	} else if (svg) {
		symbol->source=g_strdup(svg);
		source=style_svg_css_insert(svg, STYLE_KEY_COLOR);
		symbol->svg=rsvg_handle_new_from_data((guint8 *)source, (gsize)strlen(source), &error);
		if (source) g_free(source);
		if (error) flo_fatal(_("Unable to parse svg from layout file: %s"), svg);
	} else { flo_fatal(_("Bad symbol: should have either svg or label :%s"), name); }
	if (type) {
		symbol->id.type=key_action_type_get(type);
		style->type_symbols=g_slist_append(style->type_symbols, (gpointer)symbol);
	} else {
		style->symbols=g_slist_append(style->symbols, (gpointer)symbol);
	}
	flo_debug(TRACE_DEBUG, "[new symbol] name=%s label=%s", name, symbol->label);
	END_FUNC
}

/* free up memory used by the symbol */
void style_symbol_free(gpointer data, gpointer userdata)
{
	START_FUNC
	struct symbol *symbol=(struct symbol *)data;
	if (symbol) {
		if (symbol->id.name) g_regex_unref(symbol->id.name);
		if (symbol->label) g_free(symbol->label);
		if (symbol->svg) rsvg_handle_free(symbol->svg);
		if (symbol->source) g_free(symbol->source);
		g_free(symbol);
	}
	END_FUNC
}

/* return TRUE if the name matches the symbol regexp */
gboolean style_symbol_matches(struct symbol *symbol, gchar *name)
{
	START_FUNC
	END_FUNC
	if (!name) return FALSE;
	return g_regex_match(symbol->id.name, name, G_REGEX_MATCH_ANCHORED|G_REGEX_MATCH_NOTEMPTY, NULL);
}

/* Draws text with cairo */
void style_draw_text(struct style *style, cairo_t *cairoctx, gchar *text, gdouble w, gdouble h)
{
	START_FUNC
	cairo_font_extents_t fe;
	cairo_text_extents_t te;
	PangoFontDescription *fontdesc;
	gchar *fontname;
	const gchar *fontfamilly;
	GtkSettings *settings=NULL;
	cairo_font_slant_t slant;
	gint size=0.0;

	style_cairo_status_check(cairoctx);

	if (settings_get_bool("style/system_font")) {
		settings=gtk_settings_get_default();
		g_object_get(settings, "gtk-font-name", &fontname, NULL);
	} else fontname=settings_get_string("style/font");
	fontdesc=pango_font_description_from_string(fontname?fontname:"sans 10");
	if (fontname) g_free(fontname);
	fontfamilly=pango_font_description_get_family(fontdesc);
	switch(pango_font_description_get_style(fontdesc)) {
		case PANGO_STYLE_NORMAL: slant=CAIRO_FONT_SLANT_NORMAL; break;
		case PANGO_STYLE_OBLIQUE: slant=CAIRO_FONT_SLANT_OBLIQUE; break;
		case PANGO_STYLE_ITALIC: slant=CAIRO_FONT_SLANT_ITALIC; break;
		default: flo_warn(_("unknown slant for font %s: %d"), fontfamilly, pango_font_description_get_style(fontdesc));
			slant=CAIRO_FONT_SLANT_NORMAL; break;
	}

	cairo_save(cairoctx);
	style_cairo_set_color(cairoctx, STYLE_TEXT_OUTLINE_COLOR);
	cairo_select_font_face(cairoctx, fontfamilly, slant, 
		pango_font_description_get_weight(fontdesc)<=500?CAIRO_FONT_WEIGHT_NORMAL:CAIRO_FONT_WEIGHT_BOLD);
	size=pango_font_description_get_size(fontdesc);
	if (pango_font_description_get_size_is_absolute(fontdesc)) {
		size=pango_units_to_double(size);
	}
	cairo_set_font_size(cairoctx, (gdouble)(size)/12800.);
	cairo_text_extents(cairoctx, text, &te);
	cairo_font_extents(cairoctx, &fe);
	if (te.width > w) {
		size=size*w/te.width;
		cairo_set_font_size(cairoctx, (gdouble)(size)/12800.);
		cairo_text_extents(cairoctx, text, &te);
		cairo_font_extents(cairoctx, &fe);
	} 
	if (fe.height > h) {
		cairo_set_font_size(cairoctx, (gdouble)(size*h/fe.height)/12800.);
		cairo_text_extents(cairoctx, text, &te);
		cairo_font_extents(cairoctx, &fe);
	}
	cairo_move_to(cairoctx, (w-te.width)/2.-te.x_bearing, ((h+fe.height)/2.)-fe.descent);
	cairo_set_line_width(cairoctx, 0.1);
	cairo_text_path(cairoctx, text);
	cairo_stroke(cairoctx);
	style_cairo_set_color(cairoctx, STYLE_TEXT_COLOR);
	cairo_move_to(cairoctx, (w-te.width)/2.-te.x_bearing, ((h+fe.height)/2.)-fe.descent);
	cairo_text_path(cairoctx, text);
	cairo_fill(cairoctx);
	cairo_restore(cairoctx);

	pango_font_description_free(fontdesc);
	END_FUNC
}

/* Draw the symbol represented by keyval */
void style_symbol_draw(struct style *style, cairo_t *cairoctx, guint keyval, gdouble w, gdouble h)
{
	START_FUNC
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
		if (*name) style_draw_text(style, cairoctx, name, w, h);
	/* the symbol has a label ==> let's draw it */
	} else if (((struct symbol *)item->data)->label) {
		style_draw_text(style, cairoctx, ((struct symbol *)item->data)->label, w, h);
	/* the symbol must have a svg => draw it */
	} else {
		style_render_svg(cairoctx, ((struct symbol *)item->data)->svg, w, h, TRUE, NULL);
	}
	END_FUNC
}

/* Draw the symbol represented by type */
void style_symbol_type_draw(struct style *style, cairo_t *cairoctx, enum key_action_type type, gdouble w, gdouble h)
{
	START_FUNC
	GSList *item=style->type_symbols;
	while (item && (type!=((struct symbol *)item->data)->id.type)) {
		item=g_slist_next(item);
	}
	if (item) {
		if (((struct symbol *)item->data)->label) {
			style_draw_text(style, cairoctx, ((struct symbol *)item->data)->label, w, h);
		} else {
			style_render_svg(cairoctx, ((struct symbol *)item->data)->svg, w, h, TRUE, NULL);
		}
	} else flo_error(_("No style symbol for action key %d"), type);
	END_FUNC
}

/* callback for layoutreader for shape */
void style_shape_new(struct style *style, char *name, char *svg)
{
	START_FUNC
	struct shape *shape=g_malloc(sizeof(struct shape));
	GError *error=NULL;
	gchar *default_uri, *source;
	memset(shape, 0, sizeof(struct shape));
	if (name) shape->name=g_strdup(name);
	if (svg) {
		source=style_svg_css_insert(svg, STYLE_KEY_COLOR);
		shape->source=(guchar *)g_strdup(svg);
	}
	shape->svg=rsvg_handle_new();
	default_uri=settings_get_string("layout/style");
	rsvg_handle_set_base_uri(shape->svg, style->base_uri?style->base_uri:default_uri);
	if (default_uri) g_free(default_uri);
	rsvg_handle_write(shape->svg, (guchar *)source, (gsize)strlen(source), &error);
	rsvg_handle_close(shape->svg, &error);
	if (error) flo_fatal(_("Unable to parse svg from layout file: svg=\"%s\" error=\"%s\""),
		source, error->message);
	style->shapes=g_slist_append(style->shapes, (gpointer)shape);
	if (!strcmp(name, "default")) style->default_shape=shape;
	flo_debug(TRACE_DEBUG, _("[new shape] name=%s svg=\"%s\""), shape->name, source);
	if (source) g_free(source);
	END_FUNC
}

/* liberate memory used for shape */
void style_shape_free(gpointer data, gpointer userdata)
{
	START_FUNC
	struct shape *shape=(struct shape *)data;
	if (shape) {
		if (shape->name) g_free(shape->name);
		if (shape->source) g_free(shape->source);
		if (shape->svg) rsvg_handle_free(shape->svg);
		if (shape->mask) cairo_surface_destroy(shape->mask);
		g_free(shape);
	}
	END_FUNC
}

/* get the shape by its name */
struct shape *style_shape_get (struct style *style, gchar *name)
{
	START_FUNC
	GSList *item=style->shapes;
	struct shape *ret=NULL;
	if (name) {
		while (item && strcmp(((struct shape *)item->data)->name, name)) item=g_slist_next(item);
		if (item) ret=(struct shape *)item->data;
		else {
			flo_warn(_("Shape %s doesn't exist for selected style."), name);
			ret=style->default_shape;
		}
	} else ret=style->default_shape;
	END_FUNC
	return ret;
}

/* draw the shape to the cairo context. */
void style_shape_draw(struct style *style, struct shape *shape, cairo_t *cairoctx,
	gdouble w, gdouble h, enum style_colours c)
{
	START_FUNC
	gchar *source, *default_uri;
	RsvgHandle *svg;
	GError *error=NULL;
	if (c==STYLE_KEY_COLOR) {
		style_render_svg(cairoctx, shape->svg, w, h, FALSE, NULL);
	} else {
		svg=rsvg_handle_new();
		if (style->base_uri) rsvg_handle_set_base_uri(svg, style->base_uri);
		else {
			default_uri=settings_get_string("layout/style");
			rsvg_handle_set_base_uri(svg, default_uri);
			if (default_uri) g_free(default_uri);
		}
		source=style_svg_css_insert((gchar *)shape->source, c);
		rsvg_handle_write(svg, (guchar *)source, (gsize)strlen(source), &error);
		rsvg_handle_close(svg, &error);
		style_render_svg(cairoctx, svg, w, h, FALSE, NULL);
		if (source) g_free(source);
		if (svg) rsvg_handle_free(svg);
	}
	END_FUNC
}

/* create a mask surface for the shape, if it doesn't already exist */
cairo_surface_t *style_shape_get_mask(struct shape *shape, guint w, guint h)
{
	START_FUNC
	cairo_t *maskctx;
	if ((!shape->mask)||(w!=shape->maskw)||(h!=shape->maskh)) {
		if (shape->mask) cairo_surface_destroy(shape->mask);
		shape->maskw=w; shape->maskh=h;
		shape->mask=cairo_image_surface_create(CAIRO_FORMAT_A8, w, h);
		maskctx=cairo_create(shape->mask);
		style_render_svg(maskctx, shape->svg, w, h, FALSE, NULL);
		cairo_destroy(maskctx);
	}
	END_FUNC
	return shape->mask;
}

/* test if point is inside the mask */
gboolean style_shape_test(struct shape *shape, gint x, gint y, guint w, guint h)
{
	START_FUNC
	cairo_surface_t *mask=style_shape_get_mask(shape, w, h);
	unsigned char *data=cairo_image_surface_get_data(mask);
	int stride=cairo_image_surface_get_stride(mask);
	END_FUNC
	return (x>=0) && (y>=0) && (x<w) && (y<h) && data[(y*stride)+x]>127;
}

/* update the color of one item */
void style_update_color (gchar *source, RsvgHandle **svg, gchar *default_uri)
{
	START_FUNC
	GError *error=NULL;
	gchar *source_with_css;
	if (*svg) {
		rsvg_handle_free(*svg);
		source_with_css=style_svg_css_insert(source, STYLE_KEY_COLOR);
		*svg=rsvg_handle_new();
		if (default_uri) rsvg_handle_set_base_uri(*svg, default_uri);
		rsvg_handle_write(*svg, (guchar *)source_with_css,
			(gsize)strlen((gchar *)source_with_css), &error);
		rsvg_handle_close(*svg, &error);
		if (error) flo_fatal(_("Unable to parse svg from layout file: %s"), source_with_css);
		if (source_with_css) g_free(source_with_css);
	}
	END_FUNC
}

/* update the colors */
void style_update_colors (struct style *style)
{
	START_FUNC
	GSList *list;
	struct symbol *symbol;
	struct shape *shape;
	gchar *default_uri;

	list=style->symbols;
	while (list) {
		symbol=(struct symbol *)list->data;
		style_update_color(symbol->source, &(symbol->svg), NULL);
		list=g_slist_next(list);
	}

	list=style->type_symbols;
	while (list) {
		symbol=(struct symbol *)list->data;
		style_update_color(symbol->source, &(symbol->svg), NULL);
		list=g_slist_next(list);
	}

	list=style->shapes;
	default_uri=settings_get_string("layout/style");
	while (list) {
		shape=(struct shape *)list->data;
		style_update_color((gchar *)shape->source, &(shape->svg),
			style->base_uri?style->base_uri:default_uri);
		list=g_slist_next(list);
	}
	if (default_uri) g_free(default_uri);
	END_FUNC
}

/* draw a style preview to a 32x32 gdk pixbuf 
 * this function is called by the settings dialog */
GdkPixbuf *style_pixbuf_draw(struct style *style)
{
	START_FUNC
	struct shape *shape=style_shape_get(style, NULL);
	GdkPixbuf *temp=rsvg_handle_get_pixbuf(shape->svg);
	GdkPixbuf *ret=gdk_pixbuf_scale_simple(temp, 32, 32, GDK_INTERP_HYPER);
	gdk_pixbuf_unref(temp);
	END_FUNC
	return ret;
}

/* Liberate the pipeline */
void style_sound_pipeline_free(GstElement *pipeline)
{
	START_FUNC
	gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));
	END_FUNC
}

/* Called on sound event */
static gboolean style_sound_bus_call(GstBus *bus, GstMessage *msg, void *user_data)
{
	GError *err;
	switch GST_MESSAGE_TYPE(msg) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, NULL);
			flo_error(_("An error occured while playing sound: %s"), err->message);
			g_error_free(err);
		case GST_MESSAGE_EOS:
			style_sound_pipeline_free((GstElement *)user_data);
			break;
		default: break;
	}
	return TRUE;
}

/* Initialize the pipeline to play a sound */
GstElement *style_sound_setup(const gchar *uri)
{
	START_FUNC
	GstElement *pipeline=NULL;
	GstBus *bus;
	if (uri) {
		pipeline=gst_element_factory_make("playbin", "player");
		g_object_set(G_OBJECT(pipeline), "uri", uri, NULL);
		bus=gst_pipeline_get_bus(GST_PIPELINE(pipeline));
		gst_bus_add_watch(bus, style_sound_bus_call, (gpointer)pipeline);
		gst_object_unref(bus);
	}
	END_FUNC
	return pipeline;
}

/* create a new sound */
void style_sound_new(struct style *style, char *match, char *press, char *release, char *hover)
{
	START_FUNC
	gchar *regex=NULL;
	struct sound *sound=g_malloc(sizeof(struct sound));
	memset(sound, 0, sizeof(struct sound));
	regex=g_strdup_printf("^%s$", match);
	sound->match=g_regex_new(regex, G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, NULL);
	g_free(regex);
	if (press) sound->press=g_strdup(press);
	if (release) sound->release=g_strdup(release);
	if (hover) sound->hover=g_strdup(hover);
	style->sounds=g_slist_append(style->sounds, (gpointer)sound);
	flo_debug(TRACE_DEBUG, "[new sound] match=%s press=%s release=%s hover=%s", match, press, release, hover);
	END_FUNC
}

/* free up memory used by the sound */
void style_sound_free(gpointer data, gpointer userdata)
{
	START_FUNC
	struct sound *sound=(struct sound *)data;
	if (sound) {
		if (sound->match) g_regex_unref(sound->match);
		if (sound->press) g_free(sound->press);
		if (sound->release) g_free(sound->release);
		if (sound->hover) g_free(sound->hover);
		g_free(sound);
	}
	END_FUNC
}

/* return true if the name matches the sound */
gboolean style_sound_matches(struct sound *sound, const gchar *name)
{
	START_FUNC
	gboolean ret;
	if (name) {
		if (sound->match)
			ret=g_regex_match(sound->match, name,
				G_REGEX_MATCH_ANCHORED|G_REGEX_MATCH_NOTEMPTY, NULL);
		else ret=TRUE;
	}
	else ret=FALSE;
	END_FUNC
	return ret;
}

/* play a sound */
void style_sound_play(struct style *style, const gchar *match, enum style_sound_type type)
{
	START_FUNC
	struct sound *sound=NULL;
	GstElement *pipeline=NULL;
	GSList *item=style->sounds;
	while (item && !style_sound_matches((struct sound *)item->data, match)) {
		item=g_slist_next(item);
	}
	if (item) {
		sound=(struct sound *)item->data;
		switch(type) {
			case STYLE_SOUND_PRESS: pipeline=style_sound_setup(sound->press); break;
			case STYLE_SOUND_RELEASE: pipeline=style_sound_setup(sound->release); break;
			case STYLE_SOUND_HOVER: pipeline=style_sound_setup(sound->hover); break;
			default: flo_error(_("Unknown sound type: %d"), type);
		};
		if (pipeline) { gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING); }
	}
	END_FUNC
}

/* create a new style from the layout file */
struct style *style_new(gchar *base_uri)
{
	START_FUNC
	struct layout *layout=NULL;
	struct layout_shape *shape=NULL;
	struct layout_symbol *symbol=NULL;
	struct layout_sound *sound=NULL;
	struct style *style=g_malloc(sizeof(struct style));
	char *uri=base_uri;

	memset(style, 0, sizeof(struct style));
	style->base_uri=base_uri;
	if (!uri) uri=settings_get_string("layout/style");
	layout=layoutreader_new(uri,
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
			style_symbol_new(style, symbol->name, symbol->svg, symbol->label, symbol->type);
			layoutreader_symbol_free(symbol);
		}
	}

	layoutreader_reset(layout);
	layoutreader_element_open(layout, "style");
	if (layoutreader_element_open(layout, "sounds")) {
		while ((sound=layoutreader_sound_new(layout))) {
			style_sound_new(style, sound->match, sound->press, sound->release, sound->hover);
			layoutreader_sound_free(sound);
		}
	}

	layoutreader_free(layout);
	if (!base_uri) g_free(uri);
	END_FUNC
	return style;
}

/* free style */
void style_free(struct style *style)
{
	START_FUNC
	if (style) {
		g_slist_foreach(style->shapes, style_shape_free, NULL);
		g_slist_free(style->shapes);
		g_slist_foreach(style->symbols, style_symbol_free, NULL);
		g_slist_free(style->symbols);
		g_slist_foreach(style->sounds, style_sound_free, NULL);
		g_slist_free(style->sounds);
		g_free(style);
	}
	END_FUNC
}

