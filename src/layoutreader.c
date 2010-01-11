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

#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>
#include <libxml/relaxng.h>
#include <libxml/xinclude.h>
#include "system.h"
#include "layoutreader.h"
#include "trace.h"
#include "settings.h"

/* Open a layout element */
gboolean layoutreader_element_open(struct layout *layout, char *name)
{
	if (name) while (layout->cur && xmlStrcmp(layout->cur->name, (xmlChar *)name))
		layout->cur=layout->cur->next;
	if (layout->cur) layout->cur=layout->cur->children;
	return layout->cur==NULL?FALSE:TRUE;
}

/* Close a layout element */
void layoutreader_element_close(struct layout *layout)
{
	if (layout->cur && layout->cur->parent) layout->cur=layout->cur->parent->next;
}

/* Reset layout cursor */
void layoutreader_reset(struct layout *layout)
{
	layout->cur=layout->doc->children;
}

/* Initialize an element structure and look for it in the layout document */
void *layoutreader_element_init(struct layout *layout, char *element, size_t size)
{
	void *ret=NULL;
	while (layout->cur && xmlStrcmp(layout->cur->name, (xmlChar *)element))
		layout->cur=layout->cur->next;
	if (layout->cur) {
		ret=g_malloc(size);
		memset(ret, 0, size);
		layout->cur=layout->cur->children;
	}
	return ret;
}

/* Get double from layout text */
double layoutreader_double_get(xmlDocPtr doc, xmlNodePtr cur)
{
	double ret=0.0;
	xmlChar *tmp=xmlNodeListGetString(doc, cur->children, 1);
	ret=g_ascii_strtod((gchar *)tmp, NULL);
	xmlFree(tmp);
	return ret;
}

/* Translate xml placement name to placement enumeration */
enum layout_placement layoutreader_placement_get(struct layout *layout)
{
	enum layout_placement ret=LAYOUT_VOID;
	xmlChar *tmp=xmlNodeListGetString(layout->doc, layout->cur->children, 1);
	if (!xmlStrcmp(tmp, (xmlChar *)"left")) ret=LAYOUT_LEFT;
	else if (!xmlStrcmp(tmp, (xmlChar *)"right")) ret=LAYOUT_RIGHT;
	else if (!xmlStrcmp(tmp, (xmlChar *)"top")) ret=LAYOUT_TOP;
	else if (!xmlStrcmp(tmp, (xmlChar *)"bottom")) ret=LAYOUT_BOTTOM;
	else flo_error(_("Unknown placement %s"), tmp);
	xmlFree(tmp);
	return ret;
}

/* Translate xml key type name into layout key type enumeration */
enum layout_key_type layoutreader_key_type_get(xmlDocPtr doc, xmlNodePtr cur)
{
	enum layout_key_type ret=LAYOUT_NORMAL;
	xmlChar *tmp=xmlNodeListGetString(doc, cur->children, 1);
	if (!xmlStrcmp(tmp, (xmlChar *)"close")) ret=LAYOUT_CLOSE;
	else if (!xmlStrcmp(tmp, (xmlChar *)"config")) ret=LAYOUT_CONFIG;
	else if (!xmlStrcmp(tmp, (xmlChar *)"move")) ret=LAYOUT_MOVE;
	else if (!xmlStrcmp(tmp, (xmlChar *)"bigger")) ret=LAYOUT_BIGGER;
	else if (!xmlStrcmp(tmp, (xmlChar *)"smaller")) ret=LAYOUT_SMALLER;
	else flo_error(_("Unknown action key type %s"), tmp);
	xmlFree(tmp);
	return ret;
}

/* Update the string if the node lang matches locale */
void layoutreader_update_lang(xmlDocPtr doc, xmlNodePtr node, char **update)
{
	xmlChar *lang=xmlNodeGetLang(node);
#ifdef HAVE_LOCALE_H
	if (!lang ||
		!xmlStrncmp(lang, (xmlChar *)setlocale(LC_MESSAGES, NULL),
		xmlStrlen(lang))) {
#else
	if (!lang) {
#endif
		if (*update) xmlFree(*update);
		*update=(char *)xmlNodeListGetString(doc, node, 1);
	}
	xmlFree(lang);
}

/* dump svg from file */
char *layoutreader_svg_get(xmlDocPtr doc, xmlNodePtr cur)
{
	char *ret=NULL;
	xmlBufferPtr buf=xmlBufferCreate();
	xmlOutputBufferPtr outputbuf=xmlOutputBufferCreateBuffer(buf,
		xmlGetCharEncodingHandler(XML_CHAR_ENCODING_UTF8));
	xmlNodeDumpOutput(outputbuf, doc, cur, 0, 1, NULL);
	ret=g_strdup((char *)xmlBufferContent(outputbuf->buffer));
	xmlFree(outputbuf);
	xmlBufferFree(buf);
	return ret;
}

/* Get the 'informatons' element data (see florence.c) */
struct layout_infos *layoutreader_infos_new(struct layout *layout)
{
	struct layout_infos *infos=layoutreader_element_init(layout,
		"informations", sizeof(struct layout_infos));
	xmlNodePtr cur=layout->cur;
	if (infos) for(;cur;cur=cur->next) {
		if (!xmlStrcmp(cur->name, (xmlChar *)"name")) {
			layoutreader_update_lang(layout->doc, cur->children, &infos->name);
		} else if (!xmlStrcmp(cur->name, (xmlChar *)"florence_version")) {
			infos->version=(char *)xmlNodeListGetString(layout->doc, cur->children, 1);
		}
	}
	layoutreader_element_close(layout);
	return infos;
}

/* Free the 'informations' element data */
void layoutreader_infos_free(struct layout_infos *infos)
{
	if (infos) {
		if (infos->name) xmlFree(infos->name);
		if (infos->version) xmlFree(infos->version);
		g_free(infos);
	}
}

/* Get the 'keyboard' element data (see keyboard.c) */
struct layout_size *layoutreader_keyboard_new(struct layout *layout)
{
	struct layout_size *size=layoutreader_element_init(layout,
		"keyboard", sizeof(struct layout_size));
	if (size) for(;layout->cur && xmlStrcmp(layout->cur->name, (xmlChar *)"key");
		layout->cur=layout->cur->next) {
		if (!xmlStrcmp(layout->cur->name, (xmlChar *)"width")) {
			size->w=layoutreader_double_get(layout->doc, layout->cur);
		} else if (!xmlStrcmp(layout->cur->name, (xmlChar *)"height")) {
			size->h=layoutreader_double_get(layout->doc, layout->cur);
		}
	}
	return size;
}

/* Free the 'keyboard' element data */
void layoutreader_keyboard_free(struct layout *layout, struct layout_size *size)
{
	layoutreader_element_close(layout);
	if (size) g_free(size);
}

/* Get the 'key' element data (see key.c) */
struct layout_key *layoutreader_key_new(struct layout *layout)
{
	xmlChar *tmp=NULL;
	xmlNodePtr cur=layout->cur;
	struct layout_key *key=layoutreader_element_init(layout, "key", sizeof(struct layout_key));
	if (key) {
		for(cur=layout->cur;cur;cur=cur->next) {
			if (!xmlStrcmp(cur->name, (xmlChar *)"code")) {
				tmp=xmlNodeListGetString(layout->doc, cur->children, 1);
				key->code=atoi((char *)tmp);
				xmlFree(tmp);
			} else if (!xmlStrcmp(cur->name, (xmlChar *)"action")) {
				key->type=layoutreader_key_type_get(layout->doc, cur);
			} else if (!xmlStrcmp(cur->name, (xmlChar *)"xpos")) {
				key->pos.x=layoutreader_double_get(layout->doc, cur);
			} else if (!xmlStrcmp(cur->name, (xmlChar *)"ypos")) {
				key->pos.y=layoutreader_double_get(layout->doc, cur);
			} else if (!xmlStrcmp(cur->name, (xmlChar *)"width")) {
				key->size.w=layoutreader_double_get(layout->doc, cur);
			} else if (!xmlStrcmp(cur->name, (xmlChar *)"height")) {
				key->size.h=layoutreader_double_get(layout->doc, cur);
			} else if (!xmlStrcmp(cur->name, (xmlChar *)"shape")) {
				key->shape=(char  *)xmlNodeListGetString(layout->doc, cur->children, 1);
			}
		}
		layoutreader_element_close(layout);
	} else layout->cur=cur;
	return key;
}

/* Free the 'key' element data */
void layoutreader_key_free(struct layout_key *key)
{
	if (key) {
		if (key->shape) xmlFree(key->shape);
		g_free(key);
	}
}

/* Get the 'extension' element data (see keyboard.c) */
struct layout_extension *layoutreader_extension_new(struct layout *layout)
{
	struct layout_extension *extension=layoutreader_element_init(layout,
		"extension", sizeof(struct layout_extension));
	if (extension) for(;layout->cur && xmlStrcmp(layout->cur->name, (xmlChar *)"keyboard");
		layout->cur=layout->cur->next) {
		if (!xmlStrcmp(layout->cur->name, (xmlChar *)"name")) {
			layoutreader_update_lang(layout->doc, layout->cur->children, &extension->name);
		} else if (!xmlStrcmp(layout->cur->name, (xmlChar *)"placement")) {
			extension->placement=layoutreader_placement_get(layout);
		} else if (!xmlStrcmp(layout->cur->name, (xmlChar *)"identifiant")) {
			extension->identifiant=(char *)xmlNodeListGetString(layout->doc,
				layout->cur->children, 1);
		}
	}
	return extension;
}

/* Free the 'extension' element data */
void layoutreader_extension_free(struct layout *layout, struct layout_extension *extension)
{
	layoutreader_element_close(layout);
	if (extension) {
		if (extension->name) xmlFree(extension->name);
		if (extension->identifiant) xmlFree(extension->identifiant);
		g_free(extension);
	}
}

/* Get the 'shape' element data (see style.c) */
struct layout_shape *layoutreader_shape_new(struct layout *layout)
{
	struct layout_shape *shape=layoutreader_element_init(layout,
		"shape", sizeof(struct layout_shape));
	xmlNodePtr cur=layout->cur;
	if (shape) for(;cur;cur=cur->next) {
		if (!xmlStrcmp(cur->name, (xmlChar *)"name")) {
			shape->name=(char *)xmlNodeListGetString(layout->doc, cur->children, 1);
		} else if (!xmlStrcmp(cur->name, (xmlChar *)"svg")) {
			shape->svg=layoutreader_svg_get(layout->doc, cur);
		}
	}
	layoutreader_element_close(layout);
	return shape;
}

/* Free the 'shape' element data */
void layoutreader_shape_free(struct layout_shape *shape)
{
	if (shape) {
		if (shape->name) xmlFree(shape->name);
		if (shape->svg) g_free(shape->svg);
		g_free(shape);
	}
}

/* Get the 'symbol' element data (see style.c) */
struct layout_symbol *layoutreader_symbol_new(struct layout *layout)
{
	struct layout_symbol *symbol=layoutreader_element_init(layout,
		"symbol", sizeof(struct layout_symbol));
	xmlNodePtr cur=layout->cur;
	if (symbol) for(;cur;cur=cur->next) {
		if (!xmlStrcmp(cur->name, (xmlChar *)"name")) {
			symbol->name=(char *)xmlNodeListGetString(layout->doc, cur->children, 1);
		} else if (!xmlStrcmp(cur->name, (xmlChar *)"svg")) {
			symbol->svg=layoutreader_svg_get(layout->doc, cur);
		} else if (!xmlStrcmp(cur->name, (xmlChar *)"label")) {
			layoutreader_update_lang(layout->doc, cur->children, &symbol->label);
		} else if (!xmlStrcmp(cur->name, (xmlChar *)"type")) {
			symbol->type=layoutreader_key_type_get(layout->doc, cur);
		}
	}
	layoutreader_element_close(layout);
	return symbol;
}

/* Free the 'shape' element data */
void layoutreader_symbol_free(struct layout_symbol *symbol)
{
	if (symbol) {
		if (symbol->name) xmlFree(symbol->name);
		if (symbol->svg) g_free(symbol->svg);
		if (symbol->label) xmlFree(symbol->label);
		g_free(symbol);
	}
}

/* instanciates a new layout reader. Called in florence.c and style.c
 * can be called either for layout or style files
 * validates the xml layout against the relax-ng validation document
 * return the layoutreader handle */
struct layout *layoutreader_new(char *layoutname, char *defaultname, char *relaxng)
{
	xmlRelaxNGParserCtxtPtr rngctx;
	xmlRelaxNGPtr rng;
	xmlRelaxNGValidCtxtPtr validrng;
        gchar *layoutfile=NULL;
	gchar *tmp=NULL;
	struct stat stat;
	int file_error=0;
	struct layout *layout=g_malloc(sizeof(struct layout));
	gboolean mustfree=FALSE;

	memset(layout, 0, sizeof(struct layout));
	LIBXML_TEST_VERSION

	layoutfile=layoutname;
	if ((!layoutfile) || (layoutfile[0]=='\0') || (file_error=lstat(layoutfile, &stat))) {
		if (file_error) flo_warn (_("Unable to open file %s, using default %s"),
			layoutfile, defaultname);
		layoutfile=(gchar *)defaultname;
	}
	/* if file is a directory, try to find a matching file in the directory */
	if (!lstat(layoutfile, &stat) && S_ISDIR(stat.st_mode)) {
		tmp=g_strdup_printf("%s/florence.style", layoutfile);
		if (lstat(tmp, &stat))
			tmp=g_strdup_printf("%s/florence.xml", layoutfile);
		if (lstat(tmp, &stat)) {
			flo_error(_("%s is a directory and no matching file has been found inside"),
				layoutfile);
			return NULL;
		}
		layoutfile=tmp; mustfree=TRUE;
	}
	layout->doc=xmlReadFile(layoutfile, NULL, XML_PARSE_NOENT|XML_PARSE_XINCLUDE);
	if (!layout->doc) flo_fatal (_("Unable to open file %s."), layoutfile);
	xmlXIncludeProcess(layout->doc);

	rngctx=xmlRelaxNGNewParserCtxt(relaxng);
	rng=xmlRelaxNGParse(rngctx);
	validrng=xmlRelaxNGNewValidCtxt(rng);
	if (0!=xmlRelaxNGValidateDoc(validrng, layout->doc))
		flo_fatal(_("%s does not valdate against %s"), layoutfile, relaxng);
	flo_debug(_("Using file %s"), layoutfile);
	if (mustfree) g_free(layoutfile);

	xmlRelaxNGFreeValidCtxt(validrng);
	xmlRelaxNGFree(rng);
	xmlRelaxNGFreeParserCtxt(rngctx);

	if (!layout->doc || !layout->doc->children)
		flo_error(_("File %s does not contain xml data"), layoutfile);
	else layout->cur=layout->doc->children;
	return layout;
}

/* free the memory used by layout reader */
void layoutreader_free(struct layout *layout)
{
	xmlFreeDoc(layout->doc);
	xmlCleanupParser();
	xmlMemoryDump();
	g_free(layout);
}

