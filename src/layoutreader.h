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
#include <libxml/xmlreader.h>

enum layout_placement {
	LAYOUT_VOID,
	LAYOUT_LEFT,
	LAYOUT_RIGHT,
	LAYOUT_TOP,
	LAYOUT_BOTTOM
};

/* callbacks */
typedef void (*layoutreader_infosprocess) (char *name, char *version);
typedef void (*layoutreader_keyprocess) (void *userdata, char *shape, unsigned char code,
	double xpos, double ypos, double width, double height);
typedef void (*layoutreader_sizeprocess) (void *userdata, double width, double height);
typedef void (*layoutreader_extprocess) (xmlTextReaderPtr reader, char *name, enum layout_placement placement,
	void *userdata);
typedef void (*layoutreader_symprocess) (xmlTextReaderPtr reader, char *name, char *label, void *userdata);
typedef void (*layoutreader_shapeprocess) (char *name, GnomeCanvasPathDef *path, void *userdata);
typedef void (*layoutreader_itemfunc) (xmlTextReaderPtr reader, void *userdata);
typedef void (*layoutreader_pointfunc) (void *userdata, double x, double y);

xmlTextReaderPtr layoutreader_new(char *layout);
void layoutreader_free(xmlTextReaderPtr reader);
double layoutreader_readdouble(xmlTextReaderPtr reader, xmlChar *name, int level);
void layoutreader_readinfos(xmlTextReaderPtr reader, layoutreader_infosprocess infosfunc);
void layoutreader_readkeyboard(xmlTextReaderPtr reader, layoutreader_keyprocess keyfunc,
	layoutreader_sizeprocess sizefunc, void *userdata, int level);
int layoutreader_readextension(xmlTextReaderPtr reader, layoutreader_extprocess extfunc, void *userdata);
void layoutreader_readstyle(xmlTextReaderPtr reader, layoutreader_shapeprocess shapefunc, 
	layoutreader_symprocess symfunc, void *userdata);
void layoutreader_readdraw(xmlTextReaderPtr reader, void *userdata, layoutreader_itemfunc arrowfunc,
	layoutreader_itemfunc linefunc, layoutreader_itemfunc rectfunc, layoutreader_shapeprocess pathfunc);
gboolean layoutreader_readpt(xmlTextReaderPtr reader, void *userdata, layoutreader_pointfunc ptfunc);
void layoutreader_readpt2(xmlTextReaderPtr reader, double *x, double *y, char *name, int level);

