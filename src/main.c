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

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include "system.h"
#include "florence.h"
#include "trace.h"
#include "settings.h"

#define EXIT_FAILURE 1

static void usage (int status);

/* The name the program was run with, stripped of any leading path. */
char *program_name=NULL;

/* Option flags and variables */
static struct option const long_options[] =
{
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'V'},
	{"config", no_argument, 0, 'c'},
	{NULL, 0, NULL, 0}
};

static int decode_switches (int argc, char **argv);

int main (int argc, char **argv)
{
	GConfClient *gconfclient;
	GtkWidget *dialog, *label;
	int result;
	int ret=EXIT_FAILURE;
	int config;

	program_name = argv[0];
	config=decode_switches (argc, argv);
	flo_info(_("Florence version %s"), VERSION);

	gtk_init(&argc, &argv);
	gconf_init(argc, argv, NULL);
	g_type_init();

	gconfclient=gconf_client_get_default();
	gconf_client_add_dir(gconfclient, "/desktop/gnome/interface", GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	if (!gconf_client_get_bool(gconfclient, "/desktop/gnome/interface/accessibility", NULL)) {
		dialog=gtk_dialog_new_with_buttons(_("Enable accessibility"), NULL, 
			GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
		label=gtk_label_new(_("Accessibility is disabled.\n"
			"Florence requires that accessibility is enabled to function.\n"
			"Click OK to enable accessibility and restart GNOME.\n"
			"Alternatively, you may enable accessibility with gnome-at-properties."));
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
		gtk_widget_show_all(dialog);
		result=gtk_dialog_run(GTK_DIALOG(dialog));
		if (result==GTK_RESPONSE_ACCEPT) {
			gconf_client_set_bool(gconfclient, "/desktop/gnome/interface/accessibility", TRUE, NULL);
			system("gnome-session-save --kill");
			ret=EXIT_SUCCESS;
		}
	} else if (config) {
		settings_init(TRUE);
		settings();
		settings_exit();
		gtk_main();
	} else { ret=florence(); }

	return ret;
}

/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.  */
static int decode_switches (int argc, char **argv)
{
	int c;
	int ret=0;

	while ((c = getopt_long (argc, argv, 
		"h"  /* help */
		"V"  /* version */
		"c", /* configuration */
		long_options, (int *) 0)) != EOF)
	{
		switch (c)
		{
			case 'V':
				printf ("Florence (%s) %s\n", argv[0], VERSION);
				exit (0);
			case 'h':
				usage (0);
			case 'c':
				ret=1; break;
			default:
				usage (EXIT_FAILURE);
		}
	}

	return ret;
}

static void usage (int status)
{
	printf (_("%s - \
Florence is a simple virtual keyboard for Gnome.\n"), program_name);
	printf (_("Usage: %s [OPTION]...\n"), program_name);
	printf (_("\
Options:\n\
  -h, --help          display this help and exit\n\
  -V, --version	      output version information and exit\n\
  -c, --config        opens configuration window\n\n\
Report bugs to <f.agerch@gmail.com>.\n\
More informations at <http://florence.sourceforge.net>.\n"));
	exit (status);
}

