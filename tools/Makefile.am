bin_SCRIPTS = florence-ctl
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = florence-ctl.in

do_subst = sed -e 's,[@]pythondir[@],$(pythondir),g' \
		  -e 's,[@]datadir[@],$(datadir),g' \
		  -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
		  -e 's,[@]VERSION[@],$(VERSION),g' 

florence-ctl: florence-ctl.in Makefile
	$(do_subst) < $(srcdir)/florence-ctl.in > florence-ctl
	chmod +x florence-ctl

