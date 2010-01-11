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

#include "config.h"
#include <gtk/gtk.h>
#ifdef ENABLE_AT_SPI
#include <cspi/spi.h>
#endif

/* sets the window icon to florence.svg */
void tools_set_icon (GtkWindow *window);
/* open a YES/NO dialog window and return the user response */
gint tools_dialog(const gchar *title, GtkWindow *parent,
	const gchar *accept, const gchar *reject, const gchar *text);
#ifdef ENABLE_AT_SPI
/* position a window near the specified object */
void tools_window_move(GtkWindow *window, Accessible *object);
#endif

#if !GLIB_CHECK_VERSION(2,14,0)
#define GRegex gchar
#define G_REGEX_OPTIMIZE NULL
#define G_REGEX_MATCH_ANCHORED NULL
#define g_regex_new(text, null0, null1, null2) tools_regex_new(text)
#define g_regex_unref tools_regex_free
#define g_regex_match(regex, text, null0, null1) tools_regex_match(regex, text)
#define g_regex_replace_literal(old, source, null0, null1, new, null2, null3) tools_regex_replace_literal(old, source, new)
gchar *tools_regex_new(gchar *regex);
void tools_regex_free(gchar *regex);
gboolean tools_regex_match(gchar *regex, gchar *text);
gchar *tools_regex_replace_literal(gchar *old, gchar *source, gchar *new);
#endif
