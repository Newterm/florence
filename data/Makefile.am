
SUBDIRS=styles layouts relaxng svg11

gsettings_in_files=org.florence.gschema.xml.in
gsettings_SHEMAS=$(gsettings_in_files:.gshema.xml.in=.gschema.xml)
@INTLTOOL_XML_NOMERGE_RULE@
#gsettings_SCHEMAS=org.florence.gschema.xml
@GSETTINGS_RULES@

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

EXTRA_DIST = $(config_DATA) $(gtkbuilder_DATA) $(icons_DATA) $(meta_DATA)\
	     $(gsettings_SCHEMAS) $(desktop_in_files:.desktop.in=.desktop.in.in)
DISTCLEANFILES = $(desktop_DATA) $(desktop_in_files) $(desktop_DATA) $(gsettings_SCHEMAS)

