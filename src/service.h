/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2012 François Agrech

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#include <gio/gio.h>
#include "view.h"

/* Service object */
struct service {
	GDBusConnection *connection;
	guint owner_id;
	GDBusNodeInfo *introspection_data;
	struct view *view;
	GCallback quit; /* Callback called to quit the applications (when the terminate method is called) */
};

/* Send the terminate signal */
void service_terminate(struct service *service);

/* Create a service object */
struct service *service_new(struct view *view, GCallback quit);
/* Destroy a service object */
void service_free(struct service *service);

