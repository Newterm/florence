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

#ifndef FLO_LAYOUTREADER
#define FLO_LAYOUTREADER

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "config.h"

/* The layout structure contains a pointer to the document
 * and a layout file cursor. */
struct layout {
	xmlDocPtr doc;
	xmlNodePtr cur;
};

/* This is used for placing extensions around the keyboard.
 * The main extension's placement is void. */
enum layout_placement {
	LAYOUT_VOID,
	LAYOUT_LEFT,
	LAYOUT_RIGHT,
	LAYOUT_TOP,
	LAYOUT_BOTTOM
};

/* Keys can be of the following types */
enum layout_key_type {
	LAYOUT_NORMAL,
	LAYOUT_CLOSE,
	LAYOUT_REDUCE,
	LAYOUT_CONFIG,
	LAYOUT_MOVE,
	LAYOUT_BIGGER,
	LAYOUT_SMALLER,
	LAYOUT_SWITCH
};

/* Data contained in the 'informations' element */
struct layout_infos {
	char *name;
	char *version;
};

/* Size of an element */
struct layout_size {
	double w, h;
};

/* Position of an element */
struct layout_pos {
	double x, y;
};

/* Data contained in the 'modifier' elements */
struct layout_modifier {
	unsigned int mod;
	enum layout_key_type type;
};

/* Data contained in 'key' elements */
struct layout_key {
	char *shape;
	unsigned char code;
	struct layout_modifier **actions; /* NULL terminated list of actions. */
	struct layout_pos pos;
	struct layout_size size;
};

/* Data contained in 'extension' elemens */
struct layout_extension {
	char *name;
	char *identifiant;
	enum layout_placement placement;
};

/* Data contained in 'shape' elemens */
struct layout_shape {
	char *name;
	char *svg;
};

/* Data contained in 'symbol' elemens */
struct layout_symbol {
	char *name;
	char *svg;
	char *label;
	enum layout_key_type type;
};

/* Create a reader for the filename provided */
struct layout *layoutreader_new(char *layoutname, char *defaultname, char *relaxng);
/* liberate memory for the reader */
void layoutreader_free(struct layout *layout);

/* Get the 'informatons' element data (see florence.c) */
struct layout_infos *layoutreader_infos_new(struct layout *layout);
/* Free the 'informations' element data */
void layoutreader_infos_free(struct layout_infos *infos);

/* Get the 'keyboard' element data (see keyboard.c) */
struct layout_size *layoutreader_keyboard_new(struct layout *layout);
/* Free the 'keyboard' element data and close the element */
void layoutreader_keyboard_free(struct layout *layout, struct layout_size *size);

/* Get the 'key' element data (see key.c) */
struct layout_key *layoutreader_key_new(struct layout *layout);
/* Free the 'key' element data */
void layoutreader_key_free(struct layout_key *key);

/* Get the 'extension' element data (see florence.c) */
struct layout_extension *layoutreader_extension_new(struct layout *layout);
/* Free the 'extension' element data and close the element */
void layoutreader_extension_free(struct layout *layout, struct layout_extension *extension);

/* Get the 'shape' element data (see style.c) */
struct layout_shape *layoutreader_shape_new(struct layout *layout);
/* Free the 'shape' element data */
void layoutreader_shape_free(struct layout_shape *shape);

/* Get the 'symbol' element data (see style.c) */
struct layout_symbol *layoutreader_symbol_new(struct layout *layout);
/* Free the 'symbol' element data */
void layoutreader_symbol_free(struct layout_symbol *symbol);

/* Open a layout element */
gboolean layoutreader_element_open(struct layout *layout, char *name);
/* Close a layout element */
void layoutreader_element_close(struct layout *layout);
/* Reset layout cursor */
void layoutreader_reset(struct layout *layout);

#endif

