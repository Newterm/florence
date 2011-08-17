bin_PROGRAMS = florence
if WITH_PANELAPPLET
   bin_PROGRAMS += florence_applet
endif

INCLUDES = -I$(top_srcdir) -DFLORENCELOCALEDIR=\""$(florencelocaledir)"\"

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c settings.c trayicon.c\
                   layoutreader.c style.c view.c status.c tools.c settings-window.c\
                   xkeyboard.c

if WITH_RAMBLE
   florence_SOURCES += ramble.c
endif

florence_CPPFLAGS = -DICONDIR="\"$(ICONDIR)\""\
   -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS)\
   $(LIBGNOME_CFLAGS) $(LIBNOTIFY_CFLAGS) $(XTST_CFLAGS) $(AT_SPI_CFLAGS) $(AT_SPI2_CFLAGS) $(INCLUDES)
florence_LDADD = $(DEPS_LIBS) $(LIBM) $(X11_LIBS) $(LIBGNOME_LIBS) $(LIBNOTIFY_LIBS) $(XTST_LIBS) $(AT_SPI2_LIBS) $(AT_SPI_LIBS)

if WITH_PANELAPPLET

florence_applet_SOURCES = applet.c florence.c keyboard.c key.c trace.c settings.c trayicon.c\
                   layoutreader.c style.c view.c status.c tools.c settings-window.c xkeyboard.c

if WITH_RAMBLE
   florence_applet_SOURCES += ramble.c
endif

florence_applet_CPPFLAGS = -DAPPLET -DICONDIR="\"$(ICONDIR)\""\
   -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS)\
   $(LIBGNOME_CFLAGS) $(LIBNOTIFY_CFLAGS) $(LIBPANELAPPLET_CFLAGS) $(XTST_CFLAGS) $(AT_SPI_CFLAGS) $(AT_SPI2_CFLAGS) $(INCLUDES)

florence_applet_LDADD = $(DEPS_LIBS) $(LIBGNOME_LIBS) $(LIBNOTIFY_LIBS) $(LIBPANELAPPLET_LIBS) $(XTST_LIBS) $(AT_SPI_LIBS) $(AT_SPI2_LIBS)

serverdir       = $(libdir)/bonobo/servers
server_in_files = florence.server.in
server_DATA     = $(server_in_files:.server.in=.server)
 
$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

@INTLTOOL_SERVER_RULE@

endif

EXTRA_DIST = florence.h keyboard.h key.h layoutreader.h settings.h settings-window.h\
             status.h style.h system.h tools.h trace.h trayicon.h view.h xkeyboard.h\
             ramble.h florence.server.in.in
 
DISTCLEANFILES = $(server_in_files) $(server_DATA)

