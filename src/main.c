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

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include "system.h"
#include "trace.h"
#include "settings.h"
#include "tools.h"
#include "florence.h"

#define EXIT_FAILURE 1

/* The name the program was run with, stripped of any leading path. */
char *program_name=NULL;
/* config file, if given as argument, or NULL. */
char *config_file=NULL;
/* focus window name, if given as argument, or NULL */
char *focus=NULL;

/* Option flags and variables */
static struct option const long_options[] =
{
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'V'},
	{"config", no_argument, 0, 'c'},
	{"debug", no_argument, 0, 'd'},
	{"no-gnome", no_argument, 0, 'n'},
	{"focus", optional_argument, 0, 'f'},
	{"use-config", required_argument, 0, 'u'},
	{NULL, 0, NULL, 0}
};

static void usage (int status);
static int decode_switches (int argc, char **argv);

int main (int argc, char **argv)
{
	struct florence *florence;
	int ret=EXIT_FAILURE;
	int config;
	const char *modules;

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, FLORENCELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);
	gconf_init(argc, argv, NULL);
	g_type_init();

	program_name=argv[0];
	config=decode_switches (argc, argv);
	trace_init(config&2);
	flo_info(_("Florence version %s"), VERSION);
#ifdef ENABLE_XRECORD
	flo_warn(_("Xorg RECORD extension is severely broken since Xorg 1.6: see bugs http://bugs.freedesktop.org/show_bug.cgi?id=20500 and http://bugs.freedesktop.org/show_bug.cgi?id=21971 ; Please disable xrecord if you are using a recent version of Xorg: --without-xrecord configure option. Use AT-SPI instead. Since XEVIE was dropped from Xorg some months ago, there is no way to provide the same functionality for now. Sorry for the inconvenience."));
#else
	flo_info(_("XRECORD has been disabled at compile time."));
#endif

	if (config&1) {
		settings_init(TRUE, config_file);
		settings();
		gtk_main();
		settings_exit();
	} else {
		settings_init(FALSE, config_file);
	        modules = g_getenv("GTK_MODULES");
		if (!modules||modules[0]=='\0') putenv("GTK_MODULES=gail:atk-bridge");
		florence=flo_new(!(config&4), focus);

		gtk_main();

		settings_exit();
		flo_free(florence);
		putenv("AT_BRIDGE_SHUTDOWN=1");
		ret=EXIT_SUCCESS;
	}
	if (config_file) g_free(config_file);
	if (focus) g_free(focus);

	trace_exit();
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
		"c"  /* configuration */
		"d"  /* debug */
		"n"  /* no gnome */
		"r"  /* restore focus */
		"t"  /* keep bringing back to front */
		"u", /* use config file */
		long_options, (int *) 0)) != EOF)
	{
		switch (c)
		{
			case 'V':printf ("Florence (%s) %s\n", argv[0], VERSION);
				exit (0);
				break;
			case 'h':usage (0);
			case 'c':ret|=1; break;
			case 'd':ret|=2; break;
			case 'n':ret|=4; break;
			case 'f':if (g_strdup(optarg)) focus=g_strdup(optarg);
				else focus=g_strdup("");
				break;
			case 'u':config_file=g_strdup(optarg);break;
			default:usage (EXIT_FAILURE); break;
		}
	}

	return ret;
}

/* Print usage message and exit */
static void usage (int status)
{
	printf (_("%s - \
Florence is a simple virtual keyboard for Gnome.\n"), program_name);
	printf (_("Usage: %s [OPTION]...\n"), program_name);
	printf (_("\
Options:\n\
  -h, --help              display this help and exit\n\
  -V, --version	          output version information and exit\n\
  -c, --config            open configuration window\n\
  -d, --debug             print debug information to stdout\n\
  -n, --no-gnome          use this flag if you are not using GNOME\n\
  -f, --focus [window]    give the focus to the window\n\
  -u, --use-config file   use the given config file instead of gconf\n\n\
Report bugs to <f.agrech@gmail.com>.\n\
More informations at <http://florence.sourceforge.net>.\n"));
	exit (status);
}

