
SUBDIRS=styles

GCONFTOOL=gconftool-2
GCONFREFRESH=$(srcdir)/../gconf-refresh

schemadir=@GCONF_SCHEMA_FILE_DIR@
schema_DATA=florence.schemas

desktopdir=$(datadir)/applications
desktop_in_files=florence.desktop.in
desktop_DATA=$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

iconsdir=$(datadir)/icons
icons_DATA=florence.svg

gladedir=$(datadir)/florence
glade_DATA=florence.glade 

layoutdir=$(datadir)/florence
layout_in_files=florence.xml.in
layout_DATA=$(layout_in_files:.xml.in=.xml) florence.rnc florence.css
@INTLTOOL_XML_RULE@

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
	$(GCONFREFRESH)

