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
#include "system.h"
#include "florence.h"

#define EXIT_FAILURE 1

static void usage (int status);

/* The name the program was run with, stripped of any leading path. */
char *program_name=NULL;
char *config_file=NULL;
char default_config_file[]=CONFIGFILE;

/* Option flags and variables */
static struct option const long_options[] =
{
	{"config", required_argument, 0, 'c'},
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'V'},
	{NULL, 0, NULL, 0}
};

static int decode_switches (int argc, char **argv);

int main (int argc, char **argv)
{
	int i;

	program_name = argv[0];

	i = decode_switches (argc, argv);

	gtk_init (&argc, &argv);

	if (!config_file) config_file=(char*)default_config_file;
	return florence(config_file);
}

/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.  */
static int
decode_switches (int argc, char **argv)
{
int c;

	while ((c = getopt_long (argc, argv, 
		"c"  /* config file */
		"h"  /* help */
		"V", /* version */
		long_options, (int *) 0)) != EOF)
{
	switch (c)
	{
		case 'V':
			printf ("florence %s\n", VERSION);
			exit (0);
		case 'h':
			usage (0);
		case 'c':
			config_file=optarg;
			break;
		default:
			usage (EXIT_FAILURE);
		}
	}

	return optind;
}

static void usage (int status)
{
	printf (_("%s - \
Florence is a simple virtual keyboard for Gnome.\n"), program_name);
	printf (_("Usage: %s [OPTION]...\n"), program_name);
	printf (_("\
Options:\n\
  -c, --config=<file>        use configuration <file> instead of default /etc/florence.conf\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\n\
Report bugs to <f.agerch@gmail.com>.\n"), program_name);
	exit (status);
}

