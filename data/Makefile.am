
GCONFTOOL=gconftool-2
GCONFREFRESH=$(srcdir)/../gconf-refresh

schemadir=@GCONF_SCHEMA_FILE_DIR@
schema_DATA=florence.schemas

desktopdir=$(datadir)/applications
desktop_DATA=florence.desktop

iconsdir=$(datadir)/icons
icons_DATA=florence.svg

gladedir=$(datadir)/florence
glade_DATA=florence.glade 

layoutdir=$(datadir)/florence
layout_DATA=florence.layout florence.rnc florence.css

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
	$(GCONFREFRESH)

