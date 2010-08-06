
SUBDIRS=styles layouts relaxng svg11

GCONFTOOL=gconftool-2

schemadir=@GCONF_SCHEMA_FILE_DIR@
schemas_in_files=florence.schemas.in
schema_DATA=$(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

desktopdir=$(datadir)/applications
desktop_in_files=florence.desktop.in
desktop_DATA=$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
$(desktop_in_files): $(desktop_in_files:.desktop.in=.desktop.in.in)
	sed -e "s|\@ICONDIR\@|$(ICONDIR)|" $< > $@

iconsdir=$(datadir)/pixmaps
icons_DATA=florence.svg

gtkbuilderdir=$(datadir)/florence
gtkbuilder_DATA=florence.glade

metadir=$(datadir)/florence
meta_DATA=florence.rnc florence.css

configdir=$(datadir)/florence
config_DATA=florence.conf

install-data-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(builddir)/$(schema_DATA)

uninstall-local: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $(builddir)/$(schema_DATA)

EXTRA_DIST = $(config_DATA) $(gtkbuilder_DATA) $(icons_DATA) $(meta_DATA)\
	     $(schemas_in_files) $(desktop_in_files:.desktop.in=.desktop.in.in)
DISTCLEANFILES = $(desktop_DATA) $(desktop_in_files) $(desktop_DATA) $(schema_DATA)

