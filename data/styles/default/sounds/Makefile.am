
soundsdir = $(datadir)/florence/styles/default/sounds
sounds_in_files = sounds.xml
$(sounds_in_files): $(sounds_in_files:.xml=.xml.in)
	sed -e "s|\@SNDDIR\@|$(soundsdir)|" $< > $@
sounds_DATA=press.ogg release.ogg $(sounds_in_files:.xml.in=.xml)

EXTRA_DIST = $(sounds_DATA) $(sounds_in_files)
DISTCLEANFILES = $(sounds_in_files:.xml.in=.xml)

