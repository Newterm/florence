bin_PROGRAMS = florence

INCLUDES = -I$(top_srcdir) -DFLORENCELOCALEDIR=\""$(florencelocaledir)"\"

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c settings.c trayicon.c\
                   layoutreader.c style.c view.c status.c tools.c settings-window.c\
                   xkeyboard.c fsm.c service.c

if WITH_RAMBLE
   florence_SOURCES += ramble.c
endif

florence_CPPFLAGS = -DICONDIR="\"$(ICONDIR)\""\
   -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS) $(GTK3_CFLAGS)\
   $(LIBGNOME_CFLAGS) $(LIBNOTIFY_CFLAGS) $(XTST_CFLAGS) $(AT_SPI_CFLAGS) $(AT_SPI2_CFLAGS) $(INCLUDES)
florence_LDADD = $(DEPS_LIBS) $(LIBM) $(X11_LIBS) $(LIBGNOME_LIBS) $(LIBNOTIFY_LIBS)\
   $(XTST_LIBS) $(AT_SPI2_LIBS) $(AT_SPI_LIBS) $(GTK3_LIBS)

EXTRA_DIST = florence.h keyboard.h key.h layoutreader.h settings.h settings-window.h\
             status.h style.h system.h tools.h trace.h trayicon.h view.h xkeyboard.h\
             ramble.h fsm.h service.h florence.server.in.in
 
DISTCLEANFILES = $(server_in_files) $(server_DATA)

