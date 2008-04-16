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
#include "layoutreader.h"
#include <glib.h>

#ifndef LIBXML_READER_ENABLED
#error "Xinclude support not compiled in."));
#endif

#define BUFFER_SIZE 32

int layoutreader_goto(xmlTextReaderPtr reader, xmlChar *name, xmlReaderTypes type, int level)
{
	int ret;
	if (!(xmlTextReaderDepth(reader)==level && xmlTextReaderNodeType(reader)==type &&
		!strcmp(xmlTextReaderConstName(reader), name)) ) {
		while ((ret=xmlTextReaderRead(reader))==1 && !(xmlTextReaderDepth(reader)==level && 
			xmlTextReaderNodeType(reader)==type && !strcmp(xmlTextReaderConstName(reader), name))) {
			switch(type) {
				case XML_READER_TYPE_ELEMENT:
					if (xmlTextReaderDepth(reader)!=level || (xmlTextReaderNodeType(reader)==type &&
						strcmp(xmlTextReaderConstName(reader), name))) { return FALSE; }
					break;
				case XML_READER_TYPE_END_ELEMENT:
					if (xmlTextReaderDepth(reader)<level) { return FALSE; }
			}
		}
	} else { ret=1; }
	return ret==1;
}

xmlChar *layoutreader_readnode(xmlTextReaderPtr reader, int level, xmlChar **name)
{
	static char buffer[BUFFER_SIZE];
	xmlChar *value=NULL;
	int ret=0;

	if (xmlTextReaderDepth(reader)==level) {
		*name=(xmlChar *)xmlTextReaderConstName(reader);
		ret=1;
	} else *name=NULL;
	if (ret==1 && (ret=xmlTextReaderRead(reader))==1 && xmlTextReaderDepth(reader)==(level+1) &&
                xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
                strncpy(buffer, (char *)xmlTextReaderConstValue(reader), BUFFER_SIZE); value=(xmlChar *) buffer;
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) value=NULL;
	}

	return value;
}

enum layout_placement layoutreader_getplacement(xmlChar *placement)
{
	enum layout_placement ret=LAYOUT_VOID;
	if (!strcmp(placement, "left")) ret=LAYOUT_LEFT;
	else if (!strcmp(placement, "right")) ret=LAYOUT_RIGHT;
	else if (!strcmp(placement, "up")) ret=LAYOUT_UP;
	else if (!strcmp(placement, "down")) ret=LAYOUT_DOWN;
	else flo_fatal(_("Unknown placement %s"), placement);
	return ret;
}

double layoutreader_readdouble(xmlTextReaderPtr reader, xmlChar *name, int level)
{
	int ret;
	double val;

	if (!(xmlTextReaderDepth(reader)==level && xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT &&
		!strcmp(xmlTextReaderConstName(reader), name))) {
		while ( ((ret=xmlTextReaderRead(reader))==1) && !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT &&
			!strcmp(xmlTextReaderConstName(reader), name)) ) {
			if (xmlTextReaderDepth(reader)<level) { flo_warn(_("Out of element")); ret=-1; break; }
		}
	} else { ret=1; }
	if (ret==1 && ((ret=xmlTextReaderRead(reader))==1) && xmlTextReaderDepth(reader)==(level+1) &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
		val=g_ascii_strtod(xmlTextReaderConstValue(reader), NULL);
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) {
			flo_fatal(_("layout parse error"));
		}
	} else { flo_fatal(_("layout parse error: %s not found"), name); }

	return val;
}

xmlChar *layoutreader_readstring(xmlTextReaderPtr reader, xmlChar *name, int level)
{
	int ret;
	xmlChar *val=NULL;

	while ( ((ret=xmlTextReaderRead(reader))==1) && !(xmlTextReaderDepth(reader)==level &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) ) {
		if (xmlTextReaderDepth(reader)<level) { flo_warn(_("%s not found"), name); ret=-1; break; }
	}
	if (strcmp(xmlTextReaderConstName(reader), name)) { ret=-1; }
	if (ret==1 && ((ret=xmlTextReaderRead(reader))==1) && xmlTextReaderDepth(reader)==(level+1) &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
		val=g_malloc(sizeof(xmlChar)*strlen(xmlTextReaderConstValue(reader))+1);
		strcpy(val, xmlTextReaderConstValue(reader));
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) {
			flo_fatal(_("layout parse error"));
		}
	}

	return val;
}

void layoutreader_readkey (xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc, void *userdata, int level)
{
	xmlChar *nodename, *nodevalue;
	double xpos, ypos;
	double width=2.0, height=2.0;
	unsigned char code;
	char *shape=NULL;

	while ( xmlTextReaderRead(reader)==1 && !(!strcmp(xmlTextReaderConstName(reader), "key") &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT) ) {
		if (xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) {
			nodevalue=(xmlChar *)layoutreader_readnode(reader, level+1, &nodename);
			if (!nodename) {
				flo_fatal(_("Layout parse error: null node name"));
			} else if (!strcmp(nodename, "code")) {
				code=atoi(nodevalue);
			} else if (!strcmp(nodename, "xpos")) {
				xpos=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "ypos")) {
				ypos=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "width")) {
				width=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "height")) {
				height=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "shape")) {
				shape=g_malloc(strlen(nodevalue));
				strcpy(shape, nodevalue);
			} else { flo_fatal(_("Unknown element: %s"), nodename); }
		}
	}
	
	keyfunc(userdata, shape, code, xpos, ypos, width, height);
	if (shape) g_free(shape);
}

void layoutreader_readinfos(xmlTextReaderPtr reader, layoutreader_infosprocess infosfunc)
{
	xmlChar *version, *name;
	if (!layoutreader_goto(reader, "informations", XML_READER_TYPE_ELEMENT, 1))
		flo_fatal(_("No informations element in layout"));
	name=layoutreader_readstring(reader, "name", 2);
	version=layoutreader_readstring(reader, "author", 2); g_free(version);
	version=layoutreader_readstring(reader, "date", 2); g_free(version);
	version=layoutreader_readstring(reader, "florence_version", 2);
	infosfunc(name, version);
	g_free(name);
	g_free(version);
	if (!layoutreader_goto(reader, "informations", XML_READER_TYPE_END_ELEMENT, 1))
		flo_fatal(_("informations element not closed in layout"));
}

void layoutreader_readcoords (xmlTextReaderPtr reader, double *x, double *y, int level) {
	*x=layoutreader_readdouble(reader, "x", level);
	*y=layoutreader_readdouble(reader, "y", level);
}

GnomeCanvasPathDef *layoutreader_readpath(xmlTextReaderPtr reader, int level) {
	GnomeCanvasPathDef *path;
	double p1x, p1y, p2x, p2y, p3x, p3y;
	xmlChar *nodename, *nodevalue;

	if (!layoutreader_goto(reader, "moveto", XML_READER_TYPE_ELEMENT, level+1))
		flo_fatal(_("Path does not start with moveto"));
	path=gnome_canvas_path_def_new();
	layoutreader_readcoords(reader, &p1x, &p1y, level+2);
	gnome_canvas_path_def_moveto(path, p1x, p1y);
	while ( xmlTextReaderRead(reader)==1 && !(!strcmp(xmlTextReaderConstName(reader), "path") &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT) ) {
		if (xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) {
			nodevalue=(xmlChar *)layoutreader_readnode(reader, level+1, &nodename);
			if (!nodename) {
				flo_fatal(_("Layout parse error: null node name"));
			} else if (!strcmp(nodename, "lineto")) {
				layoutreader_readcoords(reader, &p1x, &p1y, level+2);
				gnome_canvas_path_def_lineto(path, p1x, p1y);
			} else if (!strcmp(nodename, "curveto")) {
				layoutreader_readpt2(reader, &p1x, &p1y, "p1", level+2);
				layoutreader_readpt2(reader, &p2x, &p2y, "p2", level+2);
				layoutreader_readpt2(reader, &p3x, &p3y, "p3", level+2);
				gnome_canvas_path_def_curveto(path, p1x, p1y, p2x, p2y, p3x, p3y);
			} else { flo_fatal(_("Unknown element: %s"), nodename); }
		}
	}
	gnome_canvas_path_def_closepath(path);

	return path;
}

gboolean layoutreader_readpt(xmlTextReaderPtr reader, void *userdata, layoutreader_pointfunc ptfunc) {
	gdouble x, y;
	gboolean ret=FALSE;
	if (layoutreader_goto(reader, "p", XML_READER_TYPE_ELEMENT, 5)) {
		layoutreader_readcoords(reader, &x, &y, 6);
		if (!layoutreader_goto(reader, "p", XML_READER_TYPE_END_ELEMENT, 5))
			flo_fatal(_("Unclosed p element"));
		ptfunc(userdata, x, y);
		ret=TRUE;
	}
	return ret;
}

void layoutreader_readpt2(xmlTextReaderPtr reader, double *x, double *y, char *name, int level) {
	if (!layoutreader_goto(reader, name, XML_READER_TYPE_ELEMENT, level))
		flo_fatal("Found %s: %s expected.", xmlTextReaderConstName(reader), name);
	layoutreader_readcoords(reader, x, y, level+1);
	if (!layoutreader_goto(reader, name, XML_READER_TYPE_END_ELEMENT, level))
		flo_fatal("%s not closed", name);
}

void layoutreader_readdraw(xmlTextReaderPtr reader, void *userdata, layoutreader_itemfunc arrowfunc,
	layoutreader_itemfunc linefunc, layoutreader_itemfunc rectfunc, layoutreader_shapeprocess pathfunc) {
	xmlChar *nodename, *nodevalue;

	if (!layoutreader_goto(reader, "draw", XML_READER_TYPE_ELEMENT, 3))
		flo_fatal(_("Expected draw or label element but %s found"), xmlTextReaderConstName(reader));
	while ( xmlTextReaderRead(reader)==1 && !(!strcmp(xmlTextReaderConstName(reader), "draw") &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT) ) {
		if (xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) {
			nodevalue=(xmlChar *)layoutreader_readnode(reader, 4, &nodename);
			if (!nodename) {
				flo_fatal(_("Layout parse error: depth is %d (expected 4)"), xmlTextReaderDepth(reader));
			} else if (!strcmp(nodename, "arrow")) {
				arrowfunc(reader, userdata);
			} else if (!strcmp(nodename, "line")) {
				linefunc(reader, userdata);
			} else if (!strcmp(nodename, "rect")) {
				rectfunc(reader, userdata);
			} else if (!strcmp(nodename, "path")) {
				pathfunc(NULL, layoutreader_readpath(reader, 4), userdata);
			} else { flo_fatal(_("Unexpected element: %s (primitive expected)"), nodename); }
		}
	}
}

void layoutreader_readsymbol(xmlTextReaderPtr reader, layoutreader_symprocess symfunc, void *userdata)
{
	xmlChar *name=NULL;
	xmlChar *label=NULL;
	xmlChar *nodename=NULL;
	name=layoutreader_readstring(reader, "name", 3);
	label=layoutreader_readstring(reader, "label", 3);
	symfunc(reader, name, label, userdata);
	if (!layoutreader_goto(reader, "symbol", XML_READER_TYPE_END_ELEMENT, 2))
		flo_fatal("Unclosed symbol element :%s", name);
	if (name) g_free(name);
	if (label) g_free(label);
}

void layoutreader_readshape(xmlTextReaderPtr reader, layoutreader_shapeprocess shapefunc, void *userdata)
{
	xmlChar *name;
	GnomeCanvasPathDef *path;
	name=layoutreader_readstring(reader, "name", 3);
	if (!layoutreader_goto(reader, "path", XML_READER_TYPE_ELEMENT, 3))
		flo_fatal(_("Read shape: expected path element found %s"), xmlTextReaderConstName(reader));
	path=layoutreader_readpath(reader, 3);
	shapefunc(name, path, userdata);
	if (!layoutreader_goto(reader, "shape", XML_READER_TYPE_END_ELEMENT, 2))
		flo_fatal("Unclosed shape element :%s", name);
	if (name) g_free(name);
}

void layoutreader_readstyle(xmlTextReaderPtr reader, layoutreader_shapeprocess shapefunc,
        layoutreader_symprocess symfunc, void *userdata) {
	if (!layoutreader_goto(reader, "style", XML_READER_TYPE_ELEMENT, 1))
		flo_fatal(_("No style element in layout"));
	while(layoutreader_goto(reader, "shape", XML_READER_TYPE_ELEMENT, 2))
		layoutreader_readshape(reader, shapefunc, userdata);
	while(layoutreader_goto(reader, "symbol", XML_READER_TYPE_ELEMENT, 2))
		layoutreader_readsymbol(reader, symfunc, userdata);
}


void layoutreader_readkeyboard(xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc, layoutreader_sizeprocess sizefunc,
	void *userdata, int level)
{
	double w, h;
	if (!layoutreader_goto(reader, "keyboard", XML_READER_TYPE_ELEMENT, level))
		flo_fatal(_("No keyboard element in layout"));
	w=layoutreader_readdouble(reader, "width", level+1);
	h=layoutreader_readdouble(reader, "height", level+1);
	sizefunc(userdata, w, h);
	while(layoutreader_goto(reader, "key", XML_READER_TYPE_ELEMENT, level+1))
		layoutreader_readkey(reader, keyfunc, userdata, level+1);
	if (strcmp(xmlTextReaderConstName(reader), "keyboard") &&
		!layoutreader_goto(reader, "keyboard", XML_READER_TYPE_END_ELEMENT, level))
		flo_fatal(_("keyboard element not closed in layout"));
}

int layoutreader_readextension(xmlTextReaderPtr reader, layoutreader_extprocess extfunc, void *userdata)
{
	xmlChar *name, *buffer;
	enum layout_placement placement;
	int order;
	int ret=FALSE;

	if (layoutreader_goto(reader, "extension", XML_READER_TYPE_ELEMENT, 1))
	{
		name=layoutreader_readstring(reader, "name", 2);
		buffer=layoutreader_readstring(reader, "placement", 2);
		placement=layoutreader_getplacement(buffer);
		g_free(buffer);
		extfunc(reader, name, placement, userdata);
		g_free(name);
		if (!layoutreader_goto(reader, "extension", XML_READER_TYPE_END_ELEMENT, 1))
			flo_fatal(_("Unclosed informations element in layout"));
		ret=TRUE;
	}

	return ret;
}

xmlTextReaderPtr layoutreader_new(char *layout)
{
	xmlTextReaderPtr reader;

	LIBXML_TEST_VERSION

	reader=xmlReaderForFile(layout, NULL, XML_PARSE_NOENT); 
	if (!reader) flo_fatal (_("Unable to open file %s"), layout);
	flo_info(_("Using layout file %s"), layout);
	if (xmlTextReaderSchemaValidate(reader, DATADIR "/florence.xsd"))
		flo_fatal(_("Unable to activate schema validation from %s."), DATADIR "/florence.xsd");
	if (!layoutreader_goto(reader, "layout", XML_READER_TYPE_ELEMENT, 0))
		flo_fatal(_("No layout element in layout"));

	return reader;
}

void layoutreader_free(xmlTextReaderPtr reader)
{
	if (xmlTextReaderIsValid(reader) != 1)
		flo_fatal(_("Invalid layout: check %s"), DATADIR "/florence.xsd");
	xmlFreeTextReader(reader);

	xmlCleanupParser();
	xmlMemoryDump();
}

