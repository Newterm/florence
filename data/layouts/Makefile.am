
layoutdir=$(datadir)/florence/layouts
layout_in_files=florence.xml.in florence-alt.xml.in compact.xml.in compact-alt.xml.in
layout_DATA=$(layout_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
$(layout_in_files): $(layout_in_files:.xml.in=.xml.in.in)
	sed -e "s|\@VERSION\@|$(VERSION)|" $(srcdir)/$@.in > $@

EXTRA_DIST = florence.xml.in.in florence-alt.xml.in.in compact.xml.in.in compact-alt.xml.in.in
DISTCLEANFILES = $(layout_DATA) $(layout_in_files)

