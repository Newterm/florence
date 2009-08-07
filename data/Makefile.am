
SUBDIRS=styles layouts relaxng svg11

GCONFTOOL=gconftool-2
GCONFREFRESH=$(srcdir)/../gconf-refresh

schemadir=@GCONF_SCHEMA_FILE_DIR@
schemas_in_files=florence.schemas.in
schema_DATA=$(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

desktopdir=$(datadir)/applications
desktop_in_files=florence.desktop.in
desktop_DATA=$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

iconsdir=$(datadir)/pixmaps
icons_DATA=florence.svg

gladedir=$(datadir)/florence
glade_DATA=florence.glade 

metadir=$(datadir)/florence
meta_DATA=florence.rnc florence.css

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)

uninstall-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $(srcdir)/$(schema_DATA)
