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

/* debug level */
enum trace_level {
	TRACE_SEVERE,
	TRACE_ERROR,
	TRACE_WARNING,
	TRACE_DEBUG,
	TRACE_HIDEBUG
};

/* initializes the trace module. Must be called before any trace function
 * level indicates the minimum debug level of errors to display */
void trace_init(enum trace_level level);
/* liberate any memory used by the trace module */
void trace_exit();
/* return trace level parsed from the string argument */
enum trace_level trace_parse_level(char *s);

void flo_fatal(char *s, ...);
void flo_error(char *s, ...);
void flo_warn(char *s, ...);
void flo_warn_distinct(char *s, ...);
void flo_info(char *s, ...);
void flo_info_distinct(char *s, ...);
void flo_debug(enum trace_level level, char *s, ...);
void flo_debug_distinct(enum trace_level level, char *s, ...);

void flo_start_func(int line, const char *func, const char *file);
void flo_end_func(int line, const char *func, const char *file);

#if __GNUC__ >= 2
#define FLO_FUNC __PRETTY_FUNCTION__
#else
#define FLO_FUNC __func__
#endif

#define START_FUNC (flo_start_func((__LINE__), (FLO_FUNC), (__FILE__)));
#define END_FUNC (flo_end_func((__LINE__), (FLO_FUNC), (__FILE__)));

