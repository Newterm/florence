bin_PROGRAMS = florence

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c
florence_CPPFLAGS = -DCONFIGFILE="\"$(sysconfdir)/florence.conf\"" $(DEPS_CFLAGS)
florence_LDADD = $(DEPS_LIBS)

