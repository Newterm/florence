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

#include "trace.h"
#include "system.h"
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdarg.h>

/* This is a boolean: if TRUE, the module will print debug information */
static int trace_debug;

/* initializes the trace module. Must be called before any trace function
 * debug is a boolean. If it's true, the trace module will print debug informations */
void trace_init(int debug)
{
	trace_debug=debug;
}

void flo_fatal(char *s, ...)
{
	va_list ap;
        g_fprintf(stderr, _("FATAL ERROR: "));
	va_start(ap, s);
        g_vfprintf(stderr, s, ap);
	g_fprintf(stderr, "\n");
	va_end(ap);
	if (!getenv("FLO_DEBUG") || strcmp(getenv("FLO_DEBUG"), "1")) {
		g_fprintf(stderr, _("If you need help, please rerun with the -d switch (debug)\n"));
		g_fprintf(stderr, _("and send the output to f.agrech@gmail.com\n\n"));
	}
        exit(EXIT_FAILURE);
}

void flo_info(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
        g_vprintf(s, ap);
        g_printf("\n");
	va_end(ap);
}

void flo_warn(char *s, ...)
{
	va_list ap;
	g_fprintf(stderr, _("WARNING: "));
	va_start(ap, s);
        g_vfprintf(stderr, s, ap);
        g_fprintf(stderr, "\n");
	va_end(ap);
}

void flo_error(char *s, ...)
{
	va_list ap;
        g_fprintf(stderr, _("ERROR: "));
	va_start(ap, s);
        g_vfprintf(stderr, s, ap);
	g_fprintf(stderr, "\n");
	va_end(ap);
}

void flo_debug(char *s, ...)
{
	if (trace_debug) {
		va_list ap;
		va_start(ap, s);
        	g_vprintf(s, ap);
		g_printf("\n");
		va_end(ap);
	}
}

