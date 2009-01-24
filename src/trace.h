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

/* initializes the trace module. Must be called before any trace function
 * debug is a boolean. If it's true, the trace module will print debug informations */
void trace_init(int debug);

void flo_fatal (char *s, ...);
void flo_error (char *s, ...);
void flo_warn (char *s, ...);
void flo_info (char *s, ...);
void flo_debug (char *s, ...);

#if __GNUC__ >= 2
#define FLO_FUNC __PRETTY_FUNCTION__
#else
#define FLO_FUNC "<unknown>"
#endif

#define flo_start_func() (g_printf("%d: %s\n", (__LINE__), (FLO_FUNC)))

