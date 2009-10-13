/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2008, 2009 François Agrech

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

#include "trace.h"
#include "system.h"
#include <glib/gprintf.h>
#include <glib/gutils.h>
#include <stdio.h>
#include <stdarg.h>

/* This is a boolean: if TRUE, the module will print debug information */
static int trace_debug;
/* the buffer records messages already printed. Used not to print the same message twice */
/* TODO: use a hash instead of the full string */
static GSList *trace_buffer=NULL;

/* prints a message to stream f, ignores it if there exists a duplicate in the buffer. */
/* TODO: make an option to trace multiple duplicate lines when useful. */
void trace_msg(char *prefix, FILE *f, char *s, va_list args)
{
	GSList *list=trace_buffer;
	gchar *str=g_strdup_vprintf((gchar *)s, args);
	gboolean ignore=FALSE;
	while (list) {
		if (!strcmp(list->data, str)) {
			ignore=TRUE;
			break;
		}
		list=list->next;
	}
	if (!ignore) {
		trace_buffer=g_slist_append(trace_buffer, str);
		g_fprintf(f, "%s%s", prefix, str);
		g_fprintf(f, "\n");
	}
}

/********************/
/* public functions */
/********************/

/* initializes the trace module. Must be called before any trace function
 * debug is a boolean. If it's true, the trace module will print debug informations */
void trace_init(int debug)
{
	trace_debug=debug;
}

/* liberate any memory used by the trace module */
void trace_exit()
{
	GSList *list=trace_buffer;
	while (list) {
		g_free(list->data);
		list=list->next;
	}
	g_slist_free(trace_buffer);
}

void flo_fatal(char *s, ...)
{
	va_list ap;
        g_fprintf(stderr, _("FATAL ERROR: "));
	va_start(ap, s);
        g_vfprintf(stderr, s, ap);
	g_fprintf(stderr, "\n");
	va_end(ap);
	if (!g_getenv("FLO_DEBUG") || strcmp(g_getenv("FLO_DEBUG"), "1")) {
		g_fprintf(stderr, _("If you need help, please rerun with the -d switch (debug)\n"));
		g_fprintf(stderr, _("and send the output to f.agrech@gmail.com\n\n"));
	}
        exit(EXIT_FAILURE);
}

void flo_info(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	trace_msg("", stdout, s, ap);
	va_end(ap);
}

void flo_warn(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	trace_msg("WARNING: ", stderr, s, ap);
	va_end(ap);
}

void flo_error(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	trace_msg("ERROR: ", stderr, s, ap);
	va_end(ap);
}

void flo_debug(char *s, ...)
{
	if (trace_debug) {
		va_list ap;
		va_start(ap, s);
		trace_msg("", stdout, s, ap);
		va_end(ap);
	}
}

