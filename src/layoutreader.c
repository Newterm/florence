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

#include <glib.h>
#include "system.h"
#include "layoutreader.h"
#include "trace.h"
#include "settings.h"

#ifndef LIBXML_READER_ENABLED
#error "Xinclude support not compiled in."
#endif

#define BUFFER_SIZE 32
#define FLO_DEFAULT_LAYOUT DATADIR "/florence.layout"

/* Move to the first occurence of the named element which is at the specified level
 * return false if no element were found */
int layoutreader_goto(xmlTextReaderPtr reader, xmlChar *name, xmlReaderTypes type, int level)
{
	int ret;
	if (!(xmlTextReaderDepth(reader)==level && xmlTextReaderNodeType(reader)==type &&
		!xmlStrcmp(xmlTextReaderConstName(reader), name)) ) {
		while ((ret=xmlTextReaderRead(reader))==1 && !(xmlTextReaderDepth(reader)==level && 
			xmlTextReaderNodeType(reader)==type && !xmlStrcmp(xmlTextReaderConstName(reader), name))) {
			switch(type) {
				case XML_READER_TYPE_ELEMENT:
					if (xmlTextReaderDepth(reader)!=level || (xmlTextReaderNodeType(reader)==type &&
						xmlStrcmp(xmlTextReaderConstName(reader), name))) { return FALSE; }
					break;
				case XML_READER_TYPE_END_ELEMENT:
					if (xmlTextReaderDepth(reader)<level) { return FALSE; }
					break;
				default:
					break;
			}
		}
	} else { ret=1; }
	return ret==1;
}

/* Read the value of the current node and put the name of the node in name
 * return the value of the node */
xmlChar *layoutreader_readnode(xmlTextReaderPtr reader, int level, xmlChar **name)
{
	static xmlChar buffer[BUFFER_SIZE];
	xmlChar *value=NULL;
	int ret=0;

	if (xmlTextReaderDepth(reader)==level) {
		*name=(xmlChar *)xmlTextReaderConstName(reader);
		ret=1;
	} else *name=NULL;
	if (ret==1 && (ret=xmlTextReaderRead(reader))==1 && xmlTextReaderDepth(reader)==(level+1) &&
                xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
                strncpy((char *)buffer, (char *)xmlTextReaderConstValue(reader), BUFFER_SIZE); value=(xmlChar *) buffer;
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) value=NULL;
	}

	return value;
}

/* translate xml placement name to placement enumeration */
enum layout_placement layoutreader_getplacement(xmlChar *placement)
{
	enum layout_placement ret=LAYOUT_VOID;
	if (!xmlStrcmp(placement, (xmlChar *)"left")) ret=LAYOUT_LEFT;
	else if (!xmlStrcmp(placement, (xmlChar *)"right")) ret=LAYOUT_RIGHT;
	else if (!xmlStrcmp(placement, (xmlChar *)"top")) ret=LAYOUT_TOP;
	else if (!xmlStrcmp(placement, (xmlChar *)"bottom")) ret=LAYOUT_BOTTOM;
	else flo_fatal(_("Unknown placement %s"), placement);
	return ret;
}

/* look for the named xml element and returns its value as double 
 * fatals if not found */
double layoutreader_readdouble(xmlTextReaderPtr reader, xmlChar *name, int level)
{
	int ret;
	double val;

	if (!(xmlTextReaderDepth(reader)==level && xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT &&
		!xmlStrcmp(xmlTextReaderConstName(reader), name))) {
		while ( ((ret=xmlTextReaderRead(reader))==1) && !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT &&
			!xmlStrcmp(xmlTextReaderConstName(reader), name)) ) {
			if (xmlTextReaderDepth(reader)<level) { flo_warn(_("Out of element")); ret=-1; break; }
		}
	} else { ret=1; }
	if (ret==1 && ((ret=xmlTextReaderRead(reader))==1) && xmlTextReaderDepth(reader)==(level+1) &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
		val=g_ascii_strtod((gchar *)xmlTextReaderConstValue(reader), NULL);
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) {
			flo_fatal(_("layout parse error"));
		}
	} else { flo_fatal(_("layout parse error: %s not found"), name); }

	return val;
}

/* look for the named xml element and returns the value as a string
 * fatals if not found */
xmlChar *layoutreader_readstring(xmlTextReaderPtr reader, xmlChar *name, int level)
{
	int ret;
	xmlChar *val=NULL;

	while ( ((ret=xmlTextReaderRead(reader))==1) && !(xmlTextReaderDepth(reader)==level &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) ) {
		if (xmlTextReaderDepth(reader)<level) { flo_warn(_("%s not found"), name); ret=-1; break; }
	}
	if (xmlStrcmp(xmlTextReaderConstName(reader), name)) { ret=-1; }
	if (ret==1 && ((ret=xmlTextReaderRead(reader))==1) && xmlTextReaderDepth(reader)==(level+1) &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_TEXT && xmlTextReaderHasValue(reader)) {
		val=g_malloc(sizeof(xmlChar)*(xmlStrlen(xmlTextReaderConstValue(reader))+1));
		val[0]=(xmlChar)'\0'; xmlStrcat(val, xmlTextReaderConstValue(reader));
		if (ret!=1 || ((ret=xmlTextReaderRead(reader))!=1) || !(xmlTextReaderDepth(reader)==level &&
			xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT)) {
			flo_fatal(_("layout parse error"));
		}
	}

	return val;
}

/* Read a key definition in the xml file and calls the keyfunc callback with the specification of the key as arguments */
void layoutreader_readkey (xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc, void *userdata1, void *userdata2, int level)
{
	xmlChar *nodename, *nodevalue;
	double xpos, ypos;
	double width=2.0, height=2.0;
	unsigned char code;
	char *shape=NULL;

	while ( xmlTextReaderRead(reader)==1 && !(!xmlStrcmp(xmlTextReaderConstName(reader), (xmlChar *)"key") &&
		xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT) ) {
		if (xmlTextReaderNodeType(reader)==XML_READER_TYPE_ELEMENT) {
			nodevalue=(xmlChar *)layoutreader_readnode(reader, level+1, &nodename);
			if (!nodename) {
				flo_fatal(_("Layout parse error: null node name"));
			} else if (!xmlStrcmp(nodename, (xmlChar *)"code")) {
				code=atoi((char *)nodevalue);
			} else if (!xmlStrcmp(nodename, (xmlChar *)"xpos")) {
				xpos=g_ascii_strtod((gchar *)nodevalue, NULL);
			} else if (!xmlStrcmp(nodename, (xmlChar *)"ypos")) {
				ypos=g_ascii_strtod((gchar *)nodevalue, NULL);
			} else if (!xmlStrcmp(nodename, (xmlChar *)"width")) {
				width=g_ascii_strtod((gchar *)nodevalue, NULL);
			} else if (!xmlStrcmp(nodename, (xmlChar *)"height")) {
				height=g_ascii_strtod((gchar *)nodevalue, NULL);
			} else if (!xmlStrcmp(nodename, (xmlChar *)"shape")) {
				shape=g_malloc(sizeof(gchar)*(1+xmlStrlen(nodevalue)));
				shape[0]='\0'; xmlStrcat((xmlChar *)shape, nodevalue);
			} else { flo_fatal(_("Unknown element: %s"), nodename); }
		}
	}
	
	keyfunc(userdata1, shape, code, xpos, ypos, width, height, userdata2);
	if (shape) g_free(shape);
}

/* Read an info node from the xml file and calls the infosprocess callback with the informations read as argument */
void layoutreader_readinfos(xmlTextReaderPtr reader, layoutreader_infosprocess infosfunc)
{
	xmlChar *version, *name;
	if (!layoutreader_goto(reader, (xmlChar *)"informations", XML_READER_TYPE_ELEMENT, 1))
		flo_fatal(_("No informations element in layout"));
	name=layoutreader_readstring(reader, (xmlChar *)"name", 2);
	version=layoutreader_readstring(reader, (xmlChar *)"author", 2); g_free(version); /* not used */
	version=layoutreader_readstring(reader, (xmlChar *)"date", 2); g_free(version); /* not used */
	version=layoutreader_readstring(reader, (xmlChar *)"florence_version", 2);
	if (infosfunc) infosfunc((char *)name, (char *)version);
	g_free(name);
	g_free(version);
	if (!layoutreader_goto(reader, (xmlChar *)"informations", XML_READER_TYPE_END_ELEMENT, 1))
		flo_fatal(_("informations element not closed in layout"));
}

void layoutreader_readsymbol(xmlTextReaderPtr reader, layoutreader_symprocess symfunc, void *userdata)
{
	xmlChar *name=NULL;
	xmlChar *label=NULL;
	xmlChar *svg=NULL;
	name=layoutreader_readstring(reader, (xmlChar *)"name", 3);
	label=layoutreader_readstring(reader, (xmlChar *)"label", 3);
	if (layoutreader_goto(reader, (xmlChar *)"svg", XML_READER_TYPE_ELEMENT, 3))
		svg=xmlTextReaderReadOuterXml(reader);
	symfunc((char *)name, (char *)svg, (char *)label, userdata);
	if (!layoutreader_goto(reader, (xmlChar *)"symbol", XML_READER_TYPE_END_ELEMENT, 2))
		flo_fatal(_("Unclosed symbol element: %s"), name);
	if (name) g_free(name);
	if (label) g_free(label);
	if (svg) xmlFree(svg);
}

/* Read a shape node see style.c */
void layoutreader_readshape(xmlTextReaderPtr reader, layoutreader_shapeprocess shapefunc, void *userdata)
{
	xmlChar *name=NULL;
	xmlChar *svg=NULL;
	name=layoutreader_readstring(reader, (xmlChar *)"name", 3);
	if (!layoutreader_goto(reader, (xmlChar *)"svg", XML_READER_TYPE_ELEMENT, 3))
		flo_fatal(_("Read shape: 'svg' expected: '%s' found "), xmlTextReaderConstName(reader));
	svg=xmlTextReaderReadOuterXml(reader);
	shapefunc((char *)name, (char *)svg, userdata);
	if (!layoutreader_goto(reader, (xmlChar *)"shape", XML_READER_TYPE_END_ELEMENT, 2))
		flo_fatal(_("Unclosed shape element: %s %s"), name, xmlTextReaderConstName(reader));
	if (svg) xmlFree(svg);
	if (name) g_free(name);
}

/* read a style: see style.c */
void layoutreader_readstyle(xmlTextReaderPtr reader, layoutreader_shapeprocess shapefunc,
        layoutreader_symprocess symfunc, void *userdata) {
	if (!layoutreader_goto(reader, (xmlChar *)"style", XML_READER_TYPE_ELEMENT, 1))
		flo_fatal(_("No style element in layout"));
	while(layoutreader_goto(reader, (xmlChar *)"shape", XML_READER_TYPE_ELEMENT, 2))
		layoutreader_readshape(reader, shapefunc, userdata);
	while(layoutreader_goto(reader, (xmlChar *)"symbol", XML_READER_TYPE_ELEMENT, 2))
		layoutreader_readsymbol(reader, symfunc, userdata);
}


/* read a keyboard: see keyboard.c */
void layoutreader_readkeyboard(xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc, layoutreader_sizeprocess sizefunc,
	void *userdata1, void *userdata2, int level)
{
	double w, h;
	if (!layoutreader_goto(reader, (xmlChar *)"keyboard", XML_READER_TYPE_ELEMENT, level))
		flo_fatal(_("No keyboard element in layout"));
	w=layoutreader_readdouble(reader, (xmlChar *)"width", level+1);
	h=layoutreader_readdouble(reader, (xmlChar *)"height", level+1);
	sizefunc(userdata1, w, h);
	while(layoutreader_goto(reader, (xmlChar *)"key", XML_READER_TYPE_ELEMENT, level+1))
		layoutreader_readkey(reader, keyfunc, userdata1, userdata2, level+1);
	if (xmlStrcmp(xmlTextReaderConstName(reader), (xmlChar *)"keyboard") &&
		!layoutreader_goto(reader, (xmlChar *)"keyboard", XML_READER_TYPE_END_ELEMENT, level))
		flo_fatal(_("keyboard element not closed in layout"));
}

/* Read an extension and returns the keyboard generated by the callback function or NULL when there is no more extension. */
void *layoutreader_readextension(xmlTextReaderPtr reader, layoutreader_keyboardprocess keyboardfunc, void *userdata)
{
	xmlChar *name, *buffer;
	enum layout_placement placement;
	void *ret=NULL;

	if (layoutreader_goto(reader, (xmlChar *)"extension", XML_READER_TYPE_ELEMENT, 1))
	{
		name=layoutreader_readstring(reader, (xmlChar *)"name", 2);
		buffer=layoutreader_readstring(reader, (xmlChar *)"placement", 2);
		placement=layoutreader_getplacement(buffer);
		g_free(buffer);
		ret=keyboardfunc(reader, 2, (gchar *)name, placement, userdata);
		g_free(name);
		if (!layoutreader_goto(reader, (xmlChar *)"extension", XML_READER_TYPE_END_ELEMENT, 1))
			flo_fatal(_("Unclosed informations element in layout"));
	}

	return ret;
}

/* instanciates a new layout reader. Called in florence.c
 * return the layoutreader handle */
xmlTextReaderPtr layoutreader_new(void)
{
	xmlTextReaderPtr reader;
	char *layoutfile=NULL;
	static char *defaultlayout=FLO_DEFAULT_LAYOUT;

	LIBXML_TEST_VERSION

        layoutfile=settings_get_string("layout/file");
	if (layoutfile==NULL || layoutfile[0]=='\0') layoutfile=defaultlayout;
	reader=xmlReaderForFile(layoutfile, NULL, XML_PARSE_NOENT); 
	if (!reader) flo_fatal (_("Unable to open file %s"), layoutfile);
	flo_info(_("Using layout file %s"), layoutfile);
	if (xmlTextReaderRelaxNGValidate(reader, DATADIR "/relaxng/florence.rng"))
		flo_fatal(_("Unable to activate schema validation from %s."), DATADIR "/florence.rng");
	if (!layoutreader_goto(reader, (xmlChar *)"layout", XML_READER_TYPE_ELEMENT, 0))
		flo_fatal(_("No layout element in layout"));

	return reader;
}

/* free the memory used by layout reader */
void layoutreader_free(xmlTextReaderPtr reader)
{
	if (xmlTextReaderIsValid(reader) != 1)
		flo_fatal(_("Invalid layout: check %s"), DATADIR "/florence.rnc");
	xmlFreeTextReader(reader);

	xmlCleanupParser();
	xmlMemoryDump();
}

