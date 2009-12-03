
styledir = $(datadir)/florence/styles/default
style_in_files = symbols.xml.in
style_DATA = florence.style florence.defs $(style_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST = $(style_DATA) $(style_in_files)
DISTCLEANFILES = $(style_in_files:.xml.in=.xml)

