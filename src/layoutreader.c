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

#ifndef LIBXML_READER_ENABLED
#error "Xinclude support not compiled in."));
#endif

#define BUFFER_SIZE 32

int layoutreader_goto(xmlTextReaderPtr reader, xmlChar *name, xmlReaderTypes type, int level)
{
	int ret;
	while ((ret=xmlTextReaderRead(reader))==1 && !(xmlTextReaderDepth(reader)==level && 
		xmlTextReaderNodeType(reader)==type && !strcmp(xmlTextReaderConstName(reader), name))) {
		switch(type) {
			case XML_READER_TYPE_ELEMENT:
				if (xmlTextReaderDepth(reader)!=level) { return FALSE; }
				break;
			case XML_READER_TYPE_END_ELEMENT:
				if (xmlTextReaderDepth(reader)<level) { return FALSE; }
		}
	}
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

enum key_class layoutreader_getclass(xmlChar *class)
{
	enum key_class ret=KEY_NOCLASS;
	if (!strcmp(class, "default")) ret=KEY_DEFAULT;
	else if (!strcmp(class, "return")) ret=KEY_RETURN;
	else if (!strcmp(class, "backspace")) ret=KEY_BACKSPACE;
	else if (!strcmp(class, "tab")) ret=KEY_TAB;
	else if (!strcmp(class, "shift")) ret=KEY_SHIFT;
	else if (!strcmp(class, "capslock")) ret=KEY_CAPSLOCK;
	else if (!strcmp(class, "leftarrow")) ret=KEY_LEFTARROW;
	else if (!strcmp(class, "rightarrow")) ret=KEY_RIGHTARROW;
	else if (!strcmp(class, "uparrow")) ret=KEY_UPARROW;
	else if (!strcmp(class, "downarrow")) ret=KEY_DOWNARROW;
	else if (!strcmp(class, "home")) ret=KEY_HOME;
	else if (!strcmp(class, "pgup")) ret=KEY_PGUP;
	else if (!strcmp(class, "pgdown")) ret=KEY_PGDOWN;
	else flo_fatal(_("Unknown key class %s"), class);
	return ret;
}

double layoutreader_getdouble(xmlTextReaderPtr reader, xmlChar *name, int level)
{
	int ret;
	double val;

	while (((ret=xmlTextReaderRead(reader))==1) && !(xmlTextReaderDepth(reader)==level && 
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT && 
		!strcmp(xmlTextReaderConstName(reader), name))) {
		if (xmlTextReaderDepth(reader)<level) { flo_warn(_("Out of element")); ret=-1; break; }
	}
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

xmlChar *layoutreader_getstring(xmlTextReaderPtr reader, xmlChar *name, int level)
{
	int ret;
	xmlChar *val;

	while (((ret=xmlTextReaderRead(reader))==1) && !(xmlTextReaderDepth(reader)==level && 
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT && 
		!strcmp(xmlTextReaderConstName(reader), name))) {
		if (xmlTextReaderDepth(reader)<level) { flo_info(_("Out of element")); ret=-1; break; }
	}
	if (ret==1 && ((ret=xmlTextReaderRead(reader))==1) && xmlTextReaderDepth(reader)==(level+1) &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
		val=g_malloc((sizeof(xmlChar)*strlen(xmlTextReaderConstValue(reader)+1)));
		strcpy(val, xmlTextReaderConstValue(reader));
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) {
			flo_fatal(_("layout parse error"));
		}
	} else { flo_fatal(_("layout parse error: %s not found"), name); }

	return val;
}

void layoutreader_readkey (xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc, void *userdata, int level)
{
	static char label_buffer[BUFFER_SIZE];
	xmlChar *nodename, *nodevalue;
	double xpos, ypos;
	double width=2.0, height=2.0;
	unsigned char code;
	char *label=NULL;
	enum key_class class=KEY_DEFAULT;

	while ( xmlTextReaderRead(reader)==1 && !(!strcmp(xmlTextReaderConstName(reader), "key") &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT) )
	{
		if (xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) {
			nodevalue=(xmlChar *)layoutreader_readnode(reader, level+1, &nodename);
			if (!strcmp(nodename, "code")) {
				code=atoi(nodevalue);
			} else if (!strcmp(nodename, "xpos")) {
				xpos=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "ypos")) {
				ypos=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "width")) {
				width=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "height")) {
				height=g_ascii_strtod(nodevalue, NULL);
			} else if (!strcmp(nodename, "label")) {
				strcpy(label_buffer, nodevalue); label=label_buffer;
			} else if (!strcmp(nodename, "class")) {
				class=layoutreader_getclass(nodevalue);
			} else { flo_fatal(_("Unknown element: %s"), nodename); }
		}
	}
	
	keyfunc(userdata, class, code, xpos, ypos, width, height, label);
}

void layoutreader_readkeyboard(xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc, layoutreader_sizeprocess sizefunc,
	void *userdata, int level)
{
	double w, h;
	if (!layoutreader_goto(reader, "keyboard", XML_READER_TYPE_ELEMENT, level))
		flo_fatal(_("No keyboard element in layout"));
	w=layoutreader_getdouble(reader, "width", level+1);
	h=layoutreader_getdouble(reader, "height", level+1);
	sizefunc(userdata, w, h);
	while(layoutreader_goto(reader, "key", XML_READER_TYPE_ELEMENT, level+1))
		layoutreader_readkey(reader, keyfunc, userdata, level+1);
	if (strcmp(xmlTextReaderConstName(reader), "keyboard") &&
		!layoutreader_goto(reader, "keyboard", XML_READER_TYPE_END_ELEMENT, level))
		flo_fatal(_("keyboard element not closed in layout"));
}

void layoutreader_readinfos(xmlTextReaderPtr reader, layoutreader_infosprocess infosfunc)
{
	xmlChar *version, *name;
	if (!layoutreader_goto(reader, "informations", XML_READER_TYPE_ELEMENT, 1))
		flo_fatal(_("No informations element in layout"));
	name=layoutreader_getstring(reader, "name", 2);
	version=layoutreader_getstring(reader, "florence_version", 2);
	infosfunc(name, version);
	g_free(name);
	g_free(version);
	if (!layoutreader_goto(reader, "informations", XML_READER_TYPE_END_ELEMENT, 1))
		flo_fatal(_("informations element not closed in layout"));
}

int layoutreader_readextension(xmlTextReaderPtr reader, layoutreader_extprocess extfunc, void *userdata)
{
	xmlChar *name, *buffer;
	enum layout_placement placement;
	int order;
	int ret=FALSE;

	if (layoutreader_goto(reader, "extension", XML_READER_TYPE_ELEMENT, 1))
	{
		name=layoutreader_getstring(reader, "name", 2);
		buffer=layoutreader_getstring(reader, "placement", 2);
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

