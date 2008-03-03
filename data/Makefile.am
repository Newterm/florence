
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
layout_DATA=florence.layout

install-data-local:
	sed -i "s!@LAYOUTDIR@!$(datadir)/florence!g" $(srcdir)/$(schema_DATA) #UGLY HACK
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
	$(GCONFREFRESH)
	sed -i "s!$(datadir)/florence!@LAYOUTDIR@!g" $(srcdir)/$(schema_DATA) #EVEN UGLIER HACK

