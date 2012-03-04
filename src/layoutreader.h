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

/* Node id */
struct layout_id {
	void *object;
	char *name;
};

/* The layout structure contains a pointer to the document
 * and a layout file cursor. */
struct layout {
	xmlDocPtr doc;
	xmlNodePtr cur;
	GSList *ids; /* list of node ids in document */
};

/* This is used for placing extensions around the keyboard.
 * The main extension's placement is void. */
enum layout_placement {
	LAYOUT_VOID,
	LAYOUT_LEFT,
	LAYOUT_RIGHT,
	LAYOUT_TOP,
	LAYOUT_BOTTOM,
	LAYOUT_OVER
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
	unsigned int mod; /* modiier code */
	unsigned char code; /* key code or 0 if none (for code keys) */
	unsigned char *action; /* action depending on type */
	unsigned char *argument; /* argument for action */
};

/* Callback for modifiers */
typedef void (*layout_modifier_cb)(struct layout_modifier *mod, void *object, void *xkb);

/* Data contained in 'key' elements */
struct layout_key {
	char *shape;
	struct layout_pos pos;
	struct layout_size size;
};

/* Data contained in 'extension' elemens */
struct layout_extension {
	char *name;
	char *identifiant;
	enum layout_placement placement;
};

/* Trigger for extensions */
struct layout_trigger {
	void *object;
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
	char *type;
};

/* Data contained in 'sound' elemens */
struct layout_sound {
	char *match;
	char *press;
	char *release;
	char *hover;
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
struct layout_key *layoutreader_key_new(struct layout *layout, layout_modifier_cb mod_cb, void *object, void *xkb);
/* Free the 'key' element data */
void layoutreader_key_free(struct layout_key *key);

/* Get the 'extension' element data (see florence.c) */
struct layout_extension *layoutreader_extension_new(struct layout *layout);
/* Free the 'extension' element data and close the element */
void layoutreader_extension_free(struct layout *layout, struct layout_extension *extension);

/* Read the trigger elements (only onhide for now) */
struct layout_trigger *layoutreader_trigger_new(struct layout *layout);
/* Free the trigger data memory */
void layoutreader_trigger_free(struct layout *layout, struct layout_trigger *trigger);

/* Get the 'shape' element data (see style.c) */
struct layout_shape *layoutreader_shape_new(struct layout *layout);
/* Free the 'shape' element data */
void layoutreader_shape_free(struct layout_shape *shape);

/* Get the 'symbol' element data (see style.c) */
struct layout_symbol *layoutreader_symbol_new(struct layout *layout);
/* Free the 'symbol' element data */
void layoutreader_symbol_free(struct layout_symbol *symbol);

/* Get the 'sound' element data (see style.c) */
struct layout_sound *layoutreader_sound_new(struct layout *layout);
/* Free the 'sound' element data */
void layoutreader_sound_free(struct layout_sound *sound);

/* Open a layout element */
gboolean layoutreader_element_open(struct layout *layout, char *name);
/* Close a layout element */
void layoutreader_element_close(struct layout *layout);
/* Reset layout cursor */
void layoutreader_reset(struct layout *layout);

#endif

