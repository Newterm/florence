bin_PROGRAMS = florence

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c settings.c trayicon.c
florence_CPPFLAGS = -DICONDIR="\"$(datadir)/icons/apps/florence\"" -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS)
florence_LDADD = $(DEPS_LIBS)

