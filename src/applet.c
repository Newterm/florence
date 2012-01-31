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

#include <sys/types.h>
#include <getopt.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "system.h"
#ifdef ENABLE_APPLET4
	#include <bonobo/bonobo-ui-component.h>
#endif
#include <panel-applet.h>
#include <gconf/gconf-client.h>
#include "trace.h"
#include "settings.h"
#include "tools.h"
#include "florence.h"

struct florence *florence=NULL;

#ifdef ENABLE_APPLET4
void applet_properties (GtkAction *action, PanelApplet *applet)
#else
void applet_properties (BonoboUIComponent *uic, gpointer *florence, gchar *cname)
#endif
{
	settings();
}

#ifdef ENABLE_APPLET4
void applet_about (GtkAction *action, PanelApplet *applet)
#else
void applet_about (BonoboUIComponent *uic, gpointer *florence, gchar *cname)
#endif
{
	trayicon_about();
}

#ifdef ENABLE_APPLET4
void applet_help (GtkAction *action, PanelApplet *applet)
#else
void applet_help (BonoboUIComponent *uic, gpointer *florence, gchar *cname)
#endif
{
	trayicon_help();
}

static gboolean
florence_applet_factory(PanelApplet *applet,
                        const gchar *iid,
                        gpointer data)
{
	const char *modules;
	char *menu;
#ifdef ENABLE_APPLET4
	static const GtkActionEntry actions[]={
		{ "Properties", GTK_STOCK_PREFERENCES, "Properties", NULL, NULL, G_CALLBACK(applet_properties) },
		{ "About", GTK_STOCK_PREFERENCES, "About", NULL, NULL, G_CALLBACK(applet_about) },
		{ "Help", GTK_STOCK_PREFERENCES, "Help", NULL, NULL, G_CALLBACK(applet_help) },
	};
	GtkActionGroup *action_group;
#else
	static const BonoboUIVerb verbs[]={
		BONOBO_UI_VERB ("Properties", (BonoboUIVerbFn)applet_properties),
		BONOBO_UI_VERB ("About", (BonoboUIVerbFn)applet_about),
		BONOBO_UI_VERB ("Help", (BonoboUIVerbFn)applet_help),
		BONOBO_UI_VERB_END
	};
#endif

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, FLORENCELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#ifdef ENABLE_APPLET4
	menu=g_strdup_printf(
		"<menuitem name=\"Properties\" action=\"Properties\" _label=\"%s\" pixtype=\"stock\" pixname=\"gtk-properties\"/>\
		 <menuitem name=\"About\" action=\"About\" _label=\"%s\" pixtype=\"stock\" pixname=\"gtk-about\"/>\
		 <menuitem name=\"Help\" action=\"Help\" _label=\"%s\" pixtype=\"stock\" pixname=\"gtk-help\"/>", _("_Preferences..."), _("_About..."), _("_Help..."));
#else
	menu=g_strdup_printf(
		"<popup name=\"button3\">\
		        <menuitem name=\"Properties\" verb=\"Properties\" _label=\"%s\" pixtype=\"stock\" pixname=\"gtk-properties\"/>\
		        <menuitem name=\"About\" verb=\"About\" _label=\"%s\" pixtype=\"stock\" pixname=\"gtk-about\"/>\
		        <menuitem name=\"Help\" verb=\"Help\" _label=\"%s\" pixtype=\"stock\" pixname=\"gtk-help\"/>\
		</popup>", _("_Preferences..."), _("_About..."), _("_Help..."));
#endif

	g_return_val_if_fail (PANEL_IS_APPLET (applet), FALSE);

#ifdef ENABLE_APPLET4
	action_group=gtk_action_group_new("Florence actions");
	gtk_action_group_add_actions(action_group, action_group, G_N_ELEMENTS(action_group), applet);
	panel_applet_setup_menu(PANEL_APPLET (applet), menu, action_group);
#else
	panel_applet_setup_menu(PANEL_APPLET (applet),
		menu,
		verbs,
		florence);
	g_free(menu);
#endif

	modules = g_getenv("GTK_MODULES");
	if (!modules||modules[0]=='\0') putenv("GTK_MODULES=gail:atk-bridge");
        florence = flo_new(TRUE, NULL, applet);
	
	gtk_widget_show_all (GTK_WIDGET (applet));

	return TRUE;
}

int main (int argc, char *argv [])
{
	GOptionContext *context;
	GError *error;
	int retval;

	context = g_option_context_new ("");
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context,
				    bonobo_activation_get_goption_group ());
	
	error = NULL;
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		if (error) {
			g_printerr (_("Cannot parse arguments: %s.\n"),
				    error->message);
			g_error_free (error);
		} else
			g_printerr (_("Cannot parse arguments.\n"));
		g_option_context_free (context);
		return 1;
	}
	
	gtk_init (&argc, &argv);
	gconf_init(argc, argv, NULL);
	g_type_init();	

	if (!bonobo_init (&argc, argv)) {
		g_printerr (_("Cannot initialize bonobo.\n"));
		return 1;
	}
	
	settings_init(FALSE, NULL);
	trace_init(TRACE_WARNING);

	retval = panel_applet_factory_main ("OAFIID:GNOME_FlorenceApplet_Factory", 
					    PANEL_TYPE_APPLET, 
					    florence_applet_factory,
					    NULL);
	g_option_context_free (context);
	if (florence) flo_free(florence);
	
	return retval;
}


