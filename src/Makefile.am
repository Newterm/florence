bin_PROGRAMS = florence

INCLUDES = -I$(top_srcdir) -DFLORENCELOCALEDIR=\""$(florencelocaledir)"\"

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c settings.c trayicon.c\
                   layoutreader.c style.c view.c status.c tools.c
florence_CPPFLAGS = -DICONDIR="\"$(ICONDIR)\""\
   -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS)\
   $(LIBGNOME_CFLAGS) $(LIBNOTIFY_CFLAGS) $(XTST_CFLAGS) $(AT_SPI_CFLAGS) $(INCLUDES)
florence_LDADD = $(DEPS_LIBS) $(LIBGNOME_LIBS) $(LIBNOTIFY_LIBS) $(XTST_LIBS) $(AT_SPI_LIBS)

