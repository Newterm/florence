bin_PROGRAMS = florence

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c settings.c trayicon.c layoutreader.c
florence_CPPFLAGS = -DICONDIR="\"$(datadir)/icons\"" -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS)
florence_LDADD = $(DEPS_LIBS)

