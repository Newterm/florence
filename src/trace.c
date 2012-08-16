/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2012 François Agrech

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
#include <glib.h>
#include <stdio.h>
#include <stdarg.h>

/* Global trace level. */
static enum trace_level trace_debug_level;
/* the buffer records checksum of messages already printed. Used not to print the same message twice */
static GSList *trace_buffer=NULL;
/* indentation of traces */
#define TRACE_MAX_INDENT 64
char trace_indent[TRACE_MAX_INDENT];
/* function check sum */
int trace_fn_cksum[TRACE_MAX_INDENT];

gint g_vfprintf(FILE *file, gchar const *format, va_list args);
gint g_fprintf(FILE *file, gchar const *format, ...) G_GNUC_PRINTF (2, 3);

/* calculate a simple checksum of a string. */
int trace_cksum(const char *str)
{
	int idx, ret=0;
	for(idx=0; str[idx]!='\0'; idx++) ret+=str[idx];
	return ret;
}

/* prints a message to stream f, ignores it if there exists a duplicate in the buffer. */
void trace_distinct_msg(char *prefix, FILE *f, char *s, va_list args)
{
	GSList *list=trace_buffer;
	gchar *str=g_strdup_vprintf((gchar *)s, args);
	int cksum=trace_cksum((const char *)str);
	int *pcksum;
	gboolean ignore=FALSE;
	while (list) {
		if ((*(int *)(list->data))==cksum) {
			ignore=TRUE;
			break;
		}
		list=list->next;
	}
	if (!ignore) {
		pcksum=g_malloc(sizeof(int));
		*pcksum=cksum;
		trace_buffer=g_slist_append(trace_buffer, pcksum);
		g_fprintf(f, "%s%s%s", trace_indent, prefix, str);
		g_fprintf(f, "\n");
	}
	g_free(str);
}

/* prints a message to stream f. */
void trace_msg(char *prefix, FILE *f, char *s, va_list args)
{
	gchar *str=g_strdup_vprintf((gchar *)s, args);
	g_fprintf(f, "%s%s%s", trace_indent, prefix, str);
	g_fprintf(f, "\n");
	g_free(str);
}

/********************/
/* public functions */
/********************/

/* initializes the trace module. Must be called before any trace function
 * debug is a boolean. If it's true, the trace module will print debug informations */
void trace_init(enum trace_level debug_level)
{
	trace_debug_level=debug_level;
	memset(trace_indent, 0, sizeof(trace_indent));
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

/* return trace level parsed from the string argument */
enum trace_level trace_parse_level(char *s)
{
	enum trace_level ret=TRACE_WARNING;
	if (!strcmp(s, "fatal")) ret=TRACE_SEVERE;
	else if (!strcmp(s, "error")) ret=TRACE_ERROR;
	else if (!strcmp(s, "warning")) ret=TRACE_WARNING;
	else if (!strcmp(s, "debug")) ret=TRACE_DEBUG;
	else if (!strcmp(s, "hidebug")) ret=TRACE_HIDEBUG;
	else flo_info(_("Unable to parse debug level <%s>, using default <warning>"), s);
	if (ret>=TRACE_DEBUG) flo_info(_("Setting debug level to <%s>"), s);
	return ret;
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

void flo_info_distinct(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	trace_distinct_msg("", stdout, s, ap);
	va_end(ap);
}

void flo_warn(char *s, ...)
{
	if (trace_debug_level>=TRACE_WARNING) {
		va_list ap;
		va_start(ap, s);
		trace_msg(_("WARNING: "), stderr, s, ap);
		va_end(ap);
	}
}

void flo_warn_distinct(char *s, ...)
{
	if (trace_debug_level>=TRACE_WARNING) {
		va_list ap;
		va_start(ap, s);
		trace_distinct_msg(_("WARNING: "), stderr, s, ap);
		va_end(ap);
	}
}

void flo_error(char *s, ...)
{
	if (trace_debug_level>=TRACE_ERROR) {
		va_list ap;
		va_start(ap, s);
		trace_msg(_("ERROR: "), stderr, s, ap);
		va_end(ap);
	}
}

void flo_debug(enum trace_level level, char *s, ...)
{
	if (trace_debug_level>=level) {
		va_list ap;
		va_start(ap, s);
		trace_msg("", stdout, s, ap);
		va_end(ap);
	}
}

void flo_debug_distinct(enum trace_level level, char *s, ...)
{
	if (trace_debug_level>=level) {
		va_list ap;
		va_start(ap, s);
		trace_distinct_msg("", stdout, s, ap);
		va_end(ap);
	}
}

void flo_start_func(int line, const char *func, const char *file)
{
	int indent;
	if (trace_debug_level>=TRACE_HIDEBUG) {
		g_fprintf(stdout, "%s<%s@%s:%d>\n", trace_indent, func, file, line);
		indent=strlen(trace_indent);
		trace_fn_cksum[indent]=trace_cksum(func);
		if (indent<TRACE_MAX_INDENT) trace_indent[indent]=' ';
		else { flo_fatal(_("Too many function levels at function <%s>."), func); }
	}
}

void flo_end_func(int line, const char *func, const char *file)
{
	int indent;
	if (trace_debug_level>=TRACE_HIDEBUG) {
		indent=strlen(trace_indent)-1;
		if (trace_fn_cksum[indent]!=trace_cksum(func)) flo_fatal(_("Trace function checksum error at function <%s>."), func);
		if (indent>=0) trace_indent[indent]='\0';
		else flo_fatal(_("Trace indent error at function <%s>."), func);
		g_fprintf(stdout, "%s</%s@%s:%d>\n", trace_indent, func, file, line);
	}
}

