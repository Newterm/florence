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

#include "system.h"
#include "trace.h"
#include "service.h"

/* Service interface */
static const gchar service_introspection[]=
	"<node>"
	"  <interface name='org.florence.Keyboard'>"
	"    <method name='show'/>"
	"    <method name='move'>"
	"      <arg type='u' name='x' direction='in'/>"
	"      <arg type='u' name='y' direction='in'/>"
	"    </method>"
	"    <method name='hide'/>"
	"    <method name='terminate'/>"
	"    <signal name='terminate'/>"
	"  </interface>"
	"</node>";

/* Called when a dbus method is called */
static void service_method_call (GDBusConnection *connection, const gchar *sender,
	const gchar *object_path, const gchar *interface_name, const gchar *method_name,
	GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
	START_FUNC
	guint x, y;
	struct service *service=(struct service *)user_data;
	if (g_strcmp0(method_name, "show")==0) {
#ifdef AT_SPI
		view_show(service->view, NULL);
#else
		view_show(service->view);
#endif
	} else if (g_strcmp0(method_name, "move")==0) {
		g_variant_get(parameters, "(uu)", &x, &y);
		gtk_window_move(GTK_WINDOW(view_window_get(service->view)), x, y);
	} else if (g_strcmp0(method_name, "hide")==0) view_hide(service->view);
	else if (g_strcmp0(method_name, "terminate")==0) service->quit();
	else flo_error(_("Unknown dbus method called: <%s>"), method_name);
	g_dbus_method_invocation_return_value(invocation, NULL);
	END_FUNC
}

/* Called when dbus has been acquired */
static void service_on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	START_FUNC
	struct service *service=(struct service *)user_data;
	static const GDBusInterfaceVTable vtable={ service_method_call, NULL, NULL };
	g_dbus_connection_register_object(connection, "/org/florence/Keyboard",
		service->introspection_data->interfaces[0], &vtable, user_data, NULL, NULL);
	service->connection=connection;
	END_FUNC
}

/* Called when dbus name has been acquired */
static void service_on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	START_FUNC
	flo_info(_("DBus name aquired: %s"), name);
	END_FUNC
}

/* Called when dbus name has been lost */
static void service_on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	START_FUNC
	flo_warn(_("Service name lost."));
	END_FUNC
}

/* Create a service object */
struct service *service_new(struct view *view, GCallback quit)
{
	START_FUNC
	struct service *service=(struct service *)g_malloc(sizeof(struct service));
	if (!service) flo_fatal(_("Unable to allocate memory for dbus service"));
	memset(service, 0, sizeof(struct service));
	service->introspection_data=g_dbus_node_info_new_for_xml(service_introspection, NULL);
	service->owner_id=g_bus_own_name(G_BUS_TYPE_SESSION, "org.florence.Keyboard",
		G_BUS_NAME_OWNER_FLAGS_NONE, service_on_bus_acquired, service_on_name_acquired,
		service_on_name_lost, service, NULL);
	service->view=view;
	service->quit=quit;
	END_FUNC
	return service;
}

/* Destroy a service object */
void service_free(struct service *service)
{
	START_FUNC
	g_bus_unown_name(service->owner_id);
	g_dbus_node_info_unref(service->introspection_data);
	g_free(service);
	END_FUNC
}

/* Send the terminate signal */
void service_terminate(struct service *service)
{
	GError *error=NULL;
	g_dbus_connection_emit_signal(service->connection, NULL, "/org/florence/Keyboard",
		"org.florence.Keyboard", "terminate", NULL, &error);
	if (error) flo_error(_("Error emitting terminate signal: %s"), error->message);
}

