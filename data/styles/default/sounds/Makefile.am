
soundsdir = $(datadir)/florence/styles/default/sounds
sounds_in_files = sounds.xml
$(sounds_in_files): $(sounds_in_files:.xml=.xml.in)
	sed -e "s|\@SNDDIR\@|$(soundsdir)|" $< > $@
sounds_DATA=press.ogg release.ogg return_press.ogg return_release.ogg\
	modifier_press.ogg modifier_release.ogg $(sounds_in_files:.xml.in=.xml)

